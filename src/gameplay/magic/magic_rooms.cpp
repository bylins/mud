#include "magic_rooms.h"
#include "administration/privilege.h"
#include "gameplay/handlers/spell_handlers.h"

#include "spells_info.h"
#include "engine/ui/modify.h"
#include "engine/entities/char_data.h"
#include "magic.h" //Включено ради material_component_processing
#include "magic_utils.h" // IsRoomBlocked / MayCastInForbiddenRoom
#include "magic_internal.h" // BuildCastContext / CastUnaffects / ProcessMatComponents
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
#include "engine/structs/bitset_flags.h"   // BitsetFlags<RoomAffectTrigger>
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
		{room_spells::ERoomAffectMsgType::kRoomAffectVisible, "kRoomAffectVisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectInvisible, "kRoomAffectInvisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectSelfInvisible, "kRoomAffectSelfInvisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectPkVisible, "kRoomAffectPkVisible"},
		{room_spells::ERoomAffectMsgType::kRoomAffectPkInvisible, "kRoomAffectPkInvisible"},
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
// Resolve a room-affecting spell to its identity flag. The ERoomAffect enumerators are named after
// their spells, so the spell's enum token round-trips to the room-affect flag; spells with no matching
// room-affect flag yield kUndefined (no identity).
ERoomAffect RoomAffectBySpell(ESpell spell_id) {
	try {
		return ITEM_BY_NAME<ERoomAffect>(NAME_BY_ITEM<ESpell>(spell_id));
	} catch (const std::out_of_range &) {
		return ERoomAffect::kUndefined;
	}
}

// issue.affect-migration: per-ERoomAffect behavior flags from room_affects.xml (mirrors the
// char-affect g_affect_flags). Source of truth for room-affect cure/dispel/refresh/decrement/...;
// the casting source only sets strength + duration. Indexed by to_underlying(ERoomAffect)
// (1-based; index 0 = kUndefined = no flags).
constexpr std::size_t kRoomAffectFlagTableSize = to_underlying(ERoomAffect::kCount);
std::array<Bitvector, kRoomAffectFlagTableSize> g_room_affect_flags{};
// issue.affect-migration: active room affects own their per-tick action here (moved off the casting
// spell) -- a data tick_spell or a code tick_handler. When the action runs is governed by the affect's
// <trigger> set (g_room_affect_triggers); see RoomAffectTrigger.
std::array<ESpell, kRoomAffectFlagTableSize> g_room_affect_tick_spell{};
std::array<std::string, kRoomAffectFlagTableSize> g_room_affect_tick_handler{};
// issue.affect-migration: an affect's own <actions> (same format as a spell's <talent_actions>) --
// the room affect performs these directly each tick, replacing a dedicated kService tick spell.
std::array<talents_actions::Actions, kRoomAffectFlagTableSize> g_room_affect_actions{};
// issue.affect-migration: which events fire the affect's actions/tick (room_affects.xml <trigger>).
// An empty set = no trigger = the affect never runs its actions.
std::array<BitsetFlags<RoomAffectTrigger>, kRoomAffectFlagTableSize> g_room_affect_triggers{};
bool g_room_affect_flags_loaded = false;

// issue.affect-migration: parse a room_affects.xml <trigger val="kPulse|kBattlePulse"> token list
// into a RoomAffectTrigger set. Unknown tokens are reported and skipped.
BitsetFlags<RoomAffectTrigger> ParseRoomAffectTriggers(const char *val, const char *id) {
	static const std::map<std::string, RoomAffectTrigger> kNames = {
		{"kPulse", RoomAffectTrigger::kPulse},
		{"kBattlePulse", RoomAffectTrigger::kBattlePulse},
	};
	BitsetFlags<RoomAffectTrigger> triggers;
	std::string token;
	for (const char *p = val;; ++p) {
		if (*p == '|' || *p == '\0') {
			if (!token.empty()) {
				if (const auto it = kNames.find(token); it != kNames.end()) {
					triggers.set(it->second);
				} else {
					log("SYSERROR: room_affects.xml: unknown trigger '%s' for %s.", token.c_str(), id);
				}
				token.clear();
			}
			if (*p == '\0') { break; }
		} else if (*p != ' ' && *p != '\t') {
			token.push_back(*p);
		}
	}
	return triggers;
}

// issue.affect-migration: is a fight underway in this room? (gates kBattlePulse triggers.)
bool RoomHasCombat(RoomData *room) {
	for (const auto tch : room->people) {
		if (tch->GetEnemy()) {
			return true;
		}
	}
	return false;
}

void BuildRoomAffectFlagTable(parser_wrapper::DataNode data) {
	g_room_affect_flags.fill(0);
	g_room_affect_tick_spell.fill(ESpell::kUndefined);
	for (auto &h : g_room_affect_tick_handler) { h.clear(); }
	for (auto &a : g_room_affect_actions) { a = talents_actions::Actions{}; }
	g_room_affect_triggers.fill(BitsetFlags<RoomAffectTrigger>{});
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
		if (const char *flags = node.GetValue("flags"); flags && *flags) {
			g_room_affect_flags[idx] = parse::ReadAsConstantsBitvector<EAffFlag>(flags);
		}
		if (const char *ts = node.GetValue("tick_spell"); ts && *ts) {
			g_room_affect_tick_spell[idx] = parse::ReadAsConstant<ESpell>(ts);
		}
		if (const char *th = node.GetValue("tick_handler"); th && *th) {
			g_room_affect_tick_handler[idx] = th;
		}
		// issue.affect-migration: read child elements off a COPY of the iteration node (the obj_sets
		// pattern); mutating the range's own node with GoToChild/GoToParent does not round-trip.
		{
			auto trig_node = node;
			if (trig_node.GoToChild("trigger")) {
				if (const char *tv = trig_node.GetValue("val"); tv && *tv) {
					g_room_affect_triggers[idx] = ParseRoomAffectTriggers(tv, id);
				}
			}
		}
		{
			auto act_node = node;
			if (act_node.GoToChild("actions")) {
				g_room_affect_actions[idx].Build(act_node);
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

ESpell RoomAffectTickSpell(ERoomAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_tick_spell[idx] : ESpell::kUndefined;
}
const std::string &RoomAffectTickHandler(ERoomAffect affect_type) {
	static const std::string kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_tick_handler[idx] : kEmpty;
}

const talents_actions::Actions &RoomAffectActions(ERoomAffect affect_type) {
	static const talents_actions::Actions kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_actions[idx] : kEmpty;
}

bool HasRoomAffectTrigger(ERoomAffect affect_type, RoomAffectTrigger trigger) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize && g_room_affect_triggers[idx].test(trigger);
}

void RoomAffectsLoader::Load(parser_wrapper::DataNode data) { ValidateRoomAffectRegistry(data); BuildRoomAffectFlagTable(data); }
void RoomAffectsLoader::Reload(parser_wrapper::DataNode data) { ValidateRoomAffectRegistry(data); BuildRoomAffectFlagTable(data); }

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

const int kRuneLabelDuration = 300;

std::list<RoomData *> affected_rooms;

void RemoveSingleRoomAffect(long caster_id, ESpell spell_id);
void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff);
void SendRemoveAffectMsgToRoom(ESpell affect_type, ERoomAffect room_affect, RoomRnum room);
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

RoomAffectIt FindAffect(RoomData *room, ESpell type) {
	for (auto affect_i = room->affected.begin(); affect_i != room->affected.end(); ++affect_i) {
		const auto affect = *affect_i;
		if (affect->type == type) {
			return affect_i;
		}
	}
	return room->affected.end();
}

bool IsZoneRoomAffected(int zone_vnum, ESpell spell) {
	for (auto & affected_room : affected_rooms) {
		if (affected_room->zone_rn == zone_vnum && IsRoomAffected(affected_room, spell)) {
			return true;
		}
	}
	return false;
}

bool IsRoomAffected(RoomData *room, ESpell spell) {
	for (const auto &af : room->affected) {
		if (af->type == spell) {
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
		if (af->type == ESpell::kPortalTimer
				&& af->pk_unique != 0
				&& af->pk_unique != exclude_uid) {
			return af->pk_unique;
		}
	}
	return 0;
}

void ShowAffectedRooms(CharData *ch) {
	std::stringstream out;
	out << " Список комнат под аффектами:" << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader <<
		"#" << "Vnum" << "Spell" << "Caster name" << "Time (s)" << table_wrapper::kEndRow;
	int count = 1;
	for (const auto r : affected_rooms) {
		for (const auto &af : r->affected) {
			table << count << r->vnum << MUD::Spell(af->type).GetName()
				  << GetNameById(af->caster_id) << af->duration * 2 << table_wrapper::kEndRow;
			++count;
		}
	}
	table_wrapper::DecorateServiceTable(ch, table);
	out << table.to_string() << "\r\n";

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

RoomData *FindAffectedRoomByCasterID(long caster_id, ESpell spell_id) {
	for (const auto room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->type == spell_id && af->caster_id == caster_id) {
				return room;
			}
		}
	}
	return nullptr;
}

template<typename F>
ESpell RemoveAffectFromRooms(ESpell spell_id, const F &filter) {
	for (const auto room : affected_rooms) {
		const auto &affect = std::find_if(room->affected.begin(), room->affected.end(), filter);
		if (affect != room->affected.end()) {
			SendRemoveAffectMsgToRoom((*affect)->type, (*affect)->affect_type, GetRoomRnum(room->vnum));
			spell_id = (*affect)->type;
			RoomRemoveAffect(room, affect);
			return spell_id;
		}
	}
	return ESpell::kUndefined;
}

void RemoveSingleRoomAffect(long caster_id, ESpell spell_id) {
	auto filter =
		[&caster_id, &spell_id](auto &af) { return (af->caster_id == caster_id && af->type == spell_id); };
	RemoveAffectFromRooms(spell_id, filter);
}

ESpell RemoveControlledRoomAffect(CharData *ch) {
	long casterID = ch->get_uid();
	auto filter =
		[&casterID](auto &af) {
			return (af->caster_id == casterID && MUD::Spell(af->type).IsFlagged(kMagNeedControl));
		};
	return RemoveAffectFromRooms(ESpell::kUndefined, filter);
}

void SendRemoveAffectMsgToRoom(ESpell affect_type, ERoomAffect room_affect, RoomRnum room) {
	// Prefer the room-affect message system; fall back to the spell expiry text (portals etc).
	const std::string &room_msg = RoomAffectMsgRaw(room_affect, ERoomAffectMsgType::kAffExpiredToRoom);
	if (!room_msg.empty()) {
		SendMsgToRoom(room_msg.c_str(), room, 0);
		return;
	}
	const std::string &msg = GetAffExpiredText(affect_type);
	if (affect_type >= ESpell::kFirst && affect_type <= ESpell::kLast && !msg.empty()) {
/*		if (affect_type == ESpell::kPortalTimer){
			mudlog("Пентаграмма медленно растаяла.");
		}
*/
		SendMsgToRoom(msg.c_str(), room, 0);
	};
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


// Раз в 2 секунды идет вызов обработчиков аффектов//
// String-named code tick handlers for room affects -- the manual-cast mechanism for per-tick
// room logic the data can't express. A room affect names one via <affects tick_handler="...">.
static const std::map<std::string, std::function<void(CharData *, const Affect<ERoomApply>::shared_ptr &)>>
		kRoomTickHandlers = {
	{"HandleThunderstormTick", handlers::HandleThunderstormTick},
};

// Per-tick room narration: cycle the impose spell's defined kCustomMsg slots by the affect's
// tick counter (apply_time), so a multi-phase room effect can show a different line each round.
static void EmitRoomTickMessage(CharData *ch, ESpell impose, int tick) {
	const auto &sheaf = MUD::SpellMessages()[impose];
	static const ESpellMsg slots[] = {ESpellMsg::kCustomMsgOne, ESpellMsg::kCustomMsgTwo,
		ESpellMsg::kCustomMsgThree, ESpellMsg::kCustomMsgFour, ESpellMsg::kCustomMsgFive,
		ESpellMsg::kCustomMsgSix, ESpellMsg::kCustomMsgSeven, ESpellMsg::kCustomMsgEight,
		ESpellMsg::kCustomMsgNine, ESpellMsg::kCustomMsgTen};
	std::vector<ESpellMsg> present;
	for (const auto k : slots) {
		if (sheaf.HasMessage(k)) present.push_back(k);
	}
	if (present.empty()) return;
	const auto &msg = sheaf.GetMessage(present[tick % present.size()]);
	if (!msg.empty()) {
		act(msg.c_str(), false, ch, nullptr, nullptr, kToRoom | kToChar | kToArenaListen);
	}
}

// Generic data-driven room-affect tick: if the impose spell carries <affects tick_spell="B">,
// cast B on the affected room each tick (with B's own actions/targets) and emit the cycled
// per-tick message. Returns true if it handled the tick (so HandleRoomAffect skips the hardcoded
// switch); false to fall through to the in-code handlers. The 4 legacy room handlers move onto
// this path in Stage D.
// NOTE: B is cast fresh via CastAreaInRoom (the legacy per-tick re-cast onto the room's foes).
// Two refinements are deferred until a spell needs them: (1) stored-potency-scaled ticks (use
// the affect's saved potency instead of B's fresh roll); (2) caster-independent ticks under
// kMagCasterAnywhere/kMagCasterInworldDelay when ch == nullptr (no caster to source the cast).
static bool RunRoomTick(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	const ESpell impose = aff->type;
	const ERoomAffect affect_type = aff->affect_type;
	// issue.affect-migration: the per-tick action is owned by the AFFECT (room_affects.xml), not the
	// casting spell -- read by affect_type. impose is kept only for the tick narration + error text.
	// 1. Code tick handler named by string (manual-cast mechanism for room ticks): the handler
	//    reads the affect directly (e.g. its duration). Used for ticks the data can't express.
	const std::string &handler = RoomAffectTickHandler(affect_type);
	if (!handler.empty()) {
		const auto it = kRoomTickHandlers.find(handler);
		if (it == kRoomTickHandlers.end()) {
			err_log("RunRoomTick: unknown tick_handler '%s' for spell %s.",
					handler.c_str(), NAME_BY_ITEM<ESpell>(impose).c_str());
			return true;
		}
		if (ch != nullptr) {
			it->second(ch, aff);
		}
		return true;
	}
	// 2. Data-driven tick_spell.
	// 2. Affect-owned actions (room_affects.xml <actions>): the affect performs its own per-tick
	//    action -- no dedicated tick spell needed. Context (level/potency) comes from the imposing
	//    spell (impose = aff->type); the action list is the affect's.
	if (const auto &actions = RoomAffectActions(affect_type); !actions.list().empty()) {
		if (ch == nullptr) {
			return true;
		}
		const int phase = aff->apply_time > 0 ? aff->apply_time - 1 : 0;
		EmitRoomTickMessage(ch, impose, phase);
		CastRoomTickActionFromActions(ch, room, impose, actions, phase);
		return true;
	}
	// 3. Data-driven tick_spell.
	const ESpell tick = RoomAffectTickSpell(affect_type);
	if (tick == ESpell::kUndefined) {
		return false;
	}
	if (ch == nullptr) {
		return true;  // tick-driven affect, but no caster to source the cast this tick
	}
	// apply_time is incremented before the handler runs, so the first tick is apply_time==1.
	// phase counts from 0 so action[phase % N] / kCustomMsg slot [phase % K] start at the first.
	const int phase = aff->apply_time > 0 ? aff->apply_time - 1 : 0;
	EmitRoomTickMessage(ch, impose, phase);
	if (MUD::Spell(tick).GetMode() == EItemMode::kService) {
		// multi-phase: run the tick spell's cycled action (action[phase % N]) on the room.
		CastRoomTickAction(ch, room, tick, phase);
	} else {
		// single-phase: area-cast the whole tick spell on the room's foes (legacy re-cast).
		CastAreaInRoom(ch, tick, GetRealLevel(ch));
	}
	return true;
}

void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	assert(aff);
	assert(room);
	// Fully data-driven dispatch (no per-spell switch): the affect owns its per-tick work via
	// <actions>, a tick_spell (data) or a tick_handler (code, by string). A triggered affect that
	// resolves to none of these is a misconfiguration -- fail loudly.
	if (!RunRoomTick(room, ch, aff)) {
		err_log("HandleRoomAffect: triggered room affect %s has no <actions>/tick_spell/tick_handler.",
				NAME_BY_ITEM<ESpell>(aff->type).c_str());
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
			auto spell_id = affect->type;
			ch = nullptr;

			if (MUD::Spell(spell_id).IsFlagged(kMagCasterInroom) ||
				MUD::Spell(spell_id).IsFlagged(kMagCasterInworld)) {
				ch = find_char_in_room(affect->caster_id, *room);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (MUD::Spell(spell_id).IsFlagged(kMagCasterInworldDelay)) {
				//Если спелл с задержкой таймера - то обнулять не надо, даже если чара нет, просто тикаем таймером как обычно
				ch = find_char_in_room(affect->caster_id, *room);
			}

			if ((!ch) && MUD::Spell(spell_id).IsFlagged(kMagCasterInworld)) {
				ch = find_char(affect->caster_id);
				if (!ch) {
					affect->duration = 0;
				}
			} else if (MUD::Spell(spell_id).IsFlagged(kMagCasterInworldDelay)) {
				ch = find_char(affect->caster_id);
			}

			if (!(ch && MUD::Spell(spell_id).IsFlagged(kMagCasterInworldDelay))) {
				switch (spell_id) {
					case ESpell::kRuneLabel: 
					affect->duration--;
					default: break;
				}
			}

			if (affect->duration >= 1) {
				affect->duration--;
				// вот что это такое здесь ?
			} else if (affect->duration == -1) {
				affect->duration = -1;
			} else {
				if (affect->type >= ESpell::kFirst && affect->type <= ESpell::kLast) {
					if (next_affect_i == affects.end()
						|| (*next_affect_i)->type != affect->type
						|| (*next_affect_i)->duration > 0) {
						SendRemoveAffectMsgToRoom(affect->type, affect->affect_type, GetRoomRnum((*room)->vnum));
					}
				}
				RoomRemoveAffect(*room, affect_i);
				continue;  // Чтоб не вызвался обработчик
			}

			// Учитываем что время выдается в пульсах а не в секундах  т.е. надо умножать на 2
			affect->apply_time++;
			// issue.affect-migration: the affect's <trigger> set decides when its actions/tick run.
			// kPulse fires every room-affect pulse (the old kAfMustBeHandled behavior); kBattlePulse
			// fires only while combat is happening in the room. An affect with no trigger never fires.
			const auto affect_type = affect->affect_type;
			if (HasRoomAffectTrigger(affect_type, RoomAffectTrigger::kPulse)
				|| (HasRoomAffectTrigger(affect_type, RoomAffectTrigger::kBattlePulse) && RoomHasCombat(*room))) {
				HandleRoomAffect(*room, ch, affect);
			}
		}

		//если больше аффектов нет, удаляем комнату из списка обкастованных
		if ((*room)->affected.empty()) {
			room = affected_rooms.erase(room);
			//Инкремент итератора. Здесь, чтобы можно было удалять элементы списка.
		} else if (room != affected_rooms.end()) {
			++room;
		}
	}
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

ECastResult CastRoomAffect(CastContext &ctx) {
	CharData *const ch = ctx.caster();
	RoomData *const room = ctx.rvict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr || room == nullptr) {
		return ECastResult::kNotCast;
	}
	// Default-init the affect array. kMaxSpellAffects slots; only slot 0 is
	// populated from the XML today, but the impose loop still walks them all
	// so multi-apply room spells stay forward-compatible.
	Affect<ERoomApply> af[kMaxSpellAffects];
	for (int i = 0; i < kMaxSpellAffects; i++) {
		af[i].type = spell_id;
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
		af[0].type = spell_id;
		af[0].affect_type = RoomAffectBySpell(spell_id);
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
		af[0].debuff = MUD::Spell(spell_id).IsViolent();
		if (!talent.GetApplies().empty()) {
			af[0].modifier = ComputeApplyModifier(talent.GetApplies()[0], ctx.CompetenceBase(), ctx.potency());
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
		auto found_spell = RemoveControlledRoomAffect(ch);
		if (found_spell != ESpell::kUndefined) {
			// Two separate messages: the interrupt line is
			// keyed on the OLD spell's sheaf so a per-spell override can flavour HOW
			// it ends ("свечение угасло" etc.); the prepare line is keyed on the NEW
			// spell so each spell announces its own preparation.
			SendSpellNameMsg(ch, found_spell, ESpellMsg::kCastInterruptedToChar);
			SendSpellNameMsg(ch, spell_id, ESpellMsg::kCastPreparedToChar);
		}
	} else {
		auto RoomAffect_i = FindAffect(room, spell_id);
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
			RemoveSingleRoomAffect(ch->get_uid(), spell_id);
		}
	}

	// Impose loop. Each affect's battleflag drives the join policy: kAfUpdate
	// Duration -> affect_room_join_fspell (replace by spell id); otherwise
	// affect_room_join with kAfAccumulateDuration deciding whether durations
	// stack. Empty slots (no duration, no location) are skipped.
	for (int i = 0; !handled_update && success && i < kMaxSpellAffects; i++) {
		af[i].type = spell_id;
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
		const ERoomAffect ra = RoomAffectBySpell(spell_id);
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

ECastResult CallMagicToRoom(CharData *ch, RoomData *room, CastContext roll) {
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

int GetUniqueAffectDuration(long caster_id, ESpell spell_id) {
	for (const auto &room : affected_rooms) {
		for (const auto &af : room->affected) {
			if (af->type == spell_id && af->caster_id == caster_id) {
				return af->duration;
			}
		}
	}
	return 0;
}

void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af) {
	bool found = false;

	for (const auto &hjp : room->affected) {
		if (hjp->type == af.type && hjp->location == af.location) {
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
		if (affect_i->type == af.type && affect_i->location == af.location) {
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
			if (affect->type == af.type
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
 * @param spell_id - removing spell affect.
 */
void RemoveSingleAffectFromWorld(CharData *ch, ESpell spell_id) {
	auto affected_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), spell_id);
	if (affected_room) {
		const auto aff = room_spells::FindAffect(affected_room, spell_id);
		if (aff != affected_room->affected.end()) {
			room_spells::RoomRemoveAffect(affected_room, aff);
			// kAfDispelledToOwner sheaf lookup: the kDefault
			// fallback is "Ваша магия была развеяна."; kRuneLabel overrides with
			// "Ваша рунная метка удалена.". Other spells using this path inherit the
			// default until a designer authors a per-spell line.
			SendSpellNameMsg(ch, spell_id, ESpellMsg::kAfDispelledToOwner);
		}
	}
}

void ProcessRoomAffectsOnEntry(CharData *ch, RoomRnum room) {
	if (privilege::IsImmortal(ch)) {
		return;
	}

	const auto affect_on_room = room_spells::FindAffect(world[room], ESpell::kHypnoticPattern);
	if (affect_on_room != world[room]->affected.end()) {
		CharData *caster = find_char((*affect_on_room)->caster_id);
		// если не в гопе, и не слепой
		if (!group::same_group(ch, caster)
			&& !AFF_FLAGGED(ch, EAffect::kBlind)){
			// отсекаем всяких непонятных личностей типо двойников и проч
			// \todo Пальцы себе отсеки за такой код. Переделать на константы, а еще лучше - сделать где-то enum или вообще конфиг с внумами мобов для спеллов
			// Клон вообще по флагу клона проверяется
			if  ((GET_MOB_VNUM(ch) >= 3000 && GET_MOB_VNUM(ch) < 4000) || GET_MOB_VNUM(ch) == 108 ) return;
			if (ch->has_master() && !ch->get_master()->IsNpc() && ch->IsNpc()) {
				return;
			}
			// если вошел игрок - ПвП - делаем проверку на шанс в зависимости от % магии кастующего
			// без магии и ниже 80%: шанс 25%, на 100% - 27%, на 200% - 37% ,при 300% - 47%
			// иначе пве, и просто кастим сон на входящего
			if (!ch->IsNpc() && (number (1, 100) > (25))) {
				return;
			}
			SendMsgToChar("Вы уставились на огненный узор, как баран на новые ворота.", ch);
			act("$n0 уставил$u на огненный узор, как баран на новые ворота.",
				true, ch, nullptr, ch, kToRoom | kToArenaListen);
			CallMagic(caster, ch, nullptr, nullptr, ESpell::kSleep, GetRealLevel(caster));
		}
	}
}

} // namespace room_spells

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
