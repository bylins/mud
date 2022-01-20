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

#include "magic/spells_info.h"

#include <cstdlib>

class CHAR_DATA;
class OBJ_DATA;
class ROOM_DATA;

// VNUM'ы мобов для заклинаний, создающих мобов
const int kMobDouble = 3000; //внум прототипа для клона
const int kMobSkeleton = 3001;
const int kMobZombie = 3002;
const int kMobBonedog = 3003;
const int kMobBonedragon = 3004;
const int kMobBonespirit = 3005;
const int kMobNecrodamager = 3007;
const int kMobNecrotank = 3008;
const int kMobNecrobreather = 3009;
const int kMobNecrocaster = 3010;
const int kLastNecroMob = 3010;
// резерв для некротических забав
const int kMobMentalShadow = 3020;
const int kMobKeeper = 3021;
const int kMobFirekeeper = 3022;

const int kMaxSpellAffects = 16; // change it if you need more

#define SpINFO spell_info[spellnum]

bool is_room_forbidden(ROOM_DATA *room);
void mobile_affect_update(void);
void player_affect_update(void);
void print_rune_log();
void show_spell_off(int aff, CHAR_DATA *ch);

int callMagicToGroup(int level, CHAR_DATA *ch, int spellnum);
int callMagicToArea(CHAR_DATA *ch, CHAR_DATA *victim, ROOM_DATA *room, int spellnum, int level);

int CallMagic(CHAR_DATA *caster, CHAR_DATA *cvict, OBJ_DATA *ovict, ROOM_DATA *rvict, int spellnum, int level);
int CastSpell(CHAR_DATA *ch, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom, int spellnum, int spell_subst);

int mag_damage(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype);
int mag_affects(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype);
int mag_summons(int level, CHAR_DATA *ch, OBJ_DATA *obj, int spellnum, int savetype);
int mag_points(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype);
int mag_unaffects(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int type);
int mag_alter_objs(int level, CHAR_DATA *ch, OBJ_DATA *obj, int spellnum, int type);
int mag_creations(int level, CHAR_DATA *ch, int spellnum);
int mag_single_target(int level, CHAR_DATA *caster, CHAR_DATA *cvict, OBJ_DATA *ovict, int spellnum, int casttype);

bool material_component_processing(CHAR_DATA *caster, CHAR_DATA *victim, int spellnum);
float func_koef_duration(int spellnum, int percent); 
float func_koef_modif(int spellnum, int percent);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
