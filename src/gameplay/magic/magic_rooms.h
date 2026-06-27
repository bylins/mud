#ifndef MAGIC_ROOMS_HPP_
#define MAGIC_ROOMS_HPP_

#include "gameplay/affects/affect_data.h"
#include "spells.h"
#include "magic.h"   // ECastResult / CastContext
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
	kCount = 11,
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
ECastResult CallMagicToRoom(CharData *ch, RoomData *room, CastContext roll);
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
ECastResult CastRoomAffect(CastContext &ctx);
void RemoveSingleAffectFromWorld(CharData *ch, ERoomAffect affect);
void ProcessRoomAffectsOnEntry(CharData *ch, RoomRnum room);

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

// issue.affect-migration: the spell whose enum token matches a room affect (inverse of
// RoomAffectBySpell); kUndefined if none. For paths that still need an ESpell from a room affect.
[[nodiscard]] ESpell SpellByRoomAffect(ERoomAffect affect_type);

// issue.affect-migration: per-ERoomAffect behavior flags from room_affects.xml (0 if none) + load gate.
[[nodiscard]] Bitvector RoomAffectFlagsByType(ERoomAffect affect_type);
[[nodiscard]] bool RoomAffectFlagsLoaded();
// issue.affect-migration: the affect's own <actions> (each gated by its <trigger>); empty when none.
[[nodiscard]] const talents_actions::Actions &RoomAffectActions(ERoomAffect affect_type);

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

