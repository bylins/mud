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

#include "spells_info.h"

#include <cstdlib>

class CharData;
class ObjData;
struct RoomData;

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

bool is_room_forbidden(RoomData *room);
void mobile_affect_update(void);
void player_affect_update(void);
void print_rune_log();
void show_spell_off(int aff, CharData *ch);

int CallMagicToGroup(int level, CharData *ch, int spellnum);
int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, int spellnum, int level);

int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, int spellnum, int level);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, int spellnum, int spell_subst);

int mag_damage(int level, CharData *ch, CharData *victim, int spellnum, ESaving savetype);
int mag_affects(int level, CharData *ch, CharData *victim, int spellnum, ESaving savetype);
int mag_summons(int level, CharData *ch, ObjData *obj, int spellnum, ESaving savetype);
int CastToPoints(int level, CharData *ch, CharData *victim, int spellnum, ESaving);
int CastUnaffects(int, CharData *ch, CharData *victim, int spellnum, ESaving);
int CastToAlterObjs(int, CharData *ch, ObjData *obj, int spellnum, ESaving);
int CastCreation(int, CharData *ch, int spellnum);
int CastToSingleTarget(int level, CharData *caster, CharData *cvict, ObjData *ovict, int spellnum, ESaving saving);

bool material_component_processing(CharData *caster, CharData *victim, int spellnum);
float CalcModCoef(int spellnum, int percent);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
