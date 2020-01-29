// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_H_
#define _FIGHT_H_

#include "fight_constants.hpp"
#include "char.hpp"
#include "structs.h"
#include "conf.h"
#include "sysdep.h"

/**
 * Для входа со скила без инита остальных полей:
 * Damage obj(SkillDmg(SKILL_NUM), dam, FightSystem::UNDEF_DMG|PHYS_DMG|MAGE_DMG)
 * obj.process(ch, victim);
 */
struct SkillDmg
{
	SkillDmg(int num) : skill_num(num) {};
	int skill_num;
};

/**
 * Для входа с закла без инита остальных полей:
 * Damage obj(SpellDmg(SPELL_NUM), dam, FightSystem::UNDEF_DMG|PHYS_DMG|MAGE_DMG)
 * obj.process(ch, victim);
 */
struct SpellDmg
{
	SpellDmg(int num) : spell_num(num) {};
	int spell_num;
};

/**
 * Для входа с необычного дамага без инита остальных полей (инится сразу номер messages):
 * Damage obj(SimpleDmg(TYPE_NUM), dam, FightSystem::UNDEF_DMG|PHYS_DMG|MAGE_DMG)
 * obj.process(ch, victim);
 */
struct SimpleDmg
{
	SimpleDmg(int num) : msg_num(num) {};
	int msg_num;
};

/**
 * Процесс инициализации, обязательные поля:
 *   dam - величина урона
 *   msg_num - напрямую или через skill_num/spell_num/hit_type
 *   dmg_type - UNDEF_DMG/PHYS_DMG/MAGE_DMG
 * Остальное по необходимости:
 *   ch_start_pos - если нужны модификаторы урона от позиции атакующего (физ дамаг)
 *   victim_start_pos - если нужны модификаторы урона от позиции жертвы (физ/маг дамаг)
 */
class Damage
{
public:
	// полностью ручное создание объекта
	Damage() { zero_init(); };

	// скилы
	Damage(SkillDmg obj, int in_dam, FightSystem::DmgType in_dmg_type)
	{
		zero_init();
		skill_num = obj.skill_num;
		dam = in_dam;
		dmg_type = in_dmg_type;
	};

	// заклинания
	Damage(SpellDmg obj, int in_dam, FightSystem::DmgType in_dmg_type)
	{
		zero_init();
		spell_num = obj.spell_num;
		dam = in_dam;
		dmg_type = in_dmg_type;
	};

	// прочий дамаг
	Damage(SimpleDmg obj, int in_dam, FightSystem::DmgType in_dmg_type)
	{
		zero_init();
		msg_num = obj.msg_num;
		dam = in_dam;
		dmg_type = in_dmg_type;
	};

	int process(CHAR_DATA *ch, CHAR_DATA *victim);

	// дамаг атакующего
	int dam;
	// flags[CRIT_HIT] = true, dam_critic = 0 - критический удар
	// flags[CRIT_HIT] = true, dam_critic > 0 - удар точным стилем
	int dam_critic;
	// тип урона (физ/маг/обычный)
	int dmg_type;
	// см. описание в HitData
	int skill_num;
	// номер заклинания, если >= 0
	int spell_num;
	// Какой стихией магии наносится урон.
	// Применяется, если урон магический, но наносится не спеллом.
	// Если спеллом - тип урона берется из самого спелла.
	int magic_type;
	// см. описание в HitData, но здесь может быть -1
	int hit_type;
	// номер сообщения об ударе из файла messages
	// инится только начиная с вызова process
	int msg_num;
	// набор флагов из HitType
	std::bitset<FightSystem::HIT_TYPE_FLAGS_NUM> flags;
	// позиция атакующего на начало атаки (по дефолту будет = текущему положению)
	int ch_start_pos;
	// позиция жертвы на начало атаки (по дефолту будет = текущему положению)
	int victim_start_pos;

private:
	// инит всех полей дефолтными значениями для конструкторов
	void zero_init();
	// инит msg_num, ch_start_pos, victim_start_pos
	// дергается в начале process, когда все уже заполнено
	void post_init(CHAR_DATA *ch, CHAR_DATA *victim);
	void post_init_shields(CHAR_DATA *victim);
	// process()
	bool magic_shields_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void armor_dam_reduce(CHAR_DATA *victim);
	bool dam_absorb(CHAR_DATA *ch, CHAR_DATA *victim);
	void process_death(CHAR_DATA *ch, CHAR_DATA *victim);
	void send_critical_message(CHAR_DATA *ch, CHAR_DATA *victim);
	void dam_message(CHAR_DATA* ch, CHAR_DATA* victim) const;

	// обратный дамаг от огненного щита
	int fs_damage;
	// строка для краткого режима щитов, дописывается после ударов и прочего
	// если во flags были соответствующие флаги
	std::string brief_shields_;
};

// fight.cpp

void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim);
inline void set_fighting(const CHAR_DATA::shared_ptr& ch, CHAR_DATA *victim) { set_fighting(ch.get(), victim); }

void stop_fighting(CHAR_DATA *ch, int switch_others);
void perform_violence();
int calc_initiative(CHAR_DATA *ch, bool mode);

// fight_hit.cpp

int compute_armor_class(CHAR_DATA *ch);
bool check_mighthit_weapon(CHAR_DATA *ch);
void apply_weapon_bonus(int ch_class, const ESkill skill, int *damroll, int *hitroll);

// fight_stuff.cpp

void die(CHAR_DATA *ch, CHAR_DATA *killer);
void raw_kill(CHAR_DATA *ch, CHAR_DATA *killer);

void alterate_object(OBJ_DATA *obj, int dam, int chance);
void alt_equip(CHAR_DATA *ch, int pos, int dam, int chance);

void char_dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *victim, bool mayflee);
void test_self_hitroll(CHAR_DATA *ch);

int calc_leadership(CHAR_DATA * ch);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
