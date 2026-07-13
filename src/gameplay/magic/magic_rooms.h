#ifndef MAGIC_ROOMS_HPP_
#define MAGIC_ROOMS_HPP_

#include "gameplay/affects/affect_data.h"
#include "spells.h"
#include "magic.h"   // ECastResult / ActionContext
#include "gameplay/abilities/talents_actions.h"  // TalentAffect::Apply (RoomAffectSeal return type)
#include "engine/structs/meta_enum.h"   // NAME_BY_ITEM / ITEM_BY_NAME / NAMES_OF

#include <list>

class CharData;

namespace room_spells {

// Константа, определяющая скорость таймера аффектов
const int kSecsPerRoomAffect = 2;

// Room-affect identity flags. Index-valued (BitsetFlags-ready, like EAffect): kUndefined=0 is the
// "no flag" sentinel (== the old static_cast<ERoomAffect>(0)); real room affects are 1-based, one
// enumerator per room-affecting spell, plus kNoPortalExit (a one-way sub-marker on a portal affect).
// kCount is the terminal counter BitsetFlags<ERoomAffect> reads via flag_traits.
enum class ERoomAffect : Bitvector {
	kUndefined = 0,
	kNoPortalExit = 1,			// портал без входа/выхода
	kForbidden = 2,
	kMeteorStorm = 3,
	kRoomLight = 4,
	kDeadlyFog = 5,
	kThunderstorm = 6,
	kRuneLabel = 7,
	kHypnoticPattern = 8,
	kBlackTentacles = 9,
	kPortalTimer = 10,
	kFireTrap = 11,			// issue.room-affect-trigger-improve: door affect -- kOpen trigger fireballs the opener
	kCount = 12,
};

// issue.affect-migration: when a room affect runs each of its <actions> is now decided per-action by
// the action's own <trigger> (talents_actions::EActionTrigger: kPulse / kBattlePulse), parsed by
// Actions::Build. There is no per-affect trigger set anymore.

// Эффекты для комнат //
enum ERoomApply {
	kNone = 0
};

} // namespace room_spells

// Room affects store an ERoomAffect flag in Affect::affect_type (see affect_data.h).
template<>
struct AffectFlagType<room_spells::ERoomApply> {
	using type = room_spells::ERoomAffect;
};

namespace room_spells {

using RoomAffects = std::list<Affect<ERoomApply>::shared_ptr>;
using RoomAffectIt = RoomAffects::iterator;

extern std::list<RoomData *> affected_rooms;

void UpdateRoomsAffects();
void ShowAffectedRooms(CharData *ch);
void RoomRemoveAffect(RoomData *room, const RoomAffectIt &affect);
bool IsRoomAffected(RoomData *room, ERoomAffect affect);
bool IsZoneRoomAffected(int zone_vnum, ERoomAffect affect);
ECastResult CallMagicToRoom(CharData *ch, RoomData *room, ActionContext roll);
// issue.room-affect-trigger-improve (door affects): cast a kMagRoom spell onto the exit in `dir`.
ECastResult CallMagicToExit(CharData *ch, int dir, ActionContext roll);
int GetUniqueAffectDuration(long caster_id, ERoomAffect affect);
RoomAffectIt FindAffect(RoomData *room, ERoomAffect affect);
// issue.affect-migration: the portal room affects (two-way kPortalTimer + one-way kNoPortalExit) keyed
// by ERoomAffect identity (their ESpell was retired). IsPortalAffect matches either; RoomHasPortal =
// any portal present; FindPortalAffect = first portal affect (or affected.end()).
[[nodiscard]] bool IsPortalAffect(ERoomAffect affect_type);
[[nodiscard]] bool RoomHasPortal(RoomData *room);
[[nodiscard]] RoomAffectIt FindPortalAffect(RoomData *room);
RoomData *FindAffectedRoomByCasterID(long caster_id, ERoomAffect affect);
void AddRoomToAffected(RoomData *room);
void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_join(RoomData *room, Affect<ERoomApply> &af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void AffectRoomJoinReplace(RoomData *room, const Affect<ERoomApply> &af);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);
// Impose the spell's room affect from the current action; callable from the per-action loop.
ECastResult CastRoomAffect(ActionContext &ctx);
void RemoveSingleAffectFromWorld(CharData *ch, ERoomAffect affect);
void ProcessRoomAffectsOnEntry(CharData *ch, RoomRnum room);

// issue.room-affect-trigger-improve: the generic on-entry trigger dispatcher runs in one of three
// phases so a blocking trigger can refuse a voluntary move (DGScript ENTER-style) WITHOUT moving a
// non-blocking affect's effect out of its destination-room context:
//   kBlockCheck         -- BEFORE placement on the walk path: run only blocking-capable actions
//                          (those with a <trigger return=> tag or a manual_cast handler) and honour
//                          return=0; returns false to refuse the move.
//   kEffectsNonBlocking -- AFTER placement on the walk path: run the remaining (pure-effect) actions
//                          in the destination room; the return is ignored.
//   kEffectsAll         -- AFTER placement on a FORCED entry (teleport/summon/flee): run every action
//                          (the block verdict is ignored -- forced entries can't be blocked).
enum class EEntryTriggerPhase { kBlockCheck, kEffectsNonBlocking, kEffectsAll };
[[nodiscard]] bool RunRoomEntryTriggers(CharData *actor, RoomData *room, EEntryTriggerPhase phase);
// issue.room-affect-trigger-improve (door affects): fire a door affect's pick/unlock/open triggers
// (`ev`) for `actor` on the exit `dir` of `room`. Returns false if a trigger returns 0 (refuse the
// door action), true otherwise.
[[nodiscard]] bool RunDoorTriggers(CharData *actor, RoomData *room, int dir, talents_actions::EActionTrigger ev);
// issue.room-affect-trigger-improve (door affects): clear affects on exit `dir` of `room` + drop its
// registry entry. Call when an exit is rebuilt/torn down (redit, dungeon reuse, DGScript wexit).
void ClearExitAffects(RoomData *room, int dir);
// issue.room-affect-trigger-improve (door affects): dispel dispellable affects on exit `dir` from
// `caster` (the <unaffect> stage of a kTarDirection cast). Reverse-resolved + author/ally/strength
// contest like the room dispel. Returns true if anything was dispelled (caller owns messaging).
bool DispelExitAffects(CharData *caster, int dir, ESpell spell_id);

// Walks room->affected for a kPortalTimer affect with pk_unique != 0 and
// pk_unique != exclude_uid; returns the matching pk_unique, or 0 if none.
// Replaces the per-room RoomData::pkPenterUnique field. exclude_uid = 0
// (the default) treats every PK pentagram as a match -- use that for the
// "is there ANY PK pentagram here?" question (do_enter arena/house gate).
// Pass the viewer's uid to find only foreign PK pentagrams -- use that for
// the "did someone else cast this?" question (do_enter entry penalty).
long FindRoomPkPortalUid(RoomData *room, long exclude_uid = 0);

// issue.dispellbug: relationship of an actor to an existing room affect.
// `free` = actor is the affect's author or a live in-world ally (acts with no
// strength contest); `author` = the live in-world author (for PK flagging) or
// nullptr when the author is offline / gone.
struct RoomAffectActor {
	bool free;
	CharData *author;
};
RoomAffectActor ClassifyRoomAffectAccess(CharData *ch, long caster_id);

// issue.affect-migration: per-ERoomAffect behavior flags from room_affects.xml (0 if none) + load gate.
[[nodiscard]] Bitvector RoomAffectFlagsByType(ERoomAffect affect_type);
[[nodiscard]] bool RoomAffectFlagsLoaded();
// issue.affects-improve: per-affect seal-strength cap from room_affects.xml (0 if none).
[[nodiscard]] int RoomAffectSealCap(ERoomAffect affect_type);
// issue.affects-improve (P1 fix): the room affect's seal-strength FORMULA (room_affects.xml
// <seal_strength>). RoomAffectHasSeal marks rows that carry one; CallMagicToRoom evaluates it via
// ComputeApplyModifier so the affect's strength is its own property, not the casting spell's apply.
[[nodiscard]] bool RoomAffectHasSeal(ERoomAffect affect_type);
[[nodiscard]] const talents_actions::TalentAffect::Apply &RoomAffectSeal(ERoomAffect affect_type);
// issue.affect-migration: the affect's own <actions> (each gated by its <trigger>); empty when none.
[[nodiscard]] const talents_actions::Actions &RoomAffectActions(ERoomAffect affect_type);

// issue.character-affect-triggers: fire a room/exit affect's <actions> matching `trig` (kExpired on
// timer end, kDispell on forced removal) with `ch` as the caster context and `potency` as the cast
// strength. No-op (returns false) if the affect has no matching action or `ch` is null -- room affects
// have no bearer char, so they need a live caster to source the cast (the affect's caster for kExpired;
// the dispeller for kDispell, which makes the retaliation self-inflicted). Defined in magic_rooms.cpp.
bool RunRoomAffectTrigger(RoomData *room, CharData *ch, ERoomAffect affect_type,
						  talents_actions::EActionTrigger trig, float potency);

} // namespace room_spells

// Room-affect registry name maps (cfg/room_affects.xml validates these). Global scope, mirroring
// the EAffect specializations in affect_contants.h.
template<>
const std::string &NAME_BY_ITEM<room_spells::ERoomAffect>(room_spells::ERoomAffect item);
template<>
room_spells::ERoomAffect ITEM_BY_NAME<room_spells::ERoomAffect>(const std::string &name);
template<>
const std::map<room_spells::ERoomAffect, std::string> &NAMES_OF<room_spells::ERoomAffect>();

#endif // MAGIC_ROOMS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

