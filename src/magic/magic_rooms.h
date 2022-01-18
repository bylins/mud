#ifndef MAGIC_ROOMS_HPP_
#define MAGIC_ROOMS_HPP_

#include "affects/affect_data.h"
#include "magic/spells.h"

#include <list>

class CHAR_DATA;

namespace room_spells {

// Битвекторы аффектов комнат - порождаются заклинаниями и не сохраняются в файле.
enum ERoomAffect : bitvector_t {
	kLight = 1 << 0,
	kPoisonFog = 1 << 1,
	kRuneLabel = 1 << 2,        // Комната помечена SPELL_MAGIC_LABEL //
	kForbidden = 1 << 3,        // Комната помечена SPELL_FORBIDDEN //
	kHypnoticPattern = 1 << 4,  // Комната под SPELL_HYPNOTIC_PATTERN //
	kBlackTentacles = 1 << 5, 	// Комната под SPELL_EVARDS_BLACK_TENTACLES //
	kMeteorstorm= 1 << 6,       // Комната под SPELL_METEORSTORM //
	kThunderstorm = 1 << 7      // SPELL_THUNDERSTORM
};

// Эффекты для комнат //
enum ERoomApply {
	kNone = 0,
	kPoison,						// Изменяет в комнате уровень ядности //
	kFlame [[maybe_unused]],		// Изменяет в комнате уровень огня (для потомков) //
	kNumApplies [[maybe_unused]]
};

using RoomAffects = std::list<AFFECT_DATA<ERoomApply>::shared_ptr>;
using RoomAffectIt = RoomAffects::iterator;

extern std::list<ROOM_DATA *> affected_rooms;

void UpdateRoomsAffects();
void ShowAffectedRooms(CHAR_DATA *ch);
void RemoveAffect(ROOM_DATA *room, const RoomAffectIt &affect);
bool IsRoomAffected(ROOM_DATA *room, ESpell spell);
bool IsZoneRoomAffected(int zone_vnum, ESpell spell);
int ImposeSpellToRoom(int level, CHAR_DATA *ch, ROOM_DATA *room, int spellnum);
int GetUniqueAffectDuration(long caster_id, int spellnum);
RoomAffectIt FindAffect(ROOM_DATA *room, int type);
ROOM_DATA *FindAffectedRoom(long caster_id, int spellnum);

}

#endif // MAGIC_ROOMS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

