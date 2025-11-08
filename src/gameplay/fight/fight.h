// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_H_
#define _FIGHT_H_

#include "fight_constants.h"
#include "engine/entities/char_data.h"
#include "engine/structs/structs.h"
#include "engine/core/conf.h"
#include "engine/core/sysdep.h"

int set_hit(CharData *ch, CharData *victim);
void SetFighting(CharData *ch, CharData *vict);
inline void set_fighting(const CharData::shared_ptr &ch, CharData *victim) { SetFighting(ch.get(), victim); }

void stop_fighting(CharData *ch, int switch_others);
void perform_violence();
int calc_initiative(CharData *ch, bool mode);

int CalcBaseAc(CharData *ch);
void GetClassWeaponMod(ECharClass class_id, const ESkill skill, int *damroll, int *hitroll);

void die(CharData *ch, CharData *killer);
void raw_kill(CharData *ch, CharData *killer);
void update_pos(CharData *victim);

void char_dam_message(int dam, CharData *ch, CharData *victim, bool mayflee);
void TestSelfHitroll(CharData *ch);
int check_agro_follower(CharData *ch, CharData *victim);
bool check_valid_chars(CharData *ch, CharData *victim, const char *fname, int line);
void perform_group_gain(CharData *ch, CharData *victim, int members, int koef);
void group_gain(CharData *ch, CharData *victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
ObjData *GetUsedWeapon(CharData *ch, fight::AttackType AttackType);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
