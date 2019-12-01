/* ************************************************************************
*   File: magic.h                                     Part of Bylins      *
*  Usage: header: low-level functions for magic; spell template code      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#ifndef _MAGIC_H_
#define _MAGIC_H_

#include <cstdlib>

class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

// VNUM'ы мобов для заклинаний, создающих мобов
const int MOB_DOUBLE = 3000; //внум прототипа для клона
const int MOB_SKELETON = 3001;
const int MOB_ZOMBIE = 3002;
const int MOB_BONEDOG = 3003;
const int MOB_BONEDRAGON = 3004;
const int MOB_BONESPIRIT = 3005;
const int MOB_NECRODAMAGER = 3007;
const int MOB_NECROTANK = 3008;
const int MOB_NECROBREATHER = 3009;
const int MOB_NECROCASTER = 3010;
const int LAST_NECRO_MOB = 3010;
// резерв для некротических забав
const int MOB_MENTAL_SHADOW = 3020;
const int MOB_KEEPER = 3021;
const int MOB_FIREKEEPER = 3022;

const int MAX_SPELL_AFFECTS = 16; // change it if you need more

//таймеры для спеллов, которые должны тикать, только если кастер помер или вышел
const int TIME_SPELL_RUNE_LABEL = 300;

#define SpINFO spell_info[spellnum]

bool is_room_forbidden(ROOM_DATA * room);
int check_recipe_items(CHAR_DATA * ch, int spellnum, int spelltype, int extract, const CHAR_DATA * targ = NULL);
void mobile_affect_update(void);
void player_affect_update(void);
void print_rune_log();

namespace RoomSpells {
	void room_affect_update(void);
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
