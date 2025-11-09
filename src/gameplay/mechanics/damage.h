/**
\file damage.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_FIGHT_DAMAGE_H_
#define BYLINS_SRC_GAMEPLAY_FIGHT_DAMAGE_H_

#include "gameplay/fight/fight_constants.h"
#include "gameplay/skills/skills.h"
#include "engine/entities/entities_constants.h"

#include <bitset>

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
  explicit SimpleDmg(int num) : msg_num(num) {};
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
  Damage() = default;

  // скилы
  Damage(SkillDmg obj, int in_dam, fight::DmgType in_dmg_type, ObjData *wielded_obj)
	  : dam(in_dam),
		dmg_type(in_dmg_type),
		skill_id(obj.skill_id),
		wielded(wielded_obj) {};

  // заклинания
  Damage(SpellDmg obj, int in_dam, fight::DmgType in_dmg_type)
	  : dam(in_dam),
		dmg_type(in_dmg_type),
		spell_id(obj.spell_id) {};

  // прочий дамаг
  Damage(SimpleDmg obj, int in_dam, fight::DmgType in_dmg_type)
	  : dam(in_dam),
		dmg_type(in_dmg_type),
		msg_num(obj.msg_num) {};

  int Process(CharData *ch, CharData *victim);

  // дамаг атакующего
  int dam{0};
  // flags[CRIT_HIT] = true, dam_critic = 0 - критический удар
  // flags[CRIT_HIT] = true, dam_critic > 0 - удар точным стилем
  int dam_critic{0};
  // тип урона (физ/маг/обычный)
  int dmg_type{-1};
  // см. описание в HitData
  ESkill skill_id{ESkill::kUndefined};
  // номер заклинания, если >= 0
  ESpell spell_id{ESpell::kUndefined};
  // Какой стихией наносится урон.
  // Применяется, если урон магический, но наносится не спеллом.
  // Если спеллом - стихия берется из самого спелла.
  EElement element{EElement::kUndefined};
  // см. описание в HitData, но здесь может быть -1
  int hit_type{-1};
  // номер сообщения об ударе из файла messages
  // инится только начиная с вызова process
  int msg_num{-1};
  // набор флагов из HitType
  std::bitset<fight::kHitFlagsNum> flags;
  // позиция атакующего на начало атаки (по дефолту будет = текущему положению)
  EPosition ch_start_pos{EPosition::kUndefined};
  // позиция жертвы на начало атаки (по дефолту будет = текущему положению)
  EPosition victim_start_pos{EPosition::kUndefined};
  // Оружие которым нанесли дамаг
  ObjData *wielded{nullptr};

 private:
  // инит всех полей дефолтными значениями для конструкторов
  // инит msg_num, ch_start_pos, victim_start_pos
  // дергается в начале process, когда все уже заполнено
  void PerformPostInit(CharData *ch, CharData *victim);
  void SetPostInitShieldFlags(CharData *victim);
  // process()
  bool CalcMagisShieldsDmgAbsoption(CharData *ch, CharData *victim);
  void CalcArmorDmgAbsorption(CharData *victim);
  bool CalcDmgAbsorption(CharData *ch, CharData *victim);
  void ProcessDeath(CharData *ch, CharData *victim) const;
  void SendCritHitMsg(CharData *ch, CharData *victim);
  void SendDmgMsg(CharData *ch, CharData *victim) const;
  void ProcessBlink(CharData *ch, CharData *victim);

  // обратный дамаг от огненного щита
  int fs_damage{0};
  // строка для краткого режима щитов, дописывается после ударов и прочего
  // если во flags были соответствующие флаги
  std::string brief_shields_;
};

#endif //BYLINS_SRC_GAMEPLAY_FIGHT_DAMAGE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
