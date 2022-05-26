// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_H_
#define _FIGHT_H_

#include "fight_constants.h"
#include "entities/char_data.h"
#include "structs/structs.h"
#include "conf.h"
#include "sysdep.h"

/**
 * Для входа со скила без инита остальных полей:
 * Damage obj(SkillDmg(SKILL_NUM), dam, FightSystem::UNDEF_DMG|PHYS_DMG|MAGE_DMG)
 * obj.process(ch, victim);
 */
struct SkillDmg {
	explicit SkillDmg(ESkill id) : skill_id(id) {};
	ESkill skill_id;
};

/**
 * Для входа с закла без инита остальных полей:
 * Damage obj(SpellDmg(SPELL_NUM), dam, FightSystem::UNDEF_DMG|PHYS_DMG|MAGE_DMG)
 * obj.process(ch, victim);
 */
struct SpellDmg {
	explicit SpellDmg(ESpell spell_id) : spell_id(spell_id) {};
	ESpell spell_id;
};

/**
 * Для входа с необычного дамага без инита остальных полей (инится сразу номер messages):
 * Damage obj(SimpleDmg(TYPE_NUM), dam, FightSystem::UNDEF_DMG|PHYS_DMG|MAGE_DMG)
 * obj.process(ch, victim);
 */
struct SimpleDmg {
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
class Damage {
 public:
	// полностью ручное создание объекта
	Damage() { zero_init(); };

	// скилы
	Damage(SkillDmg obj, int in_dam, fight::DmgType in_dmg_type, ObjData *wielded_obj) {
		zero_init();
		skill_id = obj.skill_id;
		dam = in_dam;
		dmg_type = in_dmg_type;
		wielded = wielded_obj;
	};

	// заклинания
	Damage(SpellDmg obj, int in_dam, fight::DmgType in_dmg_type) {
		zero_init();
		spell_id = obj.spell_id;
		dam = in_dam;
		dmg_type = in_dmg_type;
	};

	// прочий дамаг
	Damage(SimpleDmg obj, int in_dam, fight::DmgType in_dmg_type) {
		zero_init();
		msg_num = obj.msg_num;
		dam = in_dam;
		dmg_type = in_dmg_type;
	};

	int Process(CharData *ch, CharData *victim);

	// дамаг атакующего
	int dam;
	// flags[CRIT_HIT] = true, dam_critic = 0 - критический удар
	// flags[CRIT_HIT] = true, dam_critic > 0 - удар точным стилем
	int dam_critic;
	// тип урона (физ/маг/обычный)
	int dmg_type;
	// см. описание в HitData
	ESkill skill_id;
	// номер заклинания, если >= 0
	ESpell spell_id;
	// Какой стихией наносится урон.
	// Применяется, если урон магический, но наносится не спеллом.
	// Если спеллом - стихия берется из самого спелла.
	EElement element;
	// см. описание в HitData, но здесь может быть -1
	int hit_type;
	// номер сообщения об ударе из файла messages
	// инится только начиная с вызова process
	int msg_num;
	// набор флагов из HitType
	std::bitset<fight::kHitFlagsNum> flags;
	// позиция атакующего на начало атаки (по дефолту будет = текущему положению)
	EPosition ch_start_pos;
	// позиция жертвы на начало атаки (по дефолту будет = текущему положению)
	EPosition victim_start_pos;
	// Оружие которым нанесли дамаг
	ObjData *wielded;

 private:
	// инит всех полей дефолтными значениями для конструкторов
	void zero_init();
	// инит msg_num, ch_start_pos, victim_start_pos
	// дергается в начале process, когда все уже заполнено
	void post_init(CharData *ch, CharData *victim);
	void post_init_shields(CharData *victim);
	// process()
	bool magic_shields_dam(CharData *ch, CharData *victim);
	void armor_dam_reduce(CharData *victim);
	bool dam_absorb(CharData *ch, CharData *victim);
	void process_death(CharData *ch, CharData *victim);
	void send_critical_message(CharData *ch, CharData *victim);
	void dam_message(CharData *ch, CharData *victim) const;

	// обратный дамаг от огненного щита
	int fs_damage;
	// строка для краткого режима щитов, дописывается после ударов и прочего
	// если во flags были соответствующие флаги
	std::string brief_shields_;
};

// fight.cpp

void SetFighting(CharData *ch, CharData *vict);
inline void set_fighting(const CharData::shared_ptr &ch, CharData *victim) { SetFighting(ch.get(), victim); }

void stop_fighting(CharData *ch, int switch_others);
void perform_violence();
int calc_initiative(CharData *ch, bool mode);

// fight_hit.cpp

int compute_armor_class(CharData *ch);
bool check_mighthit_weapon(CharData *ch);
void GetClassWeaponMod(ECharClass class_id, const ESkill skill, int *damroll, int *hitroll);

// fight_stuff.cpp

void die(CharData *ch, CharData *killer);
void raw_kill(CharData *ch, CharData *killer);

void alterate_object(ObjData *obj, int dam, int chance);
void alt_equip(CharData *ch, int pos, int dam, int chance);

void char_dam_message(int dam, CharData *ch, CharData *victim, bool mayflee);
void TestSelfHitroll(CharData *ch);

int calc_leadership(CharData *ch);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
