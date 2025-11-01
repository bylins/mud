#ifndef MAGIC_ROOMS_HPP_
#define MAGIC_ROOMS_HPP_

#include "gameplay/affects/affect_data.h"
#include "spells.h"

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

using RoomAffects = std::list<Affect<ERoomApply>::shared_ptr>;
using RoomAffectIt = RoomAffects::iterator;

extern std::list<RoomData *> affected_rooms;

void UpdateRoomsAffects();
void ShowAffectedRooms(CharData *ch);
void RoomRemoveAffect(RoomData *room, const RoomAffectIt &affect);
bool IsRoomAffected(RoomData *room, ESpell spell);
bool IsZoneRoomAffected(int zone_vnum, ESpell spell);
int CallMagicToRoom(int level, CharData *ch, RoomData *room, ESpell spell_id);
int GetUniqueAffectDuration(long caster_id, ESpell spell_id);
RoomAffectIt FindAffect(RoomData *room, ESpell type);
RoomData *FindAffectedRoomByCasterID(long caster_id, ESpell spell_id);
void AddRoomToAffected(RoomData *room);
void affect_room_join_fspell(RoomData *room, const Affect<ERoomApply> &af);
void affect_room_join(RoomData *room, Affect<ERoomApply> &af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void AffectRoomJoinReplace(RoomData *room, const Affect<ERoomApply> &af);
void affect_to_room(RoomData *room, const Affect<ERoomApply> &af);
void RemoveSingleAffectFromWorld(CharData *ch, ESpell spell_id);
void ProcessRoomAffectsOnEntry(CharData *ch, RoomRnum room);

} // namespace room_spells

#endif // MAGIC_ROOMS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

