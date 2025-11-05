// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_HIT_HPP_
#define _FIGHT_HIT_HPP_

#include "fight_constants.h"
#include "gameplay/affects/affect_handler.h"
#include "utils/utils.h"
#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"
#include "fight.h"

struct HitData {
	HitData() : weapon(fight::kMainHand), wielded(nullptr), weapon_pos(EEquipPos::kWield), weap_skill(ESkill::kUndefined),
				weap_skill_is(0), skill_num(ESkill::kUndefined), hit_type(0), hit_no_parry(false),
				ch_start_pos(EPosition::kUndefined), victim_start_pos(EPosition::kUndefined), victim_ac(0), calc_thaco(0),
				dam(0), dam_critic(0) {
		diceroll = number(100, 2099) / 100;
	};

	void Init(CharData *ch, CharData *victim);
	void CalcBaseHitroll(CharData *ch);
	void CalcCircumstantialHitroll(CharData *ch, CharData *victim);
	void CalcStaticHitroll(CharData *ch);
	void CalcAc(CharData *victim);
	void AddWeaponDmg(CharData *ch, bool need_dice = true);
	void AddBareHandsDmg(CharData *ch, bool need_dice = true);
	void ProcessDefensiveAbilities(CharData *ch, CharData *victim);
	void CalcCritHitChance(CharData *ch);
	int CalcDmg(CharData *ch, bool need_dice = true);
	int ProcessExtradamage(CharData *ch, CharData *victim);

	// init()
	// 1 - атака правой или двумя руками (RIGHT_WEAPON),
	// 2 - атака левой рукой (LEFT_WEAPON)
	fight::AttackType weapon;
	// пушка, которой в данный момент производится удар
	ObjData *wielded;
	// номер позиции (NUM_WEARS) пушки
	int weapon_pos;
	// номер скила, взятый из пушки или голых рук
	ESkill weap_skill;
	// очки скила weap_skill у чара, взятые через train_skill (могут быть сфейлены)
	int weap_skill_is;
	// брошенные кубики на момент расчета попадания
	int diceroll;
	// номер скила, пришедший из вызова hit(), может быть kUndefined
	// в целом если < 0 - считается, что бьем простой атакой hit_type
	// если >= 0 - считается, что бьем скилом
	ESkill skill_num;
	// тип удара пушкой или руками (attack_hit_text[])
	// инится в любом случае независимо от skill_num
	int hit_type;
	// true - удар не парируется/не блочится/не веерится и т.п.
	bool hit_no_parry;
	// позиция атакующего на начало атаки
	EPosition ch_start_pos;
	// позиция жертвы на начало атаки
	EPosition victim_start_pos;

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
	using HitFlags = std::bitset<fight::kHitFlagsNum>;

	[[nodiscard]] const HitFlags &GetFlags() const { return m_flags; }
	void SetFlag(const size_t flag) { m_flags.set(flag); }
	void ResetFlag(const size_t flag) { m_flags.reset(flag); }
	static void CheckWeapFeats(const CharData *ch, ESkill weap_skill, int &calc_thaco, int &dam);
 private:
	// какой-никакой набор флагов, так же передается в damage()
	HitFlags m_flags;

  [[nodiscard]] Damage GenerateExtradamage(int initial_dmg);
};

int CalcAC(CharData *ch);

int check_agro_follower(CharData *ch, CharData *victim);
void set_battle_pos(CharData *ch);

void gain_battle_exp(CharData *ch, CharData *victim, int dam);
void perform_group_gain(CharData *ch, CharData *victim, int members, int koef);
void group_gain(CharData *ch, CharData *victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
bool check_valid_chars(CharData *ch, CharData *victim, const char *fname, int line);

void ProcessExtrahits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon);
void hit(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon);

void Appear(CharData *ch);

int GetRealDamroll(CharData *ch);
int GetAutoattackDamroll(CharData *ch, int weapon_skill);

#endif // _FIGHT_HIT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
