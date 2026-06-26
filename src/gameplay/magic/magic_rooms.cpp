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
// issue.affect-migration: an affect's own <actions> (same format as a spell's <talent_actions>). Each
// <action> carries its own <trigger> (EActionTrigger) deciding when it fires; the room affect performs
// the matching actions directly each tick -- no dedicated tick_spell / tick_handler. A side_spell action
// replaces the old tick_spell; a manual_cast action replaces the old tick_handler.
std::array<talents_actions::Actions, kRoomAffectFlagTableSize> g_room_affect_actions{};
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

const talents_actions::Actions &RoomAffectActions(ERoomAffect affect_type) {
	static const talents_actions::Actions kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kRoomAffectFlagTableSize ? g_room_affect_actions[idx] : kEmpty;
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
	// issue.affect-migration: match by room-affect identity (ERoomAffect), not Affect::type.
	const ERoomAffect want = RoomAffectBySpell(type);
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

bool IsZoneRoomAffected(int zone_vnum, ESpell spell) {
	for (auto & affected_room : affected_rooms) {
		if (affected_room->zone_rn == zone_vnum && IsRoomAffected(affected_room, spell)) {
			return true;
		}
	}
	return false;
}

bool IsRoomAffected(RoomData *room, ESpell spell) {
	const ERoomAffect want = RoomAffectBySpell(spell);   // issue.affect-migration: by affect_type
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

void ShowAffectedRooms(CharData *ch) {
	std::stringstream out;
	out << " Список комнат под аффектами:" << "\r\n";

	table_wrapper::Table table;
	table << table_wrapper::kHeader <<
		"#" << "Vnum" << "Spell" << "Caster name" << "Time (s)" << table_wrapper::kEndRow;
	int count = 1;
	for (const auto r : affected_rooms) {
		for (const auto &af : r->affected) {
			table << count << r->vnum
				  // issue.affect-migration: room-affect name by ERoomAffect when its ESpell is retired.
				  << (af->type != ESpell::kUndefined ? MUD::Spell(af->type).GetName()
						: NAME_BY_ITEM<ERoomAffect>(af->affect_type))
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
	const ERoomAffect want = RoomAffectBySpell(spell_id);   // issue.affect-migration: by affect_type
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
	const ERoomAffect want = RoomAffectBySpell(spell_id);   // issue.affect-migration: by affect_type
	auto filter =
		[&caster_id, want](auto &af) { return (af->caster_id == caster_id && af->affect_type == want); };
	RemoveAffectFromRooms(spell_id, filter);
}

ESpell RemoveControlledRoomAffect(CharData *ch) {
	long casterID = ch->get_uid();
	// issue.affect-migration: "controlled" is the kAfNeedControl flag on the room affect (set in
	// room_affects.xml -- the room-affect counterpart of the spell's EMagic kMagNeedControl). General
	// flag check, not a specific affect.
	auto filter =
		[&casterID](auto &af) {
			return (af->caster_id == casterID && IS_SET(af->battleflag, kAfNeedControl));
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

// issue.affect-migration: per-tick room-affect handler. The affect owns its work as <actions>, each
// gated by its own <trigger> (a side_spell action replaces the old tick_spell; a manual_cast action
// replaces the old tick_handler). Collect the actions whose trigger fires on this pulse (kPulse always;
// kBattlePulse only in combat) and run ONE per tick, cycled by the affect's tick counter. Returns false
// if the affect has no pulse-triggered action (a passive affect -- nothing to do this tick).
static bool RunRoomTick(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	const ESpell impose = aff->type;
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
	EmitRoomTickMessage(ch, impose, phase);
	// Thread the affect's current duration in/out so a manual_cast handler (e.g. thunderstorm) can
	// branch on it -- and end the effect early by zeroing it.
	int dur = aff->duration;
	CastRoomTickActionFromActions(ch, room, impose, pulse, phase, &dur);
	aff->duration = dur;
	return true;
}

void HandleRoomAffect(RoomData *room, CharData *ch, const Affect<ERoomApply>::shared_ptr &aff) {
	assert(aff);
	assert(room);
	// issue.affect-migration: run the affect's pulse-triggered <actions> (if any). A passive affect
	// with no pulse-triggered action simply does nothing this tick.
	RunRoomTick(room, ch, aff);
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
						SendRemoveAffectMsgToRoom(affect->type, affect->affect_type, GetRoomRnum((*room)->vnum));
					}
				}
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
	const ERoomAffect want = RoomAffectBySpell(spell_id);   // issue.affect-migration: by affect_type
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
