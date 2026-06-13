#ifndef MAGIC_ROOMS_HPP_
#define MAGIC_ROOMS_HPP_

#include "gameplay/affects/affect_data.h"
#include "spells.h"
#include "magic.h"   // ECastResult / CastContext

#include <list>

class CharData;

namespace room_spells {

// Константа, определяющая скорость таймера аффектов
const int kSecsPerRoomAffect = 2;

enum ERoomAffect : Bitvector {
	kNoPortalExit = 1 << 0		// портал без входа\выхода
};


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
bool IsRoomAffected(RoomData *room, ESpell spell);
bool IsZoneRoomAffected(int zone_vnum, ESpell spell);
ECastResult CallMagicToRoom(CharData *ch, RoomData *room, CastContext roll);
int GetUniqueAffectDuration(long caster_id, ESpell spell_id);
RoomAffectIt FindAffect(RoomData *room, ESpell type);
RoomData *FindAffectedRoomByCasterID(long caster_id, ESpell spell_id);
void AddRoomToAffected(RoomData *room);
void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_join(RoomData *room, Affect<ERoomApply> &af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void AffectRoomJoinReplace(RoomData *room, const Affect<ERoomApply> &af);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);
// Impose the spell's room affect from the current action; callable from the per-action loop.
ECastResult CastRoomAffect(CastContext &ctx);
void RemoveSingleAffectFromWorld(CharData *ch, ESpell spell_id);
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

} // namespace room_spells

#endif // MAGIC_ROOMS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

