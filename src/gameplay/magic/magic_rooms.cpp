#include "magic_rooms.h"
#include "utils/utils_string.h"   // str_cmp (Vedun CanonicalElementId)
#include "administration/privilege.h"
#include "gameplay/handlers/spell_handlers.h"

#include "spells_info.h"
#include "engine/ui/modify.h"
#include "engine/entities/char_data.h"
#include "magic.h" //Включено ради material_component_processing
#include "magic_utils.h" // IsRoomBlocked / MayCastInForbiddenRoom
#include "magic_internal.h" // BuildActionContext / CastUnaffects / ProcessMatComponents
#include "engine/ui/table_wrapper.h"
#include "engine/db/global_objects.h"
#include "gameplay/skills/townportal.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "engine/db/player_index.h"
#include "engine/db/db.h"           // chardata_by_uid
#include "gameplay/fight/pk.h"          // pk_agro_action
#include "room_affects_loader.h"   // RoomAffectsLoader
#include "gameplay/affects/affect_contants.h"   // EAffFlag
#include "utils/utils_parse.h"   // parse::ReadAsConstantsBitvector
#include <array>
#include "room_affect_messages.h"   // RoomAffectMessagesLoader
#include "engine/structs/msg_container.h"
#include <set>

// Структуры и функции для работы с заклинаниями, обкастовывающими комнаты

// --- issue.affect-migration: room-affect registry name maps (cfg/room_affects.xml) -----------------
// Mirrors the EAffect name maps in affect_contants.cpp. kUndefined (sentinel) and kCount (terminal)
// are excluded -- only real room affects get a row.
namespace {
std::map<room_spells::ERoomAffect, std::string> g_room_affect_name_by_value;
std::map<std::string, room_spells::ERoomAffect> g_room_affect_value_by_name;
void init_room_affect_names() {
	using RA = room_spells::ERoomAffect;
	g_room_affect_name_by_value = {
		{RA::kNoPortalExit, "kNoPortalExit"},
		{RA::kForbidden, "kForbidden"},
		{RA::kMeteorStorm, "kMeteorStorm"},
		{RA::kRoomLight, "kRoomLight"},
		{RA::kDeadlyFog, "kDeadlyFog"},
		{RA::kThunderstorm, "kThunderstorm"},
		{RA::kRuneLabel, "kRuneLabel"},
		{RA::kHypnoticPattern, "kHypnoticPattern"},
		{RA::kBlackTentacles, "kBlackTentacles"},
		{RA::kPortalTimer, "kPortalTimer"},
		{RA::kFireTrap, "kFireTrap"},
	};
	for (const auto &[value, token] : g_room_affect_name_by_value) {
		g_room_affect_value_by_name.emplace(token, value);
	}
}
}  // namespace

template<>
const std::string &NAME_BY_ITEM<room_spells::ERoomAffect>(room_spells::ERoomAffect item) {
	if (g_room_affect_name_by_value.empty()) { init_room_affect_names(); }
	return g_room_affect_name_by_value.at(item);
}
template<>
room_spells::ERoomAffect ITEM_BY_NAME<room_spells::ERoomAffect>(const std::string &name) {
	if (g_room_affect_value_by_name.empty()) { init_room_affect_names(); }
	return g_room_affect_value_by_name.at(name);
}
template<>
const std::map<room_spells::ERoomAffect, std::string> &NAMES_OF<room_spells::ERoomAffect>() {
	if (g_room_affect_name_by_value.empty()) { init_room_affect_names(); }
	return g_room_affect_name_by_value;
}

// --- ERoomAffectMsgType name map (mirrors kAffectMsgTypeNames) ------------------------------------
namespace {
const std::map<room_spells::ERoomAffectMsgType, std::string> kRoomAffectMsgTypeNames{
		{room_spells::ERoomAffectMsgType::kUndefined, "kUndefined"},
		{room_spells::ERoomAffectMsgType::kShortDesc, "kShortDesc"},
		{room_spells::ERoomAffectMsgType::kAffImposedToChar, "kAffImposedToChar"},
		{room_spells::ERoomAffectMsgType::kAffImposedToRoom, "kAffImposedToRoom"},
		{room_spells::ERoomAffectMsgType::kAffDispelledToChar, "kAffDispelledToChar"},
		{room_spells::ERoomAffectMsgType::kAffDispelledToRoom, "kAffDispelledToRoom"},
		{room_spells::ERoomAffectMsgType::kAffExpiredToChar, "kAffExpiredToChar"},
		{room_spells::ERoomAffectMsgType::kAffExpiredToRoom, "kAffExpiredToRoom"},
		{room_spells::ERoomAffectMsgType::kAffInterruptedToChar, "kAffInterruptedToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnEntryToChar, "kTriggerOnEntryToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnEntryToRoom, "kTriggerOnEntryToRoom"},
		{room_spells::ERoomAffectMsgType::kTriggerOnOpenToChar, "kTriggerOnOpenToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnOpenToRoom, "kTriggerOnOpenToRoom"},
		{room_spells::ERoomAffectMsgType::kTriggerOnPickToChar, "kTriggerOnPickToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnPickToRoom, "kTriggerOnPickToRoom"},
		{room_spells::ERoomAffectMsgType::kTriggerOnUnlockToChar, "kTriggerOnUnlockToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnUnlockToRoom, "kTriggerOnUnlockToRoom"},
		{room_spells::ERoomAffectMsgType::kTriggerOnCloseToChar, "kTriggerOnCloseToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnCloseToRoom, "kTriggerOnCloseToRoom"},
		{room_spells::ERoomAffectMsgType::kTriggerOnLockToChar, "kTriggerOnLockToChar"},
		{room_spells::ERoomAffectMsgType::kTriggerOnLockToRoom, "kTriggerOnLockToRoom"},
		{room_spells::ERoomAffectMsgType::kTriggerNoEffect, "kTriggerNoEffect"},
		{room_spells::ERoomAffectMsgType::kTickMsgOne, "kTickMsgOne"},
		{room_spells::ERoomAffectMsgType::kTickMsgTwo, "kTickMsgTwo"},
		{room_spells::ERoomAffectMsgType::kTickMsgThree, "kTickMsgThree"},
		{room_spells::ERoomAffectMsgType::kTickMsgFour, "kTickMsgFour"},
		{room_spells::ERoomAffectMsgType::kTickMsgFive, "kTickMsgFive"},
		{room_spells::ERoomAffectMsgType::kTickMsgSix, "kTickMsgSix"},
		{room_spells::ERoomAffectMsgType::kTickMsgSeven, "kTickMsgSeven"},
		{room_spells::ERoomAffectMsgType::kTickMsgEight, "kTickMsgEight"},
		{room_spells::ERoomAffectMsgType::kRoomAffectVisible, "kRoomAffectVisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectInvisible, "kRoomAffectInvisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectSelfInvisible, "kRoomAffectSelfInvisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectPkVisible, "kRoomAffectPkVisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectPkInvisible, "kRoomAffectPkInvisible"},
		{room_spells::ERoomAffectMsgType::kDamageToChar, "kDamageToChar"},
		{room_spells::ERoomAffectMsgType::kDamageToVict, "kDamageToVict"},
		{room_spells::ERoomAffectMsgType::kDamageToRoom, "kDamageToRoom"},
	};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<room_spells::ERoomAffectMsgType>(room_spells::ERoomAffectMsgType item) {
	return kRoomAffectMsgTypeNames.at(item);
}
template<>
const std::map<room_spells::ERoomAffectMsgType, std::string> &NAMES_OF<room_spells::ERoomAffectMsgType>() {
	return kRoomAffectMsgTypeNames;
}
template<>
room_spells::ERoomAffectMsgType ITEM_BY_NAME<room_spells::ERoomAffectMsgType>(const std::string &name) {
	static std::map<std::string, room_spells::ERoomAffectMsgType> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kRoomAffectMsgTypeNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace room_spells {

namespace {
// room_affects.xml is the room-affect registry (id-only for now; grows per-affect data later).
// issue.affects-improve: a room affect is identified solely by its own ERoomAffect id. There is no
// spell<->affect name mapping; the imposing spell declares its affect via <affects id=...> and that
// id is what flows through impose/find/remove. Nothing maps an affect back to a spell.

// issue.affect-migration: per-ERoomAffect behavior flags from room_affects.xml (mirrors the
// char-affect g_affect_flags). Source of truth for room-affect cure/dispel/refresh/decrement/...;
// the casting source only sets strength + duration. Indexed by to_underlying(ERoomAffect)
// (1-based; index 0 = kUndefined = no flags).
constexpr std::size_t kRoomAffectFlagTableSize = to_underlying(ERoomAffect::kCount);
std::array<Bitvector, kRoomAffectFlagTableSize> g_room_affect_flags{};
// issue.affect-migration: an affect's own <actions> (same format as a spell's <talent_actions>). Each
// <action> carries its own <trigger> (EActionTrigger) deciding when it fires; the room affect performs
// the matching actions directly each tick -- no dedicated tick_spell / tick_handler. A side_spell action
// replaces the old tick_spell; a manual_cast action replaces the old tick_handler.
std::array<talents_actions::Actions, kRoomAffectFlagTableSize> g_room_affect_actions{};
// issue.affects-improve (Phase B): per-affect seal-strength cap (the modifier ceiling the
// star rating scales against), affect-native so the display never needs the imposing spell.
std::array<int, kRoomAffectFlagTableSize> g_room_affect_cap{};
// issue.affects-improve (P1 fix): per-affect seal-strength FORMULA. The room affect's strength
// (af.modifier -- the seal level that gates the ward in CallMagicToRoom and the star rating in
// sight.cpp) is the affect's OWN property now, not the casting spell's apply (which is a bare
// <affect id=> ref carrying no coefficients). g_room_affect_has_seal[idx] marks the rows that
// carry a <seal_strength> formula; the rest compute no modifier (flag-only wards like kRuneLabel).
std::array<talents_actions::TalentAffect::Apply, kRoomAffectFlagTableSize> g_room_affect_seal{};
std::array<bool, kRoomAffectFlagTableSize> g_room_affect_has_seal{};
bool g_room_affect_flags_loaded = false;

// issue.affect-migration: is a fight underway in this room? (gates kBattlePulse-triggered actions.)
bool RoomHasCombat(RoomData *room) {
	for (const auto tch : room->people) {
		if (tch->GetEnemy()) {
			return true;
		}
	}
	return false;
}

// issue.affect-migration: does this action fire on the current room-affect pulse? kPulse fires every
// pulse; kBattlePulse fires only while combat is underway in the room.
bool ActionFiresOnPulse(const talents_actions::Action &action, bool combat) {
	const auto &t = action.GetTrigger();
	return t.test(talents_actions::EActionTrigger::kPulse)
		|| (t.test(talents_actions::EActionTrigger::kBattlePulse) && combat);
}

void BuildRoomAffectFlagTable(parser_wrapper::DataNode data) {
	g_room_affect_flags.fill(0);
	g_room_affect_cap.fill(0);
	g_room_affect_has_seal.fill(false);
	for (auto &s : g_room_affect_seal) { s = talents_actions::TalentAffect::Apply{}; }
	for (auto &a : g_room_affect_actions) { a = talents_actions::Actions{}; }
	for (auto &node : data.Children("room_affect")) {
		const char *id = node.GetValue("id");
		if (!id || !*id) {
			continue;
		}
		ERoomAffect affect;
		try {
			affect = ITEM_BY_NAME<ERoomAffect>(id);
		} catch (const std::out_of_range &) {
			continue;  // unknown id already reported by the validator
		}
		const auto idx = static_cast<std::size_t>(to_underlying(affect));
		if (idx >= kRoomAffectFlagTableSize) {
			continue;
		}
		// issue.affect-migration: effect flags + actions live in child tags (unified with spells.xml),
		// read off a COPY of the iteration node (the obj_sets pattern). Each <action> carries its own
		// <trigger>, parsed by Actions::Build/ParseAction.
		{
			auto fnode = node;
			if (fnode.GoToChild("flags")) {
				if (const char *flags = fnode.GetValue("val"); flags && *flags) {
					g_room_affect_flags[idx] = parse::ReadAsConstantsBitvector<EAffFlag>(flags);
				}
			}
		}
		{
			auto act_node = node;
			if (act_node.GoToChild("actions")) {
				g_room_affect_actions[idx].Build(act_node);
			}
		}
		{
			auto cnode = node;
			if (cnode.GoToChild("seal_strength")) {
				// issue.affects-improve (P1 fix): the seal strength is a modifier FORMULA (the same
				// min/dices_weight/alpha/beta/factor/cap set spells.xml applies use), evaluated per cast
				// via ComputeApplyModifier. The bare attribute = the legacy cap-only form (formula stays
				// at defaults: min=0 beta=0 -> modifier 0). kForbidden carries beta=1 cap=100 = MIN(100, C).
				auto &seal = g_room_affect_seal[idx];
				const auto dbl = [&](const char *attr, double def) {
					const char *v = cnode.GetValue(attr);
					return (v && *v) ? parse::ReadAsDouble(v) : def;
				};
				seal.min = dbl("min", 0.0);
				seal.beta = dbl("beta", 0.0);
				seal.weight = dbl("weight", 0.0);
				if (const char *f = cnode.GetValue("factor"); f && *f) { seal.factor = parse::ReadAsInt(f); }
				if (const char *cap = cnode.GetValue("cap"); cap && *cap) {
					seal.cap = parse::ReadAsInt(cap);
					g_room_affect_cap[idx] = seal.cap;  // kept for sight.cpp's star-rating denominator
				}
				g_room_affect_has_seal[idx] = true;
			}
		}
	}
	g_room_affect_flags_loaded = true;
}

void ValidateRoomAffectRegistry(parser_wrapper::DataNode data) {
	std::set<std::string> seen;
	for (auto &node : data.Children("room_affect")) {
		const char *id = node.GetValue("id");
		if (!id || !*id) {
			log("SYSERROR: room_affects.xml: <room_affect> without an id -- skipped.");
			continue;
		}
		try {
			(void) ITEM_BY_NAME<ERoomAffect>(id);
		} catch (const std::out_of_range &) {
			log("SYSERROR: room_affects.xml: unknown room-affect id '%s' -- skipped.", id);
			continue;
		}
		seen.insert(id);
	}
	for (const auto &[affect, token] : NAMES_OF<ERoomAffect>()) {
		if (seen.find(token) == seen.end()) {
			log("SYSERROR: room_affects.xml: missing <room_affect id=\"%s\">.", token.c_str());
		}
	}
}
}  // namespace

// issue.affect-migration: a room affect's intrinsic behavior flags from room_affects.xml, keyed by
// affect_type. 0 for room affects with no row/flags. Loaded? guards apply-time sourcing (Phase R2).
Bitvector RoomAffectFlagsByType(ERoomAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_flags[idx] : Bitvector{0};
}
bool RoomAffectFlagsLoaded() { return g_room_affect_flags_loaded; }

int RoomAffectSealCap(ERoomAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_cap[idx] : 0;
}

bool RoomAffectHasSeal(ERoomAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize && g_room_affect_has_seal[idx];
}

const talents_actions::TalentAffect::Apply &RoomAffectSeal(ERoomAffect affect_type) {
	static const talents_actions::TalentAffect::Apply kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_seal[idx] : kEmpty;
}

const talents_actions::Actions &RoomAffectActions(ERoomAffect affect_type) {
	static const talents_actions::Actions kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_actions[idx] : kEmpty;
}

void RoomAffectsLoader::Load(parser_wrapper::DataNode data) { ValidateRoomAffectRegistry(data); BuildRoomAffectFlagTable(data); }
void RoomAffectsLoader::Reload(parser_wrapper::DataNode data) { ValidateRoomAffectRegistry(data); BuildRoomAffectFlagTable(data); }

// issue.vedun-editor: in-game editing of cfg/room_affects.xml.
std::string RoomAffectsLoader::EditableWhat() const { return "room_affects"; }

std::vector<cfg_manager::EditableElement> RoomAffectsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &[ra, token] : NAMES_OF<ERoomAffect>()) {
		if (ra == ERoomAffect::kUndefined) {
			continue;
		}
		const std::string &label = RoomAffectMsgRaw(ra, ERoomAffectMsgType::kShortDesc);
		out.push_back({token, label.empty() ? token : label});
	}
	return out;
}

// Dry-run on save: the .scheme already constrains the editor's pick-lists (ERoomAffect / EAffFlag /
// the action enums); this re-checks the room-affect identity + flag set against the enum maps so a
// bad value never reaches the file. Deeper action-tree values are typed by the scheme + caught by the
// action parser on reload.
cfg_manager::ValidationResult RoomAffectsLoader::Validate(parser_wrapper::DataNode &doc) const {
	for (auto &ra : doc.Children("room_affect")) {
		const char *id = ra.GetValue("id");
		if (!id || !*id) {
			return {false, "<room_affect> without an id."};
		}
		try {
			(void) ITEM_BY_NAME<ERoomAffect>(id);
		} catch (const std::out_of_range &) {
			return {false, std::string("unknown room affect id '") + id + "'."};
		}
		if (auto fnode = ra; fnode.GoToChild("flags")) {
			if (const char *fv = fnode.GetValue("val"); fv && *fv) {
				try {
					(void) parse::ReadAsConstantsBitvector<EAffFlag>(fv);
				} catch (const std::exception &) {
					return {false, std::string("room affect '") + id + "': unknown flag in '" + fv + "'."};
				}
			}
		}
	}
	return {true, ""};
}

// --- room-affect message container (cfg/messages/ru/room_affect_msg.xml) -------------------------
namespace {
msg_container::MsgContainer<ERoomAffect, ERoomAffectMsgType> &RoomAffectMsgContainer() {
	static msg_container::MsgContainer<ERoomAffect, ERoomAffectMsgType> container;
	return container;
}
}  // namespace

const std::string &RoomAffectMsg(ERoomAffect affect, ERoomAffectMsgType slot) {
	return RoomAffectMsgContainer().GetMessage(affect, slot);
}
const std::string &RoomAffectMsgRaw(ERoomAffect affect, ERoomAffectMsgType slot) {
	static const std::string kEmpty;
	auto &c = RoomAffectMsgContainer();
	return c.IsKnown(affect) ? c[affect].GetMessage(slot) : kEmpty;
}
void RoomAffectMessagesLoader::Load(parser_wrapper::DataNode data) {
	RoomAffectMsgContainer().Init(data.Children());
}
void RoomAffectMessagesLoader::Reload(parser_wrapper::DataNode data) {
	RoomAffectMsgContainer().Reload(data.Children());
}

// issue.unstable-hotfixes: Vedun in-game editing of room-affect messages (`vedun roomaffectmsg`).
// Mirrors AffectMessagesLoader -- keyed by the <msg_sheaf id=> (ERoomAffect token; shared = "kDefault").
std::string RoomAffectMessagesLoader::EditableWhat() const {
	return "roomaffectmsg";
}

std::vector<cfg_manager::EditableElement> RoomAffectMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : RoomAffectMsgContainer()) {
		const ERoomAffect id = sheaf.GetId();
		const std::string id_str = (id == ERoomAffect::kUndefined) ? "kDefault" : NAME_BY_ITEM<ERoomAffect>(id);
		out.push_back({id_str, sheaf.GetMessage(ERoomAffectMsgType::kShortDesc)});   // label = short desc
	}
	return out;
}

cfg_manager::ValidationResult RoomAffectMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (RoomAffectMsgContainer().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Room-affect message data failed to parse (see syslog for the offending sheaf/message)."};
}

std::string RoomAffectMessagesLoader::CanonicalElementId(const std::string &id) const {
	for (const auto &[value, name] : NAMES_OF<ERoomAffect>()) {
		if (value != ERoomAffect::kUndefined && str_cmp(name, id) == 0) {
			return name;
		}
	}
	return "";
}

parser_wrapper::DataNode RoomAffectMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

const int kRuneLabelDuration = 300;

std::list<RoomData *> affected_rooms;

// issue.room-affect-trigger-improve (door affects): registry of enchanted exits so the affect pulse
// can tick/expire door affects without scanning every room's six exits. Keyed by (room, dir) -- the
// cast-side host. File-local: only this TU's tick + impose touch it.
namespace { struct ExitAffectHost { RoomData *room; int dir; }; }
std::list<ExitAffectHost> affected_exits;

void AddExitToAffected(RoomData *room, int dir) {
	for (const auto &e : affected_exits) {
		if (e.room == room && e.dir == dir) { return; }
	}
	affected_exits.push_back({room, dir});
}

// issue.room-affect-trigger-improve (door affects): clear affects on the exit `dir` of `room` and drop
// its enchanted-exit registry entry. Called when an exit is rebuilt/torn down (redit save, dungeon
// reuse, DGScript wexit purge) so a stale affect or registry row never outlives its ExitData. The tick
// also self-heals empty/missing exits; this just makes it proactive (and explicit "the door changed").
void ClearExitAffects(RoomData *room, int dir) {
	if (room && dir >= 0 && dir < EDirection::kMaxDirNum) {
		if (const auto ex = room->dir_option[dir]) {
			ex->affected.clear();
		}
	}
	affected_exits.remove_if([&](const ExitAffectHost &e) { return e.room == room && e.dir == dir; });
}

void RemoveSingleRoomAffect(long caster_id, ERoomAffect want);
void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff);
void SendRemoveAffectMsgToRoom(ERoomAffect room_affect, RoomRnum room);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);

namespace {
// Look up `key` in `spell`'s sheaf (with kDefault fallback) and send to `ch`,
// substituting any {name} placeholder with `spell`'s canonical name. Trailing
// "\r\n" matches the SendMsgToChar convention.
void SendSpellNameMsg(CharData *ch, ESpell spell, ESpellMsg key) {
	std::string msg = MUD::SpellMessages().GetMessage(spell, key);
	const auto pos = msg.find("{name}");
	if (pos != std::string::npos) {
		msg.replace(pos, std::strlen("{name}"), MUD::Spell(spell).GetCName());
	}
	SendMsgToChar(msg + "\r\n", ch);
}
}  // namespace

void RoomRemoveAffect(RoomData *room, const RoomAffectIt &affect) {
	if (room->affected.empty()) {
		log("ERROR: Attempt to remove affect from no affected room!");
		return;
	}
	room->affected.erase(affect);
}

RoomAffectIt FindAffect(RoomData *room, ERoomAffect want) {
	for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i) {
		const auto affect = *affect_i;
		if (affect->affect_type == want) {
			return affect_i;
		}
	}
	return room->affected.end();
}

// issue.affect-migration: the two portal room affects -- two-way kPortalTimer and one-way kNoPortalExit
// -- share one identity space (both were the now-retired kPortalTimer spell). These query by the
// ERoomAffect identity instead of the legacy Affect::type (ESpell).
bool IsPortalAffect(ERoomAffect affect_type) {
	return affect_type == ERoomAffect::kPortalTimer || affect_type == ERoomAffect::kNoPortalExit;
}
bool RoomHasPortal(RoomData *room) {
	for (const auto &af : room->affected) {
		if (IsPortalAffect(af->affect_type)) {
			return true;
		}
	}
	return false;
}
RoomAffectIt FindPortalAffect(RoomData *room) {
	for (auto it = room->affected.begin(); it != room->affected.end(); ++it) {
		if (IsPortalAffect((*it)->affect_type)) {
			return it;
		}
	}
	return room->affected.end();
}

bool IsZoneRoomAffected(int zone_vnum, ERoomAffect affect) {
	for (auto & affected_room : affected_rooms) {
		if (affected_room->zone_rn == zone_vnum && IsRoomAffected(affected_room, affect)) {
			return true;
		}
	}
	return false;
}

bool IsRoomAffected(RoomData *room, ERoomAffect want) {
	for (const auto &af : room->affected) {
		if (af->affect_type == want) {
			return true;
		}
	}
	return false;
}

long FindRoomPkPortalUid(RoomData *room, long exclude_uid) {
	if (!room) {
		return 0;
	}
	for (const auto &af : room->affected) {
		if (IsPortalAffect(af->affect_type)
				&& af->pk_unique != 0
				&& af->pk_unique != exclude_uid) {
			return af->pk_unique;
		}
	}
	return 0;
}

// issue.room-affect-trigger-improve: one-decimal potency for the affect tables.
static std::string FmtPotency(float p) {
	char b[16];
	snprintf(b, sizeof(b), "%.1f", p);
	return b;
}

void ShowAffectedRooms(CharData *ch) {
	std::stringstream out;
	out << " Rooms with active affects:" << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader <<
		"#" << "Vnum" << "affect" << "Caster name" << "Time (s)" << "Potency" << table_wrapper::kEndRow;
	int count = 1;
	for (const auto r : affected_rooms) {
		for (const auto &af : r->affected) {
			table << count << r->vnum
				  // issue.affect-migration: room-affect name by its ERoomAffect identity.
				  << NAME_BY_ITEM<ERoomAffect>(af->affect_type)
				  << GetNameById(af->caster_id) << af->duration * 2
				  << FmtPotency(af->potency) << table_wrapper::kEndRow;
			++count;
		}
	}
	table_wrapper::DecorateServiceTable(ch, table);
	out << table.to_string() << "\r\n";

	// issue.room-affect-trigger-improve (door affects): exits carrying active affects.
	out << " Exits with active affects:" << "\r\n";
	table_wrapper::Table etable;
	etable << table_wrapper::kHeader <<
		"#" << "From" << "To" << "affect" << "Caster name" << "Time (s)" << "Potency" << "Charges"
		<< table_wrapper::kEndRow;
	int ecount = 1;
	for (const auto &e : affected_exits) {
		const auto ex = (e.room && e.dir >= 0 && e.dir < EDirection::kMaxDirNum) ? e.room->dir_option[e.dir] : nullptr;
		if (!ex) { continue; }
		const int to_vnum = (ex->to_room() != kNowhere) ? world[ex->to_room()]->vnum : -1;
		for (const auto &af : ex->affected) {
			etable << ecount << e.room->vnum << to_vnum
				   << NAME_BY_ITEM<ERoomAffect>(af->affect_type)
				   << GetNameById(af->caster_id)
				   << (af->duration == -1 ? std::string("перм") : std::to_string(af->duration * 2))
				   << FmtPotency(af->potency)
				   << (af->charges == -1 ? std::string("беск") : std::to_string(af->charges))
				   << table_wrapper::kEndRow;
			++ecount;
		}
	}
	table_wrapper::DecorateServiceTable(ch, etable);
	out << etable.to_string() << "\r\n";

	page_string(ch->desc, out.str());
}

CharData *find_char_in_room(long char_id, RoomData *room) {
	assert(room);
	for (const auto tch : room->people) {
		if (tch->get_uid() == char_id) {
			return (tch);
		}
	}
	return nullptr;
}

RoomData *FindAffectedRoomByCasterID(long caster_id, ERoomAffect want) {
	for (const auto room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->affect_type == want && af->caster_id == caster_id) {
				return room;
			}
		}
	}
	return nullptr;
}

template<typename F>
ERoomAffect RemoveAffectFromRooms(const F &filter) {
	for (const auto room : affected_rooms) {
		const auto &affect = std::find_if(room->affected.begin(), room->affected.end(), filter);
		if (affect != room->affected.end()) {
			const ERoomAffect removed = (*affect)->affect_type;
			SendRemoveAffectMsgToRoom(removed, GetRoomRnum(room->vnum));
			RoomRemoveAffect(room, affect);
			return removed;
		}
	}
	return ERoomAffect::kUndefined;
}

void RemoveSingleRoomAffect(long caster_id, ERoomAffect want) {
	auto filter =
		[&caster_id, want](auto &af) { return (af->caster_id == caster_id && af->affect_type == want); };
	// RemoveAffectFromRooms ignores its first arg (it derives the message from the affect itself).
	RemoveAffectFromRooms(filter);
}

// issue.affects-improve: find the caster's controlled room affect (kAfNeedControl) WITHOUT removing
// it, so the "you interrupted" notice can be shown before RemoveControlledRoomAffect emits the
// affect's expiry line -- giving the natural order (you interrupt, then the effect ceases).
ERoomAffect FindControlledRoomAffect(CharData *ch) {
	const long uid = ch->get_uid();
	for (const auto room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->caster_id == uid && IS_SET(af->battleflag, kAfNeedControl)) {
				return af->affect_type;
			}
		}
	}
	return ERoomAffect::kUndefined;
}

ERoomAffect RemoveControlledRoomAffect(CharData *ch) {
	long casterID = ch->get_uid();
	// issue.affect-migration: "controlled" is the kAfNeedControl flag on the room affect (set in
	// room_affects.xml -- the room-affect counterpart of the spell's EMagic kMagNeedControl). General
	// flag check, not a specific affect.
	auto filter =
		[&casterID](auto &af) {
			return (af->caster_id == casterID && IS_SET(af->battleflag, kAfNeedControl));
		};
	return RemoveAffectFromRooms(filter);
}

void SendRemoveAffectMsgToRoom(ERoomAffect room_affect, RoomRnum room) {
	// issue.affects-improve: expiry/removal narration from the room-affect message system, keyed by
	// the affect itself -- kAffExpiredToRoom is defined for every room affect. No spell lookup.
	const std::string &room_msg = RoomAffectMsgRaw(room_affect, ERoomAffectMsgType::kAffExpiredToRoom);
	if (!room_msg.empty()) {
		SendMsgToRoom(room_msg.c_str(), room, 0);
	}
}

void AddRoomToAffected(RoomData *room) {
	const auto it = std::find(affected_rooms.begin(), affected_rooms.end(), room);
	if (it == affected_rooms.end())
		affected_rooms.push_back(room);
}

// Per-tick handler for kDeadlyFog room affect. Each duration step triggers a
// progressively more harmful cascade against everyone in the room.
// Per-tick handler for kThunderstorm room affect. Each duration step triggers a
// different elemental cascade until the storm fades.
// Emit a kThunderstorm per-tick line from the kThunderstorm sheaf (kCustomMsgOne..Eight, one per
// duration phase) to the whole room. Replaces the hardcoded SendMsgToChar/act pairs.


// Per-tick room narration: cycle the impose spell's defined kCustomMsg slots by the affect's
// tick counter (apply_time), so a multi-phase room effect can show a different line each round.
// issue.affects-improve (Phase B): per-pulse flavor line for a room affect, rotated by tick count.
// Affect-native -- reads the affect's own kTickMsg* sheaf, no imposing spell needed.
static void EmitRoomTickMessage(CharData *ch, ERoomAffect affect, int tick) {
	static const ERoomAffectMsgType slots[] = {
		ERoomAffectMsgType::kTickMsgOne, ERoomAffectMsgType::kTickMsgTwo,
		ERoomAffectMsgType::kTickMsgThree, ERoomAffectMsgType::kTickMsgFour,
		ERoomAffectMsgType::kTickMsgFive, ERoomAffectMsgType::kTickMsgSix,
		ERoomAffectMsgType::kTickMsgSeven, ERoomAffectMsgType::kTickMsgEight};
	std::vector<const std::string *> present;
	for (const auto k : slots) {
		const std::string &m = RoomAffectMsgRaw(affect, k);
		if (!m.empty()) present.push_back(&m);
	}
	if (present.empty()) return;
	act(present[tick % present.size()]->c_str(), false, ch, nullptr, nullptr, kToRoom | kToChar | kToArenaListen);
}

// issue.affect-migration: per-tick room-affect handler. The affect owns its work as <actions>, each
// gated by its own <trigger> (a side_spell action replaces the old tick_spell; a manual_cast action
// replaces the old tick_handler). Collect the actions whose trigger fires on this pulse (kPulse always;
// kBattlePulse only in combat) and run ONE per tick, cycled by the affect's tick counter. Returns false
// if the affect has no pulse-triggered action (a passive affect -- nothing to do this tick).
static bool RunRoomTick(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	const ERoomAffect affect_type = aff->affect_type;
	const bool combat = RoomHasCombat(room);
	std::vector<talents_actions::Action> pulse;
	for (const auto &action : RoomAffectActions(affect_type).list()) {
		if (ActionFiresOnPulse(action, combat)) {
			pulse.push_back(action);
		}
	}
	if (pulse.empty()) {
		return false;
	}
	if (ch == nullptr) {
		return true;  // triggered affect, but no caster to source the cast this tick
	}
	// apply_time is incremented before the handler runs, so the first tick is apply_time==1; phase
	// counts from 0 so action[phase % N] / kCustomMsg slot [phase % K] start at the first.
	const int phase = aff->apply_time > 0 ? aff->apply_time - 1 : 0;
	EmitRoomTickMessage(ch, affect_type, phase);
	// Thread the affect's current duration in/out so a manual_cast handler (e.g. thunderstorm) can
	// branch on it -- and end the effect early by zeroing it.
	int dur = aff->duration;
	// issue.affects-improve (Phase B): the tick cast runs spell-free on the affect's stored
	// potency (fixed_potency); ctx_spell is kUndefined -- side_spell actions cast their own spells.
	CastRoomTickActionFromActions(ch, room, ESpell::kUndefined, pulse, phase, &dur, aff->potency);
	aff->duration = dur;
	return true;
}

// issue.character-affect-triggers: run a room/exit affect's actions for a LIFECYCLE trigger
// (kExpired / kDispell) -- the room-side analog of RunCharAffectTrigger. Mirrors RunRoomTick but
// filters by an explicit trigger instead of the pulse/battle-pulse set. `ch` sources the cast: for
// kExpired it is the affect's caster (find_char(caster_id)); for kDispell it is the dispeller, so a
// kTarFightSelf effect lands on them as self-inflicted damage (no PvP against anyone). Recursion is
// bounded by the shared depth guard inside RunRoomCycledAction.
bool RunRoomAffectTrigger(RoomData *room, CharData *ch, ERoomAffect affect_type,
						  talents_actions::EActionTrigger trig, float potency) {
	if (!room || ch == nullptr) {
		return false;   // room affects have no bearer -- without a caster there is nothing to source the cast
	}
	std::vector<talents_actions::Action> fired;
	for (const auto &action : RoomAffectActions(affect_type).list()) {
		if (action.GetTrigger().test(trig)) {
			fired.push_back(action);
		}
	}
	if (fired.empty()) {
		return false;
	}
	// issue.character-affect-triggers: hand the affect's own damage flavor to any <damage> in the chain
	// (a room/exit trap or a kDispell sting), so it replaces the generic combat line -- the room-side
	// analog of RunCharAffectTrigger's SetAffectDamageMsg. Empty slots keep the generic message.
	CastRoomTickActionFromActions(ch, room, ESpell::kUndefined, fired, 0, nullptr, potency,
			RoomAffectMsgRaw(affect_type, ERoomAffectMsgType::kDamageToChar),
			RoomAffectMsgRaw(affect_type, ERoomAffectMsgType::kDamageToVict),
			RoomAffectMsgRaw(affect_type, ERoomAffectMsgType::kDamageToRoom));
	return true;
}

void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	assert(aff);
	assert(room);
	// issue.affect-migration: run the affect's pulse-triggered <actions> (if any). A passive affect
	// with no pulse-triggered action simply does nothing this tick.
	RunRoomTick(room, ch, aff);
}

// issue.room-affect-trigger-improve (door affects): tick the timer on enchanted exits. Mirrors the
// room loop: duration>=1 decrements, duration==-1 is PERMANENT (stays until removed -- a trap that
// "remains in place"), 0/expired removes the affect (and announces via the host room's sheaf). Walks
// the affected_exits registry, dropping exits whose affect list empties.
static void UpdateExitsAffects() {
	for (auto it = affected_exits.begin(); it != affected_exits.end();) {
		RoomData *const room = it->room;
		const int dir = it->dir;
		const auto exit = (room && dir >= 0 && dir < EDirection::kMaxDirNum) ? room->dir_option[dir] : nullptr;
		if (!exit || exit->affected.empty()) {
			it = affected_exits.erase(it);
			continue;
		}
		auto &affects = exit->affected;
		for (auto ai = affects.begin(); ai != affects.end();) {
			const auto &af = *ai;
			if (af->duration >= 1) {
				af->duration--;
				++ai;
			} else if (af->duration == -1) {
				++ai;   // permanent: never decays
			} else {
				if (af->affect_type != ERoomAffect::kUndefined) {
					SendRemoveAffectMsgToRoom(af->affect_type, GetRoomRnum(room->vnum));
				}
				// issue.character-affect-triggers: kExpired on a door affect's timeout. The exit loop
				// doesn't track the caster like the room loop, so resolve it here (no-op if gone).
				RunRoomAffectTrigger(room, find_char(af->caster_id), af->affect_type,
									 talents_actions::EActionTrigger::kExpired, af->potency);
				ai = affects.erase(ai);
			}
		}
		if (exit->affected.empty()) {
			it = affected_exits.erase(it);
		} else {
			++it;
		}
	}
}

// раз в 2 секунды
void UpdateRoomsAffects() {
	CharData *ch;

	for (auto room = affected_rooms.begin(); room != affected_rooms.end();) {
		assert(*room);
		auto &affects = (*room)->affected;
		auto next_affect_i = affects.begin();
		for (auto affect_i = next_affect_i; affect_i != affects.end(); affect_i = next_affect_i) {
			++next_affect_i;
			const auto &affect = *affect_i;
			// issue.affect-migration: caster dependency read from the room affect's OWN flags (set in
			// room_affects.xml -- the room-affect counterparts of the spell EMagic kMagCaster* flags), so
			// the behavior lives on the affect, not MUD::Spell(af->type). General mechanism: any room affect
			// carrying these flags gets the treatment. ch (the resolved caster) flows to HandleRoomAffect.
			ch = nullptr;
			if (IS_SET(affect->battleflag, kAfCasterInRoom) || IS_SET(affect->battleflag, kAfCasterInWorld)) {
				ch = find_char_in_room(affect->caster_id, *room);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (IS_SET(affect->battleflag, kAfCasterInWorldDelay)) {
				// задержка таймера: не обнуляем, даже если кастера нет, просто тикаем как обычно
				ch = find_char_in_room(affect->caster_id, *room);
			}

			if ((!ch) && IS_SET(affect->battleflag, kAfCasterInWorld)) {
				ch = find_char(affect->caster_id);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (IS_SET(affect->battleflag, kAfCasterInWorldDelay)) {
				ch = find_char(affect->caster_id);
			}

			// kAfCasterInWorldDelay affects decay while their caster is away (general -- the old code
			// special-cased kRuneLabel, the only such affect).
			if (IS_SET(affect->battleflag, kAfCasterInWorldDelay) && !ch) {
				affect->duration--;
			}

			if (affect->duration >= 1) {
				affect->duration--;
				// вот что это такое здесь ?
			} else if (affect->duration == -1) {
				affect->duration = -1;
			} else {
				// issue.affect-migration: expiry announcement keys on the room-affect identity
				// (affect_type), not the legacy Affect::type (ESpell) -- so affects whose ESpell was
				// retired (e.g. the portal pair) still announce. Dedup a multi-slot affect by affect_type.
				if (affect->affect_type != ERoomAffect::kUndefined) {
					if (next_affect_i == affects.end()
						|| (*next_affect_i)->affect_type != affect->affect_type
						|| (*next_affect_i)->duration > 0) {
						SendRemoveAffectMsgToRoom(affect->affect_type, GetRoomRnum((*room)->vnum));
					}
				}
				// issue.character-affect-triggers: kExpired -- the room affect's timer ran out; fire its
				// kExpired actions in the caster's context (ch, resolved above) before the strip. No-op
				// if the caster is gone (ch == nullptr), exactly like the pulse tick.
				RunRoomAffectTrigger(*room, ch, affect->affect_type,
									 talents_actions::EActionTrigger::kExpired, affect->potency);
				RoomRemoveAffect(*room, affect_i);
				continue;  // Чтоб не вызвался обработчик
			}

			// Учитываем что время выдается в пульсах а не в секундах  т.е. надо умножать на 2
			affect->apply_time++;
			// issue.affect-migration: run the affect's pulse-triggered <actions>. HandleRoomAffect ->
			// RunRoomTick filters by each action's own <trigger> (kPulse / kBattlePulse) and no-ops for
			// passive affects, so this is safe to call for every room affect.
			HandleRoomAffect(*room, ch, affect);
		}

		//если больше аффектов нет, удаляем комнату из списка обкастованных
		if ((*room)->affected.empty()) {
			room = affected_rooms.erase(room);
			//Инкремент итератора. Здесь, чтобы можно было удалять элементы списка.
		} else if (room != affected_rooms.end()) {
			++room;
		}
	}
	// issue.room-affect-trigger-improve: tick door affects on the same 2s cadence.
	UpdateExitsAffects();
}

// =============================================================== //

// Apply a room-targeted spell to ch's current room. Pure data-driven: every
// affect parameter -- duration, modifier, potency, debuff flag, refresh /
// dedup policy, imposition narration -- comes from the spell's XML
// <talent_actions>/<affects> block and spell_msg.xml. No per-spell switch or
// code-set messages remain. New room spells need only their XML rows; this
// function is unchanged.
// Three universal gates run before any affect data is computed:
//   1. Material component (ProcessMatComponents). Missing -> abort silently.
//   2. <blocking><room_flags val>. Room carries a blocking flag and the caster
//      is not exempt (MayCastInForbiddenRoom) -> emit kCastForbidden* and
//      abort. This is the kRuneLabel/kForbidden/kNoMagic fizzle path.
//   3. (no third gate -- everything else is data-driven inside the impose loop)
// Duration unit: room-affect durations are in RAW ROOM-TICK PULSES, NOT the
// hours used by char affects. The reader uses base + skill_bonus directly,
// without the kSecsPerMudHour multiplier that CalcDuration applies for PCs.
// This preserves the OLD pulse-direct semantics of kDeadlyFog (8 pulses),
// kMeteorStorm (3 pulses), etc. -- sub-hour values that hours-based duration
// can't express in integers.
// Impose the spell's room affect on ctx.rvict from the CURRENT action's <affects> block.
// Reads ctx.action_or_default(): action[0] for the legacy CallMagicToRoom entry, or the cursor
// action when driven from the per-action loop -- so a kTarRoomThis action can scale the room
// affect off a prior action's result via ctx.CompetenceBase(). kSuccess on impose, kNotCast on no-effect.
// issue.dispellbug: classify an actor against an existing room affect (author/ally
// => free action; otherwise a strength contest, with PK liability for a player vs a
// live author). Author lookup via chardata_by_uid: presence + in-world => alive author.
RoomAffectActor ClassifyRoomAffectAccess(CharData *ch, long caster_id) {
	CharData *author = nullptr;
	const auto it = chardata_by_uid.find(caster_id);
	if (it != chardata_by_uid.end() && it->second
			&& !it->second->purged() && it->second->in_room != kNowhere) {
		author = it->second;
	}
	if (ch->get_uid() == caster_id) {
		return {true, author};
	}
	if (author && group::same_group(ch, author)) {
		return {true, author};
	}
	return {false, author};
}

ECastResult CastRoomAffect(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	RoomData *const room = ctx.rvict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr || room == nullptr) {
		return ECastResult::kNotCast;
	}
	// issue.affects-improve: the affect this spell imposes is exactly what its <affects id=...> declares.
	const ERoomAffect room_affect = ctx.action_or_default().Contains(talents_actions::EAction::kAffect)
		? ctx.action_or_default().GetAffect().GetRoomAffect() : ERoomAffect::kUndefined;
	// Default-init the affect array. kMaxSpellAffects slots; only slot 0 is
	// populated from the XML today, but the impose loop still walks them all
	// so multi-apply room spells stay forward-compatible.
	Affect<ERoomApply> af[kMaxSpellAffects];
	for (int i = 0; i < kMaxSpellAffects; i++) {
		af[i].affect_type = static_cast<ERoomAffect>(0);
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = kNone;
		af[i].caster_id = 0;
		af[i].apply_time = 0;
		af[i].duration = 0;
	}

	// Data-driven defaults from the spell's <talent_actions>/<affects> block.
	// Fills af[0] with duration, battleflag, modifier, potency, debuff -- the
	// same fields CastAffect populates on a per-target affect (the helpers
	// CalcCastPotency / ComputeApplyModifier are shared with CastAffect's
	// apply_one path so both record the same values for the same cast roll).
	if (ctx.action_or_default().Contains(talents_actions::EAction::kAffect)) {
		const auto &talent = ctx.action_or_default().GetAffect();
		af[0].affect_type = room_affect;
		af[0].caster_id = ch->get_uid();
		// issue.affect-migration: flags come from room_affects.xml by affect_type (so the update-gate
		// below reads the right kAfUpdateDuration/kAfUpdateMod).
		af[0].battleflag = RoomAffectFlagsByType(af[0].affect_type);
		const ESkill dur_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
		int skill_bonus = (talent.GetDurationSkillDivisor() > 0 && dur_skill != ESkill::kUndefined)
			? CalcNoviceSkillBonus(ch, dur_skill, talent.GetDurationSkillDivisor()) : 0;
		if (talent.GetDurationMin() > 0) skill_bonus = std::max(skill_bonus, talent.GetDurationMin());
		if (talent.GetDurationMax() > 0) skill_bonus = std::min(skill_bonus, talent.GetDurationMax());
		af[0].duration = talent.GetDurationBase() + static_cast<unsigned>(skill_bonus);
		// Stored potency scaled by <affects potency_weight=> (default 1.0).
		// Symmetric with the char-affect path in TryApplyAffectTalent.
		af[0].potency = CalcCastPotency(ctx.potency()) * talent.GetPotencyWeight();
		af[0].charges = talent.GetChargesMax();   // issue.room-affect-trigger-improve: trigger charges (-1 = unlimited)
		// issue.affects-improve (P1 fix): the seal strength (modifier) is the ROOM AFFECT's own
		// property (room_affects.xml <seal_strength>), not the casting spell's apply. The spell now
		// names the affect via a bare <affect id=> ref carrying no coefficients, so reading
		// talent.GetApplies()[0] here yielded modifier 0 -- which silently disabled the kForbidden
		// ward (gated on number(1,100) <= af->modifier) and zeroed the sight.cpp star rating.
		if (RoomAffectHasSeal(af[0].affect_type)) {
			af[0].modifier = ComputeApplyModifier(RoomAffectSeal(af[0].affect_type),
												  ctx.CompetenceBase(), ctx.potency());
		}
	}

	/*
	// The hack for displaying the sealing level has been removed. The code is
	// left in case we need to quickly revert it.
	if (spell_id == ESpell::kForbidden) {
		if (af[0].modifier > 99) {
			// top tier -> spell_msg.xml kForbidden/kAffImposed*
		} else if (af[0].modifier > 79) {
			to_char = "Вы почти полностью запечатали магией все входы.";
			to_room = "$n почти полностью запечатал$g магией все входы.";
		} else {
			to_char = "Вы очень плохо запечатали магией все входы.";
			to_room = "$n очень плохо запечатал$g магией все входы.";
		}
	}
	*/

	// Refresh / dedup policy. kMagNeedControl spells cancel any other
	// controlled affect; otherwise a duplicate cast is allowed iff the
	// affect's first slot carries kAfUpdateDuration. kAfUnique drops any
	// prior copy from this caster.
	bool success = true;
	bool handled_update = false;   // existing room affect updated in place -> skip impose loop
	if (MUD::Spell(spell_id).IsFlagged(kMagNeedControl)) {
		// issue.affects-improve: show the caster's interrupt notice FIRST (keyed on the OLD affect),
		// THEN remove it (which narrates the effect ceasing), THEN the new spell announces itself.
		const ERoomAffect controlled = FindControlledRoomAffect(ch);
		if (controlled != ERoomAffect::kUndefined) {
			const auto &imsg = RoomAffectMsg(controlled, ERoomAffectMsgType::kAffInterruptedToChar);
			if (!imsg.empty()) {
				act(imsg.c_str(), false, ch, nullptr, nullptr, kToChar);
			}
		}
		if (RemoveControlledRoomAffect(ch) != ERoomAffect::kUndefined) {
			SendSpellNameMsg(ch, spell_id, ESpellMsg::kCastPreparedToChar);
		}
	} else {
		auto RoomAffect_i = FindAffect(room, room_affect);
		if (RoomAffect_i != room->affected.end() && !IS_SET(af[0].battleflag, kAfUnique)) {
			// issue.dispellbug: re-cast over an existing (non per-caster-unique) room affect.
			// Per-field gating: kAfUpdateDuration -> duration, kAfUpdateMod -> modifier;
			// neither -> not updatable. Author/live-ally update for free (buff OR weaken);
			// an outsider must win a strength contest, and a player outsider vs a live
			// author commits an aggressive PK act.
			const auto &existing = *RoomAffect_i;
			const bool can_dur = IS_SET(af[0].battleflag, kAfUpdateDuration);
			const bool can_mod = IS_SET(af[0].battleflag, kAfUpdateMod);
			if (!can_dur && !can_mod) {
				success = false;
			} else {
				// Only dispellable wards (e.g. kForbidden) require an author/ally + strength
				// contest to update; non-dispellable room buffs (e.g. room light) update
				// freely for now (issue.dispellbug).
				bool may = true;
				if (IS_SET(af[0].battleflag, kAfDispellable)) {
					const auto access = ClassifyRoomAffectAccess(ch, existing->caster_id);
					may = access.free;
					if (!access.free) {
						if (!ch->IsNpc() && access.author && !pk_agro_action(ch, access.author)) {
							success = false;
						} else if (number(1, 100) <= 5 || af[0].potency > existing->potency) {
							may = true;
						} else {
							success = false;
						}
					}
				}
				if (success && may) {
					if (can_dur) {
						existing->duration =
							CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[0].duration);
					}
					if (can_mod) {
						existing->modifier = af[0].modifier;
					}
					existing->caster_id = ch->get_uid();   // authorship transfer
					AddRoomToAffected(room);
					handled_update = true;
				}
			}
		} else if (IS_SET(af[0].battleflag, kAfUnique)) {
			RemoveSingleRoomAffect(ch->get_uid(), room_affect);
		}
	}

	// Impose loop. Each affect's battleflag drives the join policy: kAfUpdate
	// Duration -> affect_room_join_fspell (replace by spell id); otherwise
	// affect_room_join with kAfAccumulateDuration deciding whether durations
	// stack. Empty slots (no duration, no location) are skipped.
	for (int i = 0; !handled_update && success && i < kMaxSpellAffects; i++) {
		if (af[i].duration
			|| af[i].location != kNone) {
			af[i].duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (IS_SET(af[i].battleflag, kAfUpdateDuration)) {
				affect_room_join_fspell(room, af[i]);
			} else {
				const bool accum_duration = IS_SET(af[i].battleflag, kAfAccumulateDuration);
				affect_room_join(room, af[i], accum_duration, false, false, false);
			}
			AddRoomToAffected(room);
		}
	}

	if (success) {
		// Imposition narration: pure spell_msg.xml lookup, sheaf-direct so a
		// missing key stays silent (same convention as CastAffect's
		// EmitImpositionEffects).
		// Prefer the room-affect message system (keyed by the affect's ERoomAffect identity);
		// fall back to the spell sheaf for anything not yet migrated.
		const ERoomAffect ra = room_affect;
		const auto &sheaf = MUD::SpellMessages()[spell_id];
		const std::string &room_to_room = RoomAffectMsgRaw(ra, ERoomAffectMsgType::kAffImposedToRoom);
		const std::string &to_room = !room_to_room.empty() ? room_to_room
				: sheaf.GetMessage(ESpellMsg::kAffImposedToRoom);
		if (!to_room.empty()) {
			act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		const std::string &room_to_char = RoomAffectMsgRaw(ra, ERoomAffectMsgType::kAffImposedToChar);
		const std::string &to_char = !room_to_char.empty() ? room_to_char
				: sheaf.GetMessage(ESpellMsg::kAffImposedToChar);
		if (!to_char.empty()) {
			act(to_char.c_str(), true, ch, nullptr, nullptr, kToChar);
		}
		return ECastResult::kSuccess;
	}

	SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	return ECastResult::kNotCast;
}

ECastResult CallMagicToRoom(CharData *ch, RoomData *room, ActionContext roll) {
	const ESpell spell_id = roll.spell_id();   // roll.level is unused for room casts
	roll.cvict = nullptr;
	roll.rvict = room;

	if (room == nullptr || ch == nullptr || ch->in_room == kNowhere) {
		return ECastResult::kNotCast;
	}

	// Material component: silently abort if the cast requires one and ch
	// can't provide it. No-op for spells without a configured component.
	if (ProcessMatComponents(ch, ch, spell_id) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}

	// Data-driven room block: same mechanism as in
	// CallMagic. Fizzle narration lives in spell_msg.xml; the kDefault
	// sheaf covers the generic case, per-spell sheaves override.
	if (IsRoomBlocked(world[ch->in_room], MUD::Spell(spell_id).actions.GetBlocking())
			&& !MayCastInForbiddenRoom(ch)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToChar) + "\r\n", ch);
		const auto &fizzle_room = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToRoom);
		if (!fizzle_room.empty()) {
			act(fizzle_room.c_str(), false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return ECastResult::kNotCast;
	}

	// Data-driven dispel. A kTarRoomThis spell carrying an <unaffect>
	// block strips room affects here, mirroring CastToSingleTarget's "Damage -> Unaffect ->
	// Affect" ordering. CastUnaffects emits its own dispel / no-effect narration via the
	// kAffDispelledTo{Char,Room} sheaves. For a pure-dispel spell (no <affects>) the impose
	// loop below then runs as a no-op; for a dual spell (kMagAffects + <unaffect>) the affect
	// imposition continues normally.
	if (CastUnaffects(roll) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}

	return CastRoomAffect(roll);
}

// issue.room-affect-trigger-improve (door affects): host a room affect on an EXIT instead of the room.
// Same Affect<ERoomApply> type, same impose -- only the host list differs. Behaviour flags come from
// room_affects.xml by affect_type, exactly like affect_to_room.
void affect_to_exit(const RoomData::exit_data_ptr &exit, const Affect<ERoomApply> &af) {
	Affect<ERoomApply>::shared_ptr new_affect(new Affect<ERoomApply>(af));
	if (RoomAffectFlagsLoaded() && af.affect_type != ERoomAffect::kUndefined) {
		new_affect->battleflag = RoomAffectFlagsByType(af.affect_type)
				| (af.battleflag & static_cast<Bitvector>(kAfFailed));
	}
	exit->affected.push_front(new_affect);
}

// Cast a kMagRoom spell onto the exit in direction `dir` from the caster's room (a kTarDirection cast).
// Mirrors CallMagicToRoom's affect build but imposes on the exit/door. NOTE (first cut): no dedup/PK
// contest and no expiry tick yet -- a door affect persists until dispelled; both are follow-ups.
ECastResult CallMagicToExit(CharData *ch, int dir, ActionContext roll) {
	const ESpell spell_id = roll.spell_id();
	roll.cvict = nullptr;
	if (ch == nullptr || ch->in_room == kNowhere || dir < 0 || dir >= EDirection::kMaxDirNum) {
		return ECastResult::kNotCast;
	}
	const auto exit = world[ch->in_room]->dir_option[dir];
	if (!exit || exit->to_room() == kNowhere) {
		SendMsgToChar("В этом направлении нет прохода.\r\n", ch);
		return ECastResult::kNotCast;
	}
	if (ProcessMatComponents(ch, ch, spell_id) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}
	// A direction cast runs ONLY the passage-meaningful stages of the spell -- impose an affect on the
	// door and/or dispel affects already on it. Char-targeted stages (damage / points / summon / ...) are
	// intentionally skipped: they are pointless on a passage. This is the general mechanism -- the spell
	// just declares <targets ...kTarDirection> and carries <affects>/<unaffect>; no per-spell id check.
	const auto &action = roll.action_or_default();
	bool did = false;

	// Dispel stage: strip dispellable affects on the passage. This is how `развеять магию <dir>` works
	// (kDispellMagic's <unaffect> block) -- it is no longer a kDispellMagic special-case in CallMagic.
	if (action.Contains(talents_actions::EAction::kUnaffect)) {
		if (DispelExitAffects(ch, dir, spell_id)) { did = true; }
	}

	// Affect-impose stage: build the affect from the spell's <affects> block (same fields CastRoomAffect
	// fills on af[0]) and host it on the exit.
	if (action.Contains(talents_actions::EAction::kAffect)) {
		const auto &talent = action.GetAffect();
		Affect<ERoomApply> af;
		af.affect_type = talent.GetRoomAffect();
		af.caster_id = ch->get_uid();
		af.location = kNone;
		af.battleflag = RoomAffectFlagsByType(af.affect_type);
		const ESkill dur_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
		int skill_bonus = (talent.GetDurationSkillDivisor() > 0 && dur_skill != ESkill::kUndefined)
			? CalcNoviceSkillBonus(ch, dur_skill, talent.GetDurationSkillDivisor()) : 0;
		if (talent.GetDurationMin() > 0) skill_bonus = std::max(skill_bonus, talent.GetDurationMin());
		if (talent.GetDurationMax() > 0) skill_bonus = std::min(skill_bonus, talent.GetDurationMax());
		af.duration = talent.GetDurationBase() + static_cast<unsigned>(skill_bonus);
		af.potency = CalcCastPotency(roll.potency()) * talent.GetPotencyWeight();
		af.charges = talent.GetChargesMax();   // trigger charges (-1 = unlimited)
		if (RoomAffectHasSeal(af.affect_type)) {
			af.modifier = ComputeApplyModifier(RoomAffectSeal(af.affect_type), roll.CompetenceBase(), roll.potency());
		}
		affect_to_exit(exit, af);
		AddExitToAffected(world[ch->in_room], dir);   // register for the timer tick (UpdateExitsAffects)
		const auto &to_char = RoomAffectMsg(af.affect_type, ERoomAffectMsgType::kAffImposedToChar);
		if (!to_char.empty()) { act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar); }
		const auto &to_room = RoomAffectMsg(af.affect_type, ERoomAffectMsgType::kAffImposedToRoom);
		if (!to_room.empty()) { act(to_room.c_str(), false, ch, nullptr, nullptr, kToRoom | kToArenaListen); }
		did = true;
	}

	if (!did) {
		// No passage-meaningful stage (a pure damage / heal / summon spell) -- nothing happens on a
		// doorway. The spell's own no-effect line says so.
		const auto &m = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect);
		SendMsgToChar((m.empty() ? "Это бессмысленно проделывать с проходом." : m) + "\r\n", ch);
		return ECastResult::kNotCast;
	}
	return ECastResult::kSuccess;
}

int GetUniqueAffectDuration(long caster_id, ERoomAffect want) {
	for (const auto &room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->affect_type == want && af->caster_id == caster_id) {
				return af->duration;
			}
		}
	}
	return 0;
}

void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af) {
	bool found = false;

	for (const auto &hjp : room->affected) {
		if (hjp->affect_type == af.affect_type && hjp->location == af.location) {
			if (hjp->modifier < af.modifier) {
				hjp->modifier = af.modifier;
			}
			if (hjp->duration < af.duration) {
				hjp->duration = af.duration;
			}
			found = true;
			break;
		}
	}

	if (!found) {
		affect_to_room(room, af);
	}
}

void AffectRoomJoinReplace(RoomData *room, const Affect<ERoomApply> &af) {
	bool found = false;

	for (auto &affect_i : room->affected) {
		if (affect_i->affect_type == af.affect_type && affect_i->location == af.location) {
			affect_i->duration = af.duration;
			affect_i->modifier = af.modifier;
			found = true;
		}
	}
	if (!found) {
		affect_to_room(room, af);
	}
}

void affect_room_join(RoomData *room, Affect<ERoomApply> &af,
					  bool add_dur, bool avg_dur, bool add_mod, bool avg_mod) {
	bool found = false;

	if (af.location) {
		for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i) {
			const auto &affect = *affect_i;
			if (affect->affect_type == af.affect_type
				&& affect->location == af.location) {
				if (add_dur) {
					af.duration += affect->duration;
				}
				if (avg_dur) {
					af.duration /= 2;
				}
				if (add_mod) {
					af.modifier += affect->modifier;
				}
				if (avg_mod) {
					af.modifier /= 2;
				}
				RoomRemoveAffect(room, affect_i); //тут будет крешь, нужно RefreshRoomAffects см выше
				affect_to_room(room, af);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		affect_to_room(room, af);
	}
}

void affect_to_room(RoomData *room, const Affect<ERoomApply> &af) {
	Affect<ERoomApply>::shared_ptr new_affect(new Affect<ERoomApply>(af));
	// issue.affect-migration R2: room-affect behavior flags are sourced from room_affects.xml by
	// affect_type; the caller contributes only the per-instance kAfFailed bit. Guarded on the table
	// being loaded (pre-cfg boot keeps caller flags); kUndefined room affects (no row) keep theirs.
	if (RoomAffectFlagsLoaded() && af.affect_type != ERoomAffect::kUndefined) {
		new_affect->battleflag = RoomAffectFlagsByType(af.affect_type)
				| (af.battleflag & static_cast<Bitvector>(kAfFailed));
	}

	room->affected.push_front(new_affect);
}

/**
 * Removing room affect from first affected room.
 * @param ch - affect caster.
 * @param affect - the room affect to remove.
 */
void RemoveSingleAffectFromWorld(CharData *ch, ERoomAffect affect) {
	auto affected_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), affect);
	if (affected_room) {
		const auto aff = room_spells::FindAffect(affected_room, affect);
		if (aff != affected_room->affected.end()) {
			room_spells::RoomRemoveAffect(affected_room, aff);
			// issue.affects-improve: owner notice from the room-affect message system, keyed by the
			// affect itself (no spell lookup). kDefault = "Ваша магия была развеяна."; kRuneLabel
			// overrides with "Ваша рунная метка удалена.".
			const auto &msg = RoomAffectMsg(affect, ERoomAffectMsgType::kAffDispelledToChar);
			if (!msg.empty()) {
				act(msg.c_str(), false, ch, nullptr, nullptr, kToChar);
			}
		}
	}
}

void ProcessRoomAffectsOnEntry(CharData *ch, RoomRnum room) {
	// issue.room-affect-trigger-improve: a FORCED / after-placement entry (teleport, summon, flee, or a
	// non-walk placement). Run every on-entry action for its EFFECT; the block verdict is ignored, since
	// a forced entry can't be refused. The interruptible walk path does NOT use this -- it calls the
	// dispatcher directly with kBlockCheck (before placement) + kEffectsNonBlocking (after).
	(void) RunRoomEntryTriggers(ch, world[room], EEntryTriggerPhase::kEffectsAll);
}

} // namespace room_spells

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
