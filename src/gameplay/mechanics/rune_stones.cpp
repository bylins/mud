#include "rune_stones.h"
#include "rune_stones_loaders.h"

#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/core/handler.h"
#include "engine/ui/modify.h"
#include "engine/ui/table_wrapper.h"
#include "gameplay/abilities/abilities_constants.h"
#include "engine/structs/msg_container.h"
#include "engine/structs/info_container.h"
#include "utils/parse.h"
#include "utils/parser_wrapper.h"

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
		emplace_back(std::string(RuneStoneName(stone_vnum)), room_vnum, skill, skill_level, id);
	}
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

bool RunestoneRoster::ViewRunestone(CharData *ch, int where_bits) {
	if (ch->GetSkill(ESkill::kTownportal) == 0) {
		return false;
	}
	auto &stone = FindRunestone(GET_ROOM_VNUM(ch->in_room));
	if (stone.IsAllowed() && IS_SET(where_bits, EFind::kObjRoom)) {
		if (stone.IsDisabled()) {
			SendMsgToChar(RuneStoneMsg(ERuneStoneMsg::kInspectDamaged) + "\r\n", ch);
		} else if (ch->IsRunestoneKnown(stone)) {
			SendMsgToChar(fmt::format(fmt::runtime(RuneStoneMsg(ERuneStoneMsg::kInspectNormal)),
					fmt::arg("name", stone.GetName())) + "\r\n", ch);
		} else if (ch->GetSkill(stone.GetSkill()) < stone.GetSkillLevel()) {
			SendMsgToChar(RuneStoneMsg(ERuneStoneMsg::kLackSkillNormal) + "\r\n", ch);
		} else {
			ch->AddRunestone(stone);
		}
		return true;
	}
	return false;
}

void RunestoneRoster::ShowRunestone(CharData *ch) {
	if (ch->GetSkill(ESkill::kTownportal)) {
		const auto &stone = FindRunestone(GET_ROOM_VNUM(ch->in_room));
		if (stone.IsAllowed()) {
			SendMsgToChar(RuneStoneMsg(stone.IsEnabled() ? ERuneStoneMsg::kRoomNormal
														 : ERuneStoneMsg::kRoomDamaged) + "\r\n", ch);
		}
	}
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
