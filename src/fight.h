// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_H_
#define _FIGHT_H_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

enum { UNDEF_DMG, PHYS_DMG, MAGE_DMG };

/** fight.cpp */

void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim);
void stop_fighting(CHAR_DATA *ch, int switch_others);
void perform_violence();

/** fight_hit.cpp */

int compute_armor_class(CHAR_DATA *ch);
bool check_mighthit_weapon(CHAR_DATA *ch);
void apply_weapon_bonus(int ch_class, int skill, int *damroll, int *hitroll);

void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon);
int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int attacktype, bool mayflee, int dmg_type = PHYS_DMG);

/** fight_stuff.cpp */

void die(CHAR_DATA *ch, CHAR_DATA *killer);
void raw_kill(CHAR_DATA *ch, CHAR_DATA *killer);

void alterate_object(OBJ_DATA *obj, int dam, int chance);
void alt_equip(CHAR_DATA *ch, int pos, int dam, int chance);

void char_dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *victim, int attacktype, bool mayflee);
void test_self_hitroll(CHAR_DATA *ch);

#endif
