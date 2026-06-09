#include "rune_stones.h"
#include "rune_stones_loaders.h"

#include "engine/db/global_objects.h"
#include "engine/db/db.h"                    // GetObjRnum
#include "engine/db/obj_prototypes.h"        // obj_proto (assign the stone spec proc)
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/core/handler.h"             // PlaceObjToRoom
#include "engine/ui/modify.h"
#include "engine/ui/interpreter.h"           // CMD_IS / cmd_info
#include "engine/ui/table_wrapper.h"
#include "gameplay/abilities/abilities_constants.h"
#include "gameplay/skills/skills_info.h"    // MUD::Skills()[..].GetName() for {skill_name}
#include "engine/structs/msg_container.h"
#include "engine/structs/info_container.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"
#include "utils/utils_string.h"              // isname
#include "utils/mud_string.h"                // one_argument

#include <fmt/format.h>
#include <algorithm>

// ============================================ MESSAGES =======================================================

namespace {
const std::map<ERuneStoneMsg, std::string> kRuneStoneMsgNames{
		{ERuneStoneMsg::kUndefined, "kUndefined"},
		{ERuneStoneMsg::kName, "kName"},
		{ERuneStoneMsg::kRoomNormal, "kRoomNormal"},
		{ERuneStoneMsg::kRoomDamaged, "kRoomDamaged"},
		{ERuneStoneMsg::kInspectNormal, "kInspectNormal"},
		{ERuneStoneMsg::kInspectDamaged, "kInspectDamaged"},
		{ERuneStoneMsg::kLackSkillNormal, "kLackSkillNormal"},
		{ERuneStoneMsg::kLackSkillDamaged, "kLackSkillDamaged"},
		{ERuneStoneMsg::kListEmpty, "kListEmpty"},
		{ERuneStoneMsg::kListHeader, "kListHeader"},
		{ERuneStoneMsg::kListCount, "kListCount"},
		{ERuneStoneMsg::kListLimit, "kListLimit"},
		{ERuneStoneMsg::kMemorized, "kMemorized"},
		{ERuneStoneMsg::kForgotten, "kForgotten"},
		{ERuneStoneMsg::kMemoryFull, "kMemoryFull"},
		{ERuneStoneMsg::kCantMemorize, "kCantMemorize"},
		{ERuneStoneMsg::kCantForget, "kCantForget"},
	};

msg_container::MsgContainer<int, ERuneStoneMsg> &RuneStoneMsgContainer() {
	static msg_container::MsgContainer<int, ERuneStoneMsg> container;
	return container;
}
}  // namespace

template<>
const std::string &NAME_BY_ITEM<ERuneStoneMsg>(const ERuneStoneMsg item) {
	return kRuneStoneMsgNames.at(item);
}
template<>
const std::map<ERuneStoneMsg, std::string> &NAMES_OF<ERuneStoneMsg>() {
	return kRuneStoneMsgNames;
}
template<>
ERuneStoneMsg ITEM_BY_NAME<ERuneStoneMsg>(const std::string &name) {
	static std::map<std::string, ERuneStoneMsg> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kRuneStoneMsgNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

const std::string &RuneStoneMsg(ERuneStoneMsg slot) {
	return RuneStoneMsgContainer().GetMessage(info_container::kUndefinedVnum, slot);
}

const std::string &RuneStoneName(int stone_vnum) {
	return RuneStoneMsgContainer().GetMessage(stone_vnum, ERuneStoneMsg::kName);
}

void RuneStoneMessagesLoader::Load(parser_wrapper::DataNode data) { RuneStoneMsgContainer().Init(data.Children()); }
void RuneStoneMessagesLoader::Reload(parser_wrapper::DataNode data) { RuneStoneMsgContainer().Reload(data.Children()); }

// ---- Vedun editable surface for the message file (vnum-keyed sheaves, mirrors guild_messages) ------

std::string RuneStoneMessagesLoader::EditableWhat() const { return "runestonemsg"; }

std::vector<cfg_manager::EditableElement> RuneStoneMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : RuneStoneMsgContainer()) {
		const int id = sheaf.GetId();
		if (id == info_container::kUndefinedVnum) {
			out.push_back({"kDefault", "runestone messages (shared)"});
		} else {
			out.push_back({std::to_string(id),
					"runestone '" + RuneStoneName(id) + "' (vnum " + std::to_string(id) + ")"});
		}
	}
	return out;
}

cfg_manager::ValidationResult RuneStoneMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (RuneStoneMsgContainer().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Runestone-message data failed to parse (see syslog for the offending sheaf/message)."};
}

std::string RuneStoneMessagesLoader::CanonicalElementId(const std::string &id) const {
	if (id == "kDefault") {
		return "kDefault";
	}
	// A per-stone sheaf is keyed by a non-negative stone vnum.
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode RuneStoneMessagesLoader::CreateElementNode(parser_wrapper::DataNode root,
																	const std::string &id) const {
	// An empty sheaf for `id`; the editor then adds <message> children.
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

// ============================================ RUNESTONE / ROSTER =============================================

void Runestone::SetEnabled(bool enabled) {
	if (state_ != State::kForbidden) {
		state_ = (enabled ? State::kEnabled : State::kDisabled);
	}
}

RunestoneRoster::RunestoneRoster() {
	incorrect_stone_ = Runestone("undefined", kNowhere, ESkill::kTownportal, 0, "", Runestone::State::kForbidden);
}

void RunestoneRoster::Load(parser_wrapper::DataNode data) {
	clear();
	const char *proto = data.GetValue("prototype");
	prototype_vnum_ = proto ? parse::ReadAsInt(proto) : 0;

	for (auto &node : data.Children()) {
		if (std::string(node.GetName()) != "stone") {
			continue;
		}
		const int stone_vnum = parse::ReadAsInt(node.GetValue("vnum"));
		std::string id = node.GetValue("id") ? node.GetValue("id") : "";
		const auto room_vnum = parse::ReadAsInt(node.GetValue("room_vnum"));
		if (GetRoomRnum(room_vnum) == kNowhere) {
			log("SYSERROR: rune_stones.xml: stone vnum %d room_vnum %d not found -- skipped.", stone_vnum, room_vnum);
			continue;
		}
		ESkill skill = ESkill::kTownportal;
		int skill_level = 1;
		if (node.GoToChild("skill")) {
			try {
				skill = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
				skill_level = parse::ReadAsInt(node.GetValue("val"));
			} catch (const std::exception &e) {
				log("SYSERROR: rune_stones.xml: stone vnum %d bad <skill> (%s).", stone_vnum, e.what());
			}
			node.GoToParent();
		}
		emplace_back(std::string(RuneStoneName(stone_vnum)), room_vnum, skill, skill_level, id,
				Runestone::State::kEnabled, stone_vnum);
	}
}

// Vedun dry-run: structurally parse every <stone> without touching the live roster. Mirrors Load's
// parsing (vnum/room_vnum ints, <skill> id resolvable to ESkill, val an int) but does NOT require the
// room to exist -- Load skips a missing-room stone with a warning, so editing one must stay possible.
bool RunestoneRoster::Validate(parser_wrapper::DataNode data) const {
	for (auto &node : data.Children()) {
		if (std::string(node.GetName()) != "stone") {
			continue;
		}
		try {
			const int stone_vnum = parse::ReadAsInt(node.GetValue("vnum"));
			(void) parse::ReadAsInt(node.GetValue("room_vnum"));
			if (node.GoToChild("skill")) {
				(void) parse::ReadAsConstant<ESkill>(node.GetValue("id"));
				(void) parse::ReadAsInt(node.GetValue("val"));
				node.GoToParent();
			}
			(void) stone_vnum;
		} catch (const std::exception &e) {
			err_log("rune_stones.xml validation failed on a <stone>: %s", e.what());
			return false;
		}
	}
	return true;
}

Runestone &RunestoneRoster::FindRunestone(RoomVnum vnum) {
	auto predicate = [vnum](const Runestone &p) { return p.GetRoomVnum() == vnum; };
	auto it = std::find_if(begin(), end(), predicate);
	if (it != end()) {
		return *it;
	}
	return incorrect_stone_;
}

Runestone &RunestoneRoster::FindRunestone(std::string_view name) {
	auto predicate = [name](const Runestone &p) { return p.GetName() == name; };
	auto it = std::find_if(begin(), end(), predicate);
	if (it != end()) {
		return *it;
	}
	return incorrect_stone_;
}

// Inspect (and, if eligible, memorise) the stone in ch's current room. Driven by the physical
// stone's object spec proc on look/examine. Always shows a runestone message (and returns true to
// suppress the prototype's generic description) -- even for a character without the skill, who gets
// the "lack skill" text. The normal/damaged and can-read/can't-read cases each have their own slot:
//   damaged + can read   -> kInspectDamaged       normal + can read + known   -> kInspectNormal {name}
//   damaged + can't read -> kLackSkillDamaged      normal + can read + unknown -> memorise
//   normal  + can't read -> kLackSkillNormal {skill_name}
bool RunestoneRoster::ViewRunestone(CharData *ch) {
	auto &stone = FindRunestone(GET_ROOM_VNUM(ch->in_room));
	if (!stone.IsAllowed()) {
		return false;
	}
	const bool can_read = ch->GetSkill(stone.GetSkill()) >= stone.GetSkillLevel();
	if (stone.IsDisabled()) {
		SendMsgToChar(RuneStoneMsg(can_read ? ERuneStoneMsg::kInspectDamaged
											: ERuneStoneMsg::kLackSkillDamaged) + "\r\n", ch);
	} else if (!can_read) {
		// {skill_name}: the skill needed to read this stone. The data model has one skill per stone,
		// so it is simply that skill's name (a multi-skill stone would pick one the reader has, else
		// the first).
		SendMsgToChar(fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kLackSkillNormal)),
				fmt::arg("skill_name", MUD::Skills()[stone.GetSkill()].GetName())) + "\r\n", ch);
	} else if (ch->IsRunestoneKnown(stone)) {
		SendMsgToChar(fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kInspectNormal)),
				fmt::arg("name", stone.GetName())) + "\r\n", ch);
	} else {
		ch->AddRunestone(stone);
	}
	return true;
}

namespace {
// The physical stone's object spec proc (assigned to the prototype vnum in SpawnStones). On a
// look/examine aimed at the stone it runs the inspect/memorise logic and suppresses the default
// examine; any other command, or a look not naming the stone, falls through untouched (return 0).
int RuneStoneObjSpec(CharData *ch, void *me, int cmd, char *argument) {
	if (!ch || ch->IsNpc()) {
		return 0;
	}
	if (!(CMD_IS("осмотреть") || CMD_IS("смотреть") || CMD_IS("examine") || CMD_IS("look"))) {
		return 0;
	}
	auto *obj = static_cast<ObjData *>(me);
	char target[kMaxInputLength];
	one_argument(argument, target);
	if (!*target || !isname(target, obj->get_aliases())) {
		return 0;
	}
	return MUD::Runestones().ViewRunestone(ch) ? 1 : 0;
}
}  // namespace

// issue.runestones phase 3: place one physical stone (the configured prototype) into each runestone
// room and give the prototype its spec proc, so the registry-driven look/examine hack in sight.cpp is
// retired. Idempotent: skips a room that already holds the stone, so it is safe to re-run on reload.
void RunestoneRoster::SpawnStones() {
	const auto proto_rnum = GetObjRnum(prototype_vnum_);
	if (proto_rnum < 0) {
		log("SYSERROR: rune_stones: prototype obj #%d not found -- no physical stones spawned.", prototype_vnum_);
		return;
	}
	obj_proto.func(proto_rnum, RuneStoneObjSpec);   // assign the spec proc to the prototype

	int placed = 0;
	for (const auto &stone : *this) {
		if (!stone.IsAllowed()) {
			continue;
		}
		const auto room_rnum = GetRoomRnum(stone.GetRoomVnum());
		if (room_rnum == kNowhere) {
			continue;
		}
		bool present = false;   // idempotent: don't stack a second stone on reload/reboot-in-place
		for (auto *o : world[room_rnum]->contents) {
			if (o->get_rnum() == proto_rnum) {
				present = true;
				break;
			}
		}
		if (present) {
			continue;
		}
		auto obj = MUD::world_objects().create_from_prototype_by_rnum(proto_rnum);
		if (!obj) {
			continue;
		}
		obj->set_description(RuneStoneMsg(stone.IsEnabled() ? ERuneStoneMsg::kRoomNormal
															: ERuneStoneMsg::kRoomDamaged));
		obj->set_extra_flag(EObjFlag::kNodecay);   // a permanent fixture, not loot
		PlaceObjToRoom(obj.get(), room_rnum);
		++placed;
	}
	log("Runestones: placed %d physical stone(s) (prototype #%d).", placed, prototype_vnum_);
}

std::vector<RoomVnum> RunestoneRoster::GetVnumRoster() {
	std::vector<RoomVnum> vnums;
	vnums.reserve(size());
	for (const auto &it : *this) {
		if (it.IsAllowed()) {
			vnums.push_back(it.GetRoomVnum());
		}
	}
	return vnums;
}

std::vector<std::string_view> RunestoneRoster::GetNameRoster() {
	std::vector<std::string_view> names;
	names.reserve(size());
	for (const auto &it : *this) {
		if (it.IsAllowed()) {
			names.push_back(it.GetName());
		}
	}
	return names;
}

void RuneStonesLoader::Load(parser_wrapper::DataNode data) { MUD::Runestones().Load(data); }
void RuneStonesLoader::Reload(parser_wrapper::DataNode data) { MUD::Runestones().Load(data); }

// ---- Vedun editable surface (vnum-keyed, mirrors guilds) -------------------------------------------

std::string RuneStonesLoader::EditableWhat() const { return "runestone"; }

std::vector<cfg_manager::EditableElement> RuneStonesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &stone : MUD::Runestones()) {
		if (!stone.IsAllowed()) {   // skip the kForbidden sentinel
			continue;
		}
		out.push_back({std::to_string(stone.GetVnum()),
				std::string(stone.GetId()) + " " + std::string(stone.GetName())});
	}
	return out;
}

cfg_manager::ValidationResult RuneStonesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::Runestones().Validate(doc)) {
		return {true, ""};
	}
	return {false, "Runestone data failed to parse (see syslog for the offending stone)."};
}

parser_wrapper::DataNode RuneStonesLoader::FindElementNode(parser_wrapper::DataNode root,
														  const std::string &id) const {
	// A <stone> carries no element `id` key; its identity is the integer `vnum`. Iterate ALL children
	// with a name check (not Children("stone")): a node copied out of a keyed range carries that
	// range's filter and would break its own later Children().
	for (auto &child : root.Children()) {
		if (std::string(child.GetName()) == "stone" && id == child.GetValue("vnum")) {
			return child;
		}
	}
	return parser_wrapper::DataNode{};
}

std::string RuneStonesLoader::CanonicalElementId(const std::string &id) const {
	// Only a non-negative integer is a valid (new) stone key.
	if (id.empty()) {
		return "";
	}
	for (const char c : id) {
		if (c < '0' || c > '9') {
			return "";
		}
	}
	return id;
}

parser_wrapper::DataNode RuneStonesLoader::CreateElementNode(parser_wrapper::DataNode root,
															const std::string &id) const {
	auto node = root.AddChild("stone");
	node.SetValue("vnum", id);
	node.SetValue("id", "kUndefined");
	node.SetValue("room_vnum", "0");
	auto skill = node.AddChild("skill");
	skill.SetValue("id", "kTownportal");
	skill.SetValue("val", "1");
	return node;
}

// ======================================== CHARACTER RUNESTONE ROSTER ==========================================

void CharacterRunestoneRoster::Serialize(std::ostringstream &out) {
	for (const auto it : *this) {
		out << fmt::format("Prtl: {}\n", it);
	}
}

void CharacterRunestoneRoster::PageToChar(CharData *ch) {
	std::ostringstream out;
	if (empty()) {
		out << " " << RuneStoneMsg(ERuneStoneMsg::kListEmpty) << "\r\n";
	} else {
		out << " " << RuneStoneMsg(ERuneStoneMsg::kListHeader) << "\r\n";
		table_wrapper::Table table;
		const int columns_num{4};
		int count = 1;
		for (const auto it : *this) {
			auto &portal = MUD::Runestones().FindRunestone(it);
			if (portal.IsAllowed()) {
				table << portal.GetName();
				if (count % columns_num == 0) {
					table << table_wrapper::kEndRow;
				}
				++count;
			}
		}
		table_wrapper::DecorateSimpleTable(ch, table);
		table_wrapper::PrintTableToStream(out, table);
		out << "\r\n " << fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kListCount)),
				fmt::arg("count", size())) << "\r\n";
	}
	out << " " << fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kListLimit)),
			fmt::arg("limit", CalcLimit(ch))) << "\r\n";

	page_string(ch->desc, out.str());
}

bool CharacterRunestoneRoster::RemoveRunestone(const Runestone &stone) {
	auto predicate = [stone](const RoomVnum p) { return p == stone.GetRoomVnum(); };
	return std::erase_if(*this, predicate);
}

bool CharacterRunestoneRoster::AddRunestone(const Runestone &stone) {
	if (stone.IsAllowed()) {
		const auto room_vnum = stone.GetRoomVnum();
		std::erase_if(*this, [room_vnum](RoomVnum p) { return p == room_vnum; });
		push_back(room_vnum);
		return true;
	}
	return false;
}

void CharacterRunestoneRoster::AddRunestone(CharData *ch, const Runestone &stone) {
	if (IsFull(ch)) {
		SendMsgToChar(RuneStoneMsg(ERuneStoneMsg::kMemoryFull) + "\r\n", ch);
		return;
	}
	if (AddRunestone(stone)) {
		SendMsgToChar(fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kMemorized)),
				fmt::arg("name", stone.GetName())) + "\r\n", ch);
	} else {
		SendMsgToChar(RuneStoneMsg(ERuneStoneMsg::kCantMemorize) + "\r\n", ch);
	}
	DeleteIrrelevant(ch);
}

void CharacterRunestoneRoster::RemoveRunestone(CharData *ch, const Runestone &stone) {
	if (RemoveRunestone(stone)) {
		SendMsgToChar(fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kForgotten)),
				fmt::arg("name", stone.GetName())) + "\r\n", ch);
	} else {
		SendMsgToChar(RuneStoneMsg(ERuneStoneMsg::kCantForget), ch);
	}
}

bool CharacterRunestoneRoster::IsFull(CharData *ch) {
	return CalcLimit(ch) <= Count();
}

bool CharacterRunestoneRoster::Contains(const Runestone &stone) {
	const auto it = std::find(begin(), end(), stone.GetRoomVnum());
	return (it != end());
}

void CharacterRunestoneRoster::DeleteIrrelevant(CharData *ch) {
	auto &runestones = MUD::Runestones();
	auto predicate =
		[&runestones](const RoomVnum vnum) { return runestones.FindRunestone(vnum).IsForbidden(); };
	std::erase_if(*this, predicate);
	if (IsOverfilled(ch)) {
		ShrinkToLimit(ch);
	}
}

bool CharacterRunestoneRoster::IsOverfilled(CharData *ch) {
	return (size() > CalcLimit(ch));
}

void CharacterRunestoneRoster::ShrinkToLimit(CharData *ch) {
	resize(CalcLimit(ch));
}

std::size_t CharacterRunestoneRoster::CalcLimit(CharData *ch) {
	const auto skill = ch->GetSkill(ESkill::kTownportal);
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (1 + low_skill/9 + hi_skill/5);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
