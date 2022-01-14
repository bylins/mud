#ifndef MAGIC_ROOMS_HPP_
#define MAGIC_ROOMS_HPP_

#include "magic/spells.h"
#include "entities/room.h"

#include <list>

class CHAR_DATA;

namespace room_spells {

using RoomAffectIt = ROOM_DATA::room_affects_list_t::iterator;

extern std::list<ROOM_DATA *> affected_rooms;

void room_affect_update();
RoomAffectIt FindAffect(ROOM_DATA *room, int type);
bool IsRoomAffected(ROOM_DATA *room, ESpell spell);
bool IsZoneRoomAffected(int zoneVNUM, ESpell spell);
void ShowAffectedRooms(CHAR_DATA *ch);
int ImposeSpellToRoom(int level, CHAR_DATA *ch, ROOM_DATA *room, int spellnum);
ROOM_DATA *FindAffectedRoom(long casterID, int spellnum);
int GetUniqueAffectDuration(long casterID, int spellnum);
void RemoveAffect(ROOM_DATA *room, const ROOM_DATA::room_affects_list_t::iterator &affect);

// Битвекторы аффектов комнат - порождаются заклинаниями и не сохраняются в файле.
enum EAffect : bitvector_t {
	kLight = 1 << 0,
	kPoisonFog = 1 << 1,
	kRuneLabel = 1 << 2,        // Комната помечена SPELL_MAGIC_LABEL //
	kForbidden = 1 << 3,        // Комната помечена SPELL_FORBIDDEN //
	kHypnoticPattern = 1 << 4,  // Комната под SPELL_HYPNOTIC_PATTERN //
	kBlackTentacles = 1 << 5, 	// Комната под SPELL_EVARDS_BLACK_TENTACLES //
	kMeteorstorm= 1 << 6,       // Комната под SPELL_METEORSTORM //
	kThunderstorm = 1 << 7      // SPELL_THUNDERSTORM
};

}

#endif // MAGIC_ROOMS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

