// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_LOCAL_HPP_
#define _FIGHT_HIT_HPP_

#include "fight_constants.hpp"
#include "AffectHandler.hpp"
#include "utils.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

struct HitData
{
	HitData() : weapon(0), wielded(0), weapon_pos(WEAR_WIELD), weap_skill(SKILL_INVALID),
		weap_skill_is(0), skill_num(-1), hit_type(0), hit_no_parry(false),
		ch_start_pos(-1), victim_start_pos(-1), victim_ac(0), calc_thaco(0),
		dam(0), dam_critic(0)
	{
		diceroll = number(100, 2099) / 100;
	};

	// hit
	void init(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_base_hr(CHAR_DATA *ch);
	void calc_rand_hr(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_stat_hr(CHAR_DATA *ch);
	void calc_ac(CHAR_DATA *victim);
	void add_weapon_damage(CHAR_DATA *ch);
	void add_hand_damage(CHAR_DATA *ch);
	void check_defense_skills(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_crit_chance(CHAR_DATA *ch);
	double crit_backstab_multiplier(CHAR_DATA *ch, CHAR_DATA *victim);

	// extdamage
	int extdamage(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_mighthit_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_stupor_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void compute_critical(CHAR_DATA *ch, CHAR_DATA *victim);

	// init()
	// 1 - атака правой или двумя руками (RIGHT_WEAPON),
	// 2 - атака левой рукой (LEFT_WEAPON)
	int weapon;
	// пушка, которой в данный момент производится удар
	OBJ_DATA *wielded;
	// номер позиции (NUM_WEARS) пушки
	int weapon_pos;
	// номер скила, взятый из пушки или голых рук
	ESkill weap_skill;
	// очки скила weap_skill у чара, взятые через train_skill (могут быть сфейлены)
	int weap_skill_is;
	// брошенные кубики на момент расчета попадания
	int diceroll;
	// номер скила, пришедший из вызова hit(), может быть TYPE_UNDEFINED
	// в целом если < 0 - считается, что бьем простой атакой hit_type
	// если >= 0 - считается, что бьем скилом
	int skill_num;
	// тип удара пушкой или руками (attack_hit_text[])
	// инится в любом случае независимо от skill_num
	int hit_type;
	// true - удар не парируется/не блочится/не веерится и т.п.
	bool hit_no_parry;
	// позиция атакующего на начало атаки
	int ch_start_pos;
	// позиция жертвы на начало атаки
	int victim_start_pos;

	// высчитывается по мере сил
	// ац жертвы для расчета попадания
	int victim_ac;
	// хитролы атакующего для расчета попадания
	int calc_thaco;
	// дамаг атакующего
	int dam;
	// flags[CRIT_HIT] = true, dam_critic = 0 - критический удар
	// flags[CRIT_HIT] = true, dam_critic > 0 - удар точным стилем
	int dam_critic;

public:
	using flags_t = std::bitset<FightSystem::HIT_TYPE_FLAGS_NUM>;

	const flags_t& get_flags() const { return m_flags; }
	void set_flag(const size_t flag) { m_flags.set(flag); }
	void reset_flag(const size_t flag) { m_flags.reset(flag); }
	static void check_weap_feats(const CHAR_DATA* ch, int weap_skill, int& calc_thaco, int& dam);
private:
	// какой-никакой набор флагов, так же передается в damage()
	flags_t m_flags;
};

int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
void set_battle_pos(CHAR_DATA * ch);

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam);
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
bool check_valid_chars(CHAR_DATA *ch, CHAR_DATA *victim, const char *fname, int line);

void exthit(CHAR_DATA * ch, int type, int weapon);
void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon);

#endif // _FIGHT_LOCAL_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
