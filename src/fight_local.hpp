// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_LOCAL_HPP_
#define _FIGHT_LOCAL_HPP_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "AffectHandler.hpp"

#define RIGHT_WEAPON 1
#define LEFT_WEAPON  2

//Polud функция, вызывающая обработчики аффектов, если они есть
template <class S> void handle_affects( S& params ) //тип params определяется при вызове функции
{
	AFFECT_DATA* aff;
	for (aff=params.ch->affected; aff; aff = aff->next)
	{
		if (aff->handler)
			aff->handler->Handle(params); //в зависимости от типа params вызовется нужный Handler
	}
}

struct HitType
{
	HitType() : weapon(0), wielded(0), weapon_pos(WEAR_WIELD), weap_skill(0),
		weap_skill_is(0), type(0), w_type(0), victim_ac(0), calc_thaco(0),
		dam(0), noparryhit(0), was_critic(0), dam_critic(0)
	{
		diceroll = number(100, 2099) / 100;
	};

	/** hit */
	void init(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_base_hr(CHAR_DATA *ch);
	void calc_rand_hr(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_ac(CHAR_DATA *victim);
	void add_weapon_damage(CHAR_DATA *ch);
	void add_hand_damage(CHAR_DATA *ch);
	void check_defense_skills(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_crit_chance(CHAR_DATA *ch);
	void check_weap_feats(CHAR_DATA *ch);

	/** extdamage */
	int extdamage(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_mighthit_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_stupor_dam(CHAR_DATA *ch, CHAR_DATA *victim);

	/** init() */
	// 1 - атака правой или двумя руками (RIGHT_WEAPON),
	// 2 - атака левой рукой (LEFT_WEAPON)
	int weapon;
	// пушка, которой в данный момент производится удар
	OBJ_DATA *wielded;
	// номер позиции (NUM_WEARS) пушки
	int weapon_pos;
	// номер скила, взятый из пушки или голых рук
	int weap_skill;
	// кол-во очков скила skill у чара
	int weap_skill_is;
	// брошенные кубики на момент расчета попадания
	int diceroll;
	// номер скила атаки, пришедший из вызова hit()
	// проверяется на TYPE_UNDEFINED и TYPE_NOPARRY, в extdamage не идет
	int type;
	// type + TYPE_HIT для вывода сообщений об ударе и еще хрен знает чего
	int w_type;

	/** высчитывается по мере сил */
	// ац жертвы для расчета попадания
	int victim_ac;
	// хитролы атакующего для расчета попадания
	int calc_thaco;
	// дамаг атакующего
	int dam;
	// дамаг от скрытого стиля, который потом плюсуется к dam
	int noparryhit;
	// was_critic = TRUE, dam_critic = 0 - критический удар
	// was_critic = TRUE, dam_critic > 0 - удар точным стилем
	int was_critic;
	int dam_critic;
};

struct DmgType
{
	DmgType() : w_type(0), dam(0), was_critic(0), dam_critic(0),
		fs_damage(0), mayflee(true), dmg_type(PHYS_DMG) {};

	int damage(CHAR_DATA *ch, CHAR_DATA *victim);
	bool magic_shields_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	bool armor_dam_reduce(CHAR_DATA *ch, CHAR_DATA *victim);
	void compute_critical(CHAR_DATA *ch, CHAR_DATA *victim);

	// type + TYPE_HIT для вывода сообщений об ударе и еще хрен знает чего
	int w_type;
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
};

/** fight.cpp */

int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
void set_battle_pos(CHAR_DATA * ch);

/** fight_hit.cpp */

int calc_leadership(CHAR_DATA * ch);
void exthit(CHAR_DATA * ch, int type, int weapon);

/** fight_stuff.cpp */

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam);
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
bool check_valid_chars(CHAR_DATA *ch, CHAR_DATA *victim, const char *fname, int line);

#endif // _FIGHT_LOCAL_HPP_
