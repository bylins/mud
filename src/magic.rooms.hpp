#ifndef __MAGIC_ROOMS_HPP__
#define __MAGIC_ROOMS_HPP__

// яРПСЙРСПШ Х ТСМЙЖХХ ДКЪ ПЮАНРШ Я ГЮЙКХМЮМХЪЛХ, НАЙЮЯРНБШБЮЧЫХЛХ ЙНЛМЮРШ

#include "spells.h"
#include "room.hpp"

#include <list>

class CHAR_DATA;

namespace RoomSpells {

	using RoomAffectListIt = ROOM_DATA::room_affects_list_t::iterator;

	extern std::list<ROOM_DATA*> aff_room_list;

	void room_affect_update(void);
	RoomAffectListIt findRoomAffect(ROOM_DATA* room, int type);
	bool isRoomAffected(ROOM_DATA * room, ESpell spell);
	bool isZoneRoomAffected(int zoneVNUM, ESpell spell);
	void showAffectedRooms(CHAR_DATA *ch);
	int imposeSpellToRoom(int level, CHAR_DATA * ch , ROOM_DATA * room, int spellnum);
	ROOM_DATA * findAffectedRoom(long casterID, int spellnum);
	int getUniqueAffectDuration(long casterID, int spellnum);
	void removeAffectFromRoom(ROOM_DATA* room, const ROOM_DATA::room_affects_list_t::iterator& affect);
}

#endif //__MAGIC_ROOMS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

