// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_H_
#define _FIGHT_H_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "char.hpp"

namespace FightSystem
{

enum DmgType { UNDEF_DMG, PHYS_DMG, MAGE_DMG };

// attack_hit_text[]
enum
{
	type_hit, type_skin, type_whip, type_slash, type_bite,
	type_bludgeon, type_crush, type_pound, type_claw, type_maul,
	type_thrash, type_pierce, type_blast, type_punch, type_stab,
	type_pick, type_sting
};

enum
{
	// игнор санки
	IGNORE_SANCT,
	// игнор призмы
	IGNORE_PRISM,
	// игнор брони
	IGNORE_ARMOR,
	// ополовинивание учитываемой брони
	HALF_IGNORE_ARMOR,
	// игнор поглощения
	IGNORE_ABSORBE,
	// нельзя сбежать
	NO_FLEE,
	// крит удар
	CRIT_HIT,
	// игнор возврата дамаги от огненного щита
	IGNORE_FSHIELD,
	// дамаг идет от магического зеркала или звукового барьера
	MAGIC_REFLECT,
	// жертва имеет огненный щит
	VICTIM_FIRE_SHIELD,
	// жертва имеет воздушный щит
	VICTIM_AIR_SHIELD,
	// жертва имеет ледяной щит
	VICTIM_ICE_SHIELD,
	// было отражение от огненного щита в кратком режиме
	DRAW_BRIEF_FIRE_SHIELD,
	// было отражение от воздушного щита в кратком режиме
	DRAW_BRIEF_AIR_SHIELD,
	// было отражение от ледяного щита в кратком режиме
	DRAW_BRIEF_ICE_SHIELD,
	// была обратка от маг. зеркала
	DRAW_BRIEF_MAG_MIRROR,

	// кол-во флагов
	HIT_TYPE_FLAGS_NUM
};

} // namespace FightSystem

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
	void armor_dam_reduce(CHAR_DATA *ch, CHAR_DATA *victim);
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
void stop_fighting(CHAR_DATA *ch, int switch_others);
void perform_violence();

// fight_hit.cpp

int compute_armor_class(CHAR_DATA *ch);
bool check_mighthit_weapon(CHAR_DATA *ch);
void apply_weapon_bonus(int ch_class, int skill, int *damroll, int *hitroll);

void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon);

// fight_stuff.cpp

void die(CHAR_DATA *ch, CHAR_DATA *killer);
void raw_kill(CHAR_DATA *ch, CHAR_DATA *killer);

void alterate_object(OBJ_DATA *obj, int dam, int chance);
void alt_equip(CHAR_DATA *ch, int pos, int dam, int chance);

void char_dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *victim, bool mayflee);
void test_self_hitroll(CHAR_DATA *ch);

#endif
