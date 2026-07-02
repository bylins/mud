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
#include <vector>

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
 * Для входа с необычного (серверного) дамага без инита остальных полей: источник урона
 * задаётся сразу как fight::EDamageSource (ловушка, кровотечение и т.п.):
 * Damage obj(SimpleDmg(fight::EDamageSource::kSuffering), dam, fight::kUndefDmg)
 * obj.process(ch, victim);
 */
struct SimpleDmg {
  explicit SimpleDmg(fight::EDamageSource source) : damage_source(source) {};
  fight::EDamageSource damage_source;
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
		damage_source(obj.damage_source) {};

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
  // Источник урона для выбора боевого сообщения: тип атаки оружием (kHit..kSting),
  // либо серверный урон (ловушка/кровотечение). kUndefined - источник не задан.
  fight::EDamageSource damage_source{fight::EDamageSource::kUndefined};
  // набор флагов из HitType
  std::bitset<fight::kHitFlagsNum> flags;
  // позиция атакующего на начало атаки (по дефолту будет = текущему положению)
  EPosition ch_start_pos{EPosition::kUndefined};
  // позиция жертвы на начало атаки (по дефолту будет = текущему положению)
  EPosition victim_start_pos{EPosition::kUndefined};
  // Оружие которым нанесли дамаг
  ObjData *wielded{nullptr};
  // UID персонажа-"автора" этого урона для начисления убийства, когда прямого атакующего
  // нет (урон нанесён сам себе: тик повреждающего аффекта и т.п.). Обычно это caster_id
  // аффекта. 0 - автора нет; равен UID жертвы - урон самому себе, убийство не засчитывается.
  long author_uid{0};
  // issue.character-affect-triggers: affect-owned damage flavor (set by the affect-trigger runners via
  // CastDamage/LandOneDamageHit). When any is non-empty it FULLY REPLACES the generic combat hit line
  // and the char_dam_message severity line in Process. $n = ch, $N = victim.
  std::string aff_msg_char_;
  std::string aff_msg_vict_;
  std::string aff_msg_room_;

 private:
  // инит всех полей дефолтными значениями для конструкторов
  // инит msg_num, ch_start_pos, victim_start_pos
  // дергается в начале process, когда все уже заполнено
  void PerformPostInit(CharData *ch, CharData *victim);
  // issue.damage-change: weighted-random-pick one elemental shield (by AffectShieldWeight) among the
  // victim's shields; stores its EAffect underlying value in selected_shield_ (-1 = none). A hit passes
  // through exactly that shield -- its retaliation/reduction actions run, the others are skipped.
  void SelectMagicShield(CharData *victim);
  // process()
  void CalcArmorDmgAbsorption(CharData *victim);
  bool CalcDmgAbsorption(CharData *ch, CharData *victim);
  void ProcessDeath(CharData *ch, CharData *victim) const;
  void SendCritHitMsg(CharData *ch, CharData *victim);
  void ProcessBlink(CharData *ch, CharData *victim);
  // issue.damage-change: apply the victim's kWardDamage <damage_change> actions to this in-flight Damage
  // (data-driven incoming-damage modifiers: scale dam, edit flags), gated by type/element/flags.
  void ApplyAffectDamageChanges(CharData *ch, CharData *victim, bool late_stage);
  // issue.damage-change: scan the victim's kWardDamage <retaliation> actions and append their thorns
  // contributions to reflect_pool_ (reads the PRE-reduction dam; never mutates it). Skipped for reflected
  // damage (kMagicReflect) to stop a Process->Process bounce. DealReflectPool deals the pool afterwards.
  void ApplyRetaliations(CharData *ch, CharData *victim);
  void DealReflectPool(CharData *ch, CharData *victim);

  // issue.damage-change: one accumulated thorns contribution (a <retaliation> that fired). Dealt back to
  // the attacker as its own Damage after the main pipeline, so the attacker's own defenses transform it.
  // Attribution is NOT stored here: the reflect is credited to the incoming attack (this->spell_id) at
  // deal time -- a ward doesn't deal damage of its own; the reflected spell does.
  struct ReflectHit {
    int amount{0};
    fight::DmgType type{fight::kUndefDmg};
    EElement element{EElement::kUndefined};
  };
  std::vector<ReflectHit> reflect_pool_;

  // issue.damage-change: the elemental shield chosen (weighted-random) to handle this hit -- underlying
  // EAffect value, or -1 if the victim has no shield. Only this shield's actions apply; others skip.
  int selected_shield_{-1};

  // строка для краткого режима щитов, дописывается после ударов и прочего
  // если во flags были соответствующие флаги
  std::string brief_shields_;
};

#endif //BYLINS_SRC_GAMEPLAY_FIGHT_DAMAGE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
