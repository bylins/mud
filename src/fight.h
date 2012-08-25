// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_H_
#define _FIGHT_H_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

enum { UNDEF_DMG, PHYS_DMG, MAGE_DMG };

// attack_hit_text[]
enum
{
	type_hit, type_skin, type_whip, type_slash, type_bite,
	type_bludgeon, type_crush, type_pound, type_claw, type_maul,
	type_thrash, type_pierce, type_blast, type_punch, type_stab,
	type_pick, type_sting
};

const int IGNORE_SANCT = 0;
const int IGNORE_PRISM = 1;
const unsigned HIT_TYPE_FLAGS_NUM = 2;

/**
 * Для входа со скила без инита остальных полей:
 * damage(ch, victim, dam, SkillDmg(SKILL_NUM), mayflee);
 */
struct SkillDmg
{
	SkillDmg(int num) : skill_num(num) {};
	int skill_num;
};

/**
 * Для входа с закла без инита остальных полей:
 * damage(ch, victim, dam, SpellDmg(SPELL_NUM), mayflee);
 */
struct SpellDmg
{
	SpellDmg(int num) : spell_num(num) {};
	int spell_num;
};

/**
 * Для входа с необычного дамага без инита остальных полей (инится сразу номер messages):
 * damage(ch, victim, dam, SimpleDmg(TYPE_NUM), mayflee);
 */
struct SimpleDmg
{
	SimpleDmg(int num) : msg_num(num) {};
	int msg_num;
};

struct DmgType
{
	DmgType()
		: skill_num(-1), spell_num(-1), msg_num(-1) { zero_init(); };
	DmgType(SkillDmg &obj)
		: skill_num(obj.skill_num), spell_num(-1), msg_num(-1) { zero_init(); };
	DmgType(SpellDmg &obj)
		: skill_num(-1), spell_num(obj.spell_num), msg_num(-1) { zero_init(); };
	DmgType(SimpleDmg &obj)
		: skill_num(-1), spell_num(-1), msg_num(obj.msg_num) { zero_init(); };

	void zero_init();
	void init_msg_num();

	int damage(CHAR_DATA *ch, CHAR_DATA *victim);
	bool magic_shields_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	bool armor_dam_reduce(CHAR_DATA *ch, CHAR_DATA *victim);
	void process_death(CHAR_DATA *ch, CHAR_DATA *victim);

	// дамаг атакующего
	int dam;
	// was_critic = TRUE, dam_critic = 0 - критический удар
	// was_critic = TRUE, dam_critic > 0 - удар точным стилем
	int was_critic;
	int dam_critic;
	// обратный дамаг от огненного щита
	int fs_damage;
	// можно ли сбежать
	bool mayflee;
	// тип урона (физ/маг/обычный)
	int dmg_type;
	// см. описание в HitData
	int skill_num;
	// номер заклинания, если >= 0
	int spell_num;
	// см. описание в HitData, но здесь может быть -1
	int hit_type;
	// номер сообщения об ударе из файла messages
	// инится только начиная с вызова damage
	int msg_num;
	// набор флагов из HitType
	std::bitset<HIT_TYPE_FLAGS_NUM> flags;
	// временно из HitType
	int skill_noparryhit_dam;
};

template<class T> int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, T obj,
	bool mayflee, int dmg_type = PHYS_DMG)
{
	DmgType dmg(obj);
	dmg.dam = dam;
	dmg.mayflee = mayflee;
	dmg.dmg_type = dmg_type;
	return dmg.damage(ch, victim);
};

/** fight.cpp */

void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim);
void stop_fighting(CHAR_DATA *ch, int switch_others);
void perform_violence();

/** fight_hit.cpp */

int compute_armor_class(CHAR_DATA *ch);
bool check_mighthit_weapon(CHAR_DATA *ch);
void apply_weapon_bonus(int ch_class, int skill, int *damroll, int *hitroll);

void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon);

/** fight_stuff.cpp */

void die(CHAR_DATA *ch, CHAR_DATA *killer);
void raw_kill(CHAR_DATA *ch, CHAR_DATA *killer);

void alterate_object(OBJ_DATA *obj, int dam, int chance);
void alt_equip(CHAR_DATA *ch, int pos, int dam, int chance);

void char_dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *victim, bool mayflee);
void test_self_hitroll(CHAR_DATA *ch);

#endif
