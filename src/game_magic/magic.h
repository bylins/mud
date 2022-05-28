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
void ShowAffExpiredMsg(ESpell aff_type, CharData *ch);

int CallMagicToGroup(int level, CharData *ch, ESpell spell_id);
int CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, ESpell spell_id, int level);

int CallMagic(CharData *caster, CharData *cvict, ObjData *ovict, RoomData *rvict, ESpell spell_id, int level);
int CastSpell(CharData *ch, CharData *tch, ObjData *tobj, RoomData *troom, ESpell spell_id, ESpell spell_subst);

int CastDamage(int level, CharData *ch, CharData *victim, ESpell spell_id);
int CastAffect(int level, CharData *ch, CharData *victim, ESpell spell_id);
int mag_summons(int level, CharData *ch, ObjData *obj, ESpell spell_id, ESaving savetype);
int CastToPoints(int level, CharData *ch, CharData *victim, ESpell spell_id);
int CastUnaffects(int, CharData *ch, CharData *victim, ESpell spell_id);
int CastToAlterObjs(int, CharData *ch, ObjData *obj, ESpell spell_id);
int CastCreation(int, CharData *ch, ESpell spell_id);
int CastToSingleTarget(int level, CharData *caster, CharData *cvict, ObjData *ovict, ESpell spell_id);

bool ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id);
float CalcModCoef(ESpell spell_id, int percent);
int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
