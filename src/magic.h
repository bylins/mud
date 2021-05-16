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

#include "obj.hpp"
#include "chars/character.h"
#include "room.hpp"
#include "spells.info.h"

#include <cstdlib>

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

#define SpINFO spell_info[spellnum]

bool is_room_forbidden(ROOM_DATA * room);
void mobile_affect_update(void);
void player_affect_update(void);
void print_rune_log();
void show_spell_off(int aff, CHAR_DATA * ch);

int callMagicToGroup(int level, CHAR_DATA * ch, int spellnum);
int callMagicToArea(CHAR_DATA* ch, CHAR_DATA* victim, ROOM_DATA* room, int spellnum, int level);

int call_magic(CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, ROOM_DATA *rvict, int spellnum, int level);
int cast_spell(CHAR_DATA * ch, CHAR_DATA * tch, OBJ_DATA * tobj, ROOM_DATA *troom, int spellnum, int spell_subst);

int mag_damage(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype);
int mag_affects(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype);
int mag_summons(int level, CHAR_DATA * ch, OBJ_DATA * obj, int spellnum, int savetype);
int mag_points(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype);
int mag_unaffects(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int type);
int mag_alter_objs(int level, CHAR_DATA * ch, OBJ_DATA * obj, int spellnum, int type);
int mag_creations(int level, CHAR_DATA * ch, int spellnum);
int mag_single_target(int level, CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, int spellnum, int casttype);

bool material_component_processing(CHAR_DATA *caster, CHAR_DATA *victim, int spellnum);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
