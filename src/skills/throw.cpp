#include "throw.h"
#include "action_targeting.h"
#include "abilities/abilities_rollsystem.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "handler.h"
#include "spells.h"
#include "protect.h"
#include "fightsystem/common.h"

using namespace FightSystem;
using namespace abilities;

// ************* THROW PROCEDURES

// Временная костыльная функция до реализации встроенного механизма работы сайдскиллов
// TODO Не забыть реализовать постоянный механизм
void performShadowThrowSideAbilities(TechniqueRollType &technique) {
  OBJ_DATA *weapon = GET_EQ(technique.actor(), technique.getWeaponEquipPosition());
  if (!weapon) {
    return;
  }
  int featureID = INCORRECT_FEAT;
  std::string to_char, to_vict, to_room;
  void (*doSideAction)(TechniqueRollType &technique);
  uint32_t mobNoFlag = MOB_DELETE;

  switch (GET_OBJ_SKILL(weapon)) {
    case SKILL_SPADES:
      mobNoFlag = MOB_NOBASH;
      featureID = SHADOW_SPEAR_FEAT;
      to_char = "Попадание копья повалило $n3 наземь.";
      to_vict = "Копье $N1 попало вам в колено. Вы рухнули наземь! Кажется, ваши приключения сейчас закончатся...";
      to_room = "Копье $N1 сбило $n3 наземь!";
      doSideAction = ([](TechniqueRollType &technique) {
        if (technique.rival()->ahorse()) { //если на лошади - падение с лагом 3
          technique.rival()->drop_from_horse();
        } else { // иначе просто садится на попу с лагом 2
          GET_POS(technique.rival()) = MIN(GET_POS(technique.rival()), POS_SITTING);
          set_wait(technique.rival(), 2, TRUE);
        }
      });
      break;
    case SKILL_SHORTS:
    case SKILL_PICK:
      mobNoFlag = MOB_NOSIELENCE;
      featureID = SHADOW_DAGGER_FEAT;
      to_char = "Меткое попадание вашего кинжала заставило $n3 умолкнуть.";
      to_vict = "Бросок $N1 угодил вам в горло. Вы прикусили язык!";
      to_room = "Меткое попадание $N1 заставило $n3 умолкнуть!";
      doSideAction = ([](TechniqueRollType &technique) {
        AFFECT_DATA<EApplyLocation> af;
        af.type = SPELL_BATTLE;
        af.bitvector = to_underlying(EAffectFlag::AFF_SILENCE);
        af.duration = pc_duration(technique.rival(), 2, GET_LEVEL(technique.actor()), 9, 6, 2);
        af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
        affect_join(technique.rival(), af, FALSE, FALSE, FALSE, FALSE);
      });
      break;
    case SKILL_CLUBS:
      mobNoFlag = MOB_NOSTUPOR;
      featureID = SHADOW_CLUB_FEAT;
      to_char = "Попадание булавы ошеломило $n3.";
      to_vict = "Брошенная $N4 булава врезалась вам в лоб! Какие красивые звёздочки вокруг...";
      to_room = "Попадание булавы $N1 ошеломило $n3!";
      doSideAction = ([](TechniqueRollType &technique) {
        AFFECT_DATA<EApplyLocation> af;
        af.type = SPELL_BATTLE;
        af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
        af.duration = pc_duration(technique.rival(), 3, 0, 0, 0, 0);
        af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
        affect_join(technique.rival(), af, FALSE, FALSE, FALSE, FALSE);
        set_wait(technique.rival(), 3, TRUE);
      });
      break;
  };

  if (!can_use_feat(technique.actor(), featureID)) {
    return;
  };
  TechniqueRollType sideAbility;
  sideAbility.initialize(technique.actor(), featureID, technique.rival());
  if (sideAbility.isSuccess() && !MOB_FLAGGED(technique.rival(), mobNoFlag)) {
    act(to_char.c_str(), FALSE, technique.rival(), 0, technique.actor(), TO_VICT);
    act(to_vict.c_str(), FALSE, technique.rival(), 0, technique.actor(), TO_CHAR);
    act(to_room.c_str(), FALSE, technique.rival(), 0, technique.actor(), TO_NOTVICT | TO_ARENA_LISTEN);
    doSideAction(technique);
  }
};

// TODO: Перенести подобную логику в модуль абилок, разделить уровни
void performWeaponThrow(TechniqueRollType &technique, Damage &techniqueDamage) {
  techniqueDamage.dam = ZERO_DMG;
  if (technique.isSuccess()) {
    techniqueDamage.dam = technique.calculateDamage();
    if (technique.isCriticalSuccess()) {
      send_to_char("&GВ яблочко!&n\r\n", technique.actor());
      techniqueDamage.flags.set(IGNORE_ARMOR);
      techniqueDamage.flags.set(CRIT_HIT);
    };
    if (timed_by_feat(technique.actor(), SHADOW_THROW_FEAT)) {
      decreaseFeatTimer(technique.actor(), SHADOW_THROW_FEAT);
    };
    if (technique.ID() == SHADOW_THROW_FEAT) {
      performShadowThrowSideAbilities(technique);
    };
  } else {
    if (technique.isCriticalFail()) {
      OBJ_DATA *weapon = unequip_char(technique.actor(), technique.getWeaponEquipPosition());
      if (weapon) {
        obj_to_char(weapon, technique.actor());
        send_to_char(technique.actor(), "&BВы выронили %s!&n\r\n", GET_OBJ_PNAME(weapon, 3).c_str());
      };
    };
  };
  techniqueDamage.process(technique.actor(), technique.rival());
};

void go_throw(CHAR_DATA *ch, CHAR_DATA *victim) {

  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  // TODO: Возможно, стоит добавить простой тест на добавление целей.
  short victimsAmount = 1 + PRF_FLAGGED(ch, PRF_DOUBLE_THROW) + 2 * PRF_FLAGGED(ch, PRF_TRIPLE_THROW);

  int techniqueID = THROW_WEAPON_FEAT;
  DmgType throwDamageKind = PHYS_DMG;
  if (PRF_FLAGGED(ch, PRF_SHADOW_THROW)) {
    send_to_char("Рукоять оружия в вашей руке налилась неестественным холодом.\r\n", ch);
    act("Оружие в руках $n1 окружила призрачная дымка.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
    techniqueID = SHADOW_THROW_FEAT;
    throwDamageKind = MAGE_DMG;
    struct timed_type shadowThrowTimed;
    shadowThrowTimed.skill = SHADOW_THROW_FEAT;
    shadowThrowTimed.time = 6;
    timed_feat_to_char(ch, &shadowThrowTimed);
    PRF_FLAGS(ch).unset(PRF_SHADOW_THROW);
  }
  TechniqueRollType weaponThrowRoll;
  Damage throwDamage(SkillDmg(SKILL_THROW), ZERO_DMG, throwDamageKind);
  throwDamage.magic_type = STYPE_DARK;

  ActionTargeting::FoesRosterType
      roster{ch, victim, [](CHAR_DATA *ch, CHAR_DATA *victim) { return CAN_SEE(ch, victim); }};
  for (auto target : roster) {
    target = try_protect(target, ch);
    weaponThrowRoll.initialize(ch, techniqueID, target);
    if (weaponThrowRoll.isWrongConditions()) {
      weaponThrowRoll.sendDenyMessageToActor();
      break;
    };
    performWeaponThrow(weaponThrowRoll, throwDamage);
    --victimsAmount;
    if (ch->purged() || victimsAmount == 0) {
      break;
    };
  };

  setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
  if (techniqueID == THROW_WEAPON_FEAT) {
    setSkillCooldownInFight(ch, SKILL_THROW, 3);
  }
}

void do_throw(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd) {
  //Svent TODO: Не забыть убрать заглушку после дописывания навыков
  if (!ch->get_skill(SKILL_THROW)) {
    send_to_char("Вы принялись метать икру. Это единственное, что вы умеете метать.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_THROW)) {
    send_to_char("Так и рука отвалится, нужно передохнуть.\r\n", ch);
    return;
  };
/*
	if (!IS_IMPL(ch) && !can_use_feat(ch, THROW_WEAPON_FEAT)) {
			send_to_char("Вы не умеете этого.\r\n", ch);
			return;
	}
*/
  CHAR_DATA *victim = findVictim(ch, argument);
  if (!victim) {
    send_to_char("В кого мечем?\r\n", ch);
    return;
  }

  if (ch == victim) {
    send_to_char("Вы начали метаться как белка в колесе.\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, victim, argument)) {
    return;
  }
  if (!check_pkill(ch, victim, arg)) {
    return;
  }

  if (subcmd == SCMD_SHADOW_THROW) {
    if (timed_by_feat(ch, SHADOW_THROW_FEAT)) {
      send_to_char("Не стоит так часто беспокоить тёмные силы.\r\n", ch);
      return;
    }
    PRF_FLAGS(ch).set(PRF_SHADOW_THROW);
  };

  if (IS_IMPL(ch) || !ch->get_fighting()) {
    go_throw(ch, victim);
  } else {
    if (isHaveNoExtraAttack(ch)) {
      act("Хорошо. Вы попытаетесь метнуть оружие в $N3.", FALSE, ch, 0, victim, TO_CHAR);
      ch->set_extra_attack(EXTRA_ATTACK_THROW, victim);
    }
  }
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
