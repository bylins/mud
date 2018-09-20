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

// These mobiles do not exist.
#define MOB_DOUBLE        3000 //внум прототипа для клона
#define MOB_SKELETON      3001
#define MOB_ZOMBIE        3002
#define MOB_BONEDOG       3003
#define MOB_BONEDRAGON    3004
#define MOB_BONESPIRIT    3005
#define MOB_SWAMPLIGHT    3006
#define MOB_BANSHEE       3007
#define MOB_LICH          3008
#define LAST_NECR_MOB	  3008
#define MOB_KEEPER        104
#define MOB_FIREKEEPER    105
#define MOB_MENTAL_SHADOW 3020

#define MAX_SPELL_AFFECTS 16	// change if more needed

//таймеры для спеллов, которые должны тикать, только если кастер помер или вышел
#define TIME_SPELL_RUNE_LABEL 300

#define SpINFO spell_info[spellnum]

int get_resist_type(int spellnum);
void delete_from_tmp_char_list(CHAR_DATA *ch);
bool is_room_forbidden(ROOM_DATA * room);
int check_recipe_items(CHAR_DATA * ch, int spellnum, int spelltype, int extract, const CHAR_DATA * targ = NULL);
void mobile_affect_update(void);
void player_affect_update(void);
void print_rune_log();

namespace RoomSpells
{
	void room_affect_update(void);
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
