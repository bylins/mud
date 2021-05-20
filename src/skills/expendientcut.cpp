//
// Created by ubuntu on 03/09/20.
//

#include "expendientcut.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/fight_constants.hpp"
#include "fightsystem/pk.h"
#include "skills/protect.h"
#include "skills.h"
#include "handler.h"

#include <math.h>

using namespace FightSystem;

ESkill ExpedientWeaponSkill(CHAR_DATA *ch) {
  ESkill skill = SKILL_PUNCH;

  if (GET_EQ(ch, WEAR_WIELD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == CObjectPrototype::ITEM_WEAPON)) {
    skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
  } else if (GET_EQ(ch, WEAR_BOTHS) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == CObjectPrototype::ITEM_WEAPON)) {
    skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
  } else if (GET_EQ(ch, WEAR_HOLD) && (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == CObjectPrototype::ITEM_WEAPON)) {
    skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD));
  };

  return skill;
}
int GetExpedientKeyParameter(CHAR_DATA *ch, ESkill skill) {
  switch (skill) {
    case SKILL_PUNCH:
    case SKILL_CLUBS:
    case SKILL_AXES:
    case SKILL_BOTHHANDS:
    case SKILL_SPADES:return ch->get_str();
      break;
    case SKILL_LONGS:
    case SKILL_SHORTS:
    case SKILL_NONSTANDART:
    case SKILL_BOWS:
    case SKILL_PICK:return ch->get_dex();
      break;
    default:return ch->get_str();
  }
}
int ParameterBonus(int parameter) {
  return ((parameter - 20) / 4);
}
int ExpedientRating(CHAR_DATA *ch, ESkill skill) {
  return floor(ch->get_skill(skill) / 2.00 + ParameterBonus(GetExpedientKeyParameter(ch, skill)));
}
int ExpedientCap(CHAR_DATA *ch, ESkill skill) {
  if (!IS_NPC(ch)) {
    return floor(1.33 * (CalcSkillRemortCap(ch) / 2.00 + ParameterBonus(GetExpedientKeyParameter(ch, skill))));
  } else {
    return floor(1.33 * ((kSkillCapOnZeroRemort + 5 * MAX(0, GET_LEVEL(ch) - 30) / 2.00
        + ParameterBonus(GetExpedientKeyParameter(ch, skill)))));
  }
}
int DegreeOfSuccess(int roll, int rating) {
  return ((rating - roll) / 5);
}

bool CheckExpedientSuccess(CHAR_DATA *ch, CHAR_DATA *victim) {
  ESkill DoerSkill = ExpedientWeaponSkill(ch);
  int DoerRating = ExpedientRating(ch, DoerSkill);
  int DoerCap = ExpedientCap(ch, DoerSkill);
  int DoerRoll = dice(1, DoerCap);
  int DoerSuccess = DegreeOfSuccess(DoerRoll, DoerRating);

  ESkill VictimSkill = ExpedientWeaponSkill(victim);
  int VictimRating = ExpedientRating(victim, VictimSkill);
  int VictimCap = ExpedientCap(victim, VictimSkill);
  int VictimRoll = dice(1, VictimCap);
  int VictimSuccess = DegreeOfSuccess(VictimRoll, VictimRating);

  if ((DoerRoll <= DoerRating) && (VictimRoll > VictimRating))
    return true;
  if ((DoerRoll > DoerRating) && (VictimRoll <= VictimRating))
    return false;
  if ((DoerRoll > DoerRating) && (VictimRoll > VictimRating))
    return CheckExpedientSuccess(ch, victim);

  if (DoerSuccess > VictimSuccess)
    return true;
  if (DoerSuccess < VictimSuccess)
    return false;

  if (ParameterBonus(GetExpedientKeyParameter(ch, DoerSkill))
      > ParameterBonus(GetExpedientKeyParameter(victim, VictimSkill)))
    return true;
  if (ParameterBonus(GetExpedientKeyParameter(ch, DoerSkill))
      < ParameterBonus(GetExpedientKeyParameter(victim, VictimSkill)))
    return false;

  if (DoerRoll < VictimRoll)
    return true;
  if (DoerRoll > VictimRoll)
    return true;

  return CheckExpedientSuccess(ch, victim);
}

void ApplyNoFleeAffect(CHAR_DATA *ch, int duration) {
  //Это жутко криво, но почемук-то при простановке сразу 2 флагов битвектора начинаются глюки, хотя все должно быть нормально
  //По-видимому, где-то просто не учтено, что ненулевых битов может быть более 1
  AFFECT_DATA<EApplyLocation> Noflee;
  Noflee.type = SPELL_BATTLE;
  Noflee.bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
  Noflee.location = EApplyLocation::APPLY_NONE;
  Noflee.modifier = 0;
  Noflee.duration = pc_duration(ch, duration, 0, 0, 0, 0);;
  Noflee.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
  affect_join(ch, Noflee, TRUE, FALSE, TRUE, FALSE);

  AFFECT_DATA<EApplyLocation> Battle;
  Battle.type = SPELL_BATTLE;
  Battle.bitvector = to_underlying(EAffectFlag::AFF_EXPEDIENT);
  Battle.location = EApplyLocation::APPLY_NONE;
  Battle.modifier = 0;
  Battle.duration = pc_duration(ch, duration, 0, 0, 0, 0);;
  Battle.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
  affect_join(ch, Battle, TRUE, FALSE, TRUE, FALSE);

  send_to_char("Вы выпали из ритма боя.\r\n", ch);
}

void go_cut_shorts(CHAR_DATA *ch, CHAR_DATA *vict) {

  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_EXPEDIENT)) {
    send_to_char("Вы еще не восстановил равновесие после предыдущего приема.\r\n", ch);
    return;
  }

  vict = try_protect(vict, ch);

  if (!CheckExpedientSuccess(ch, vict)) {
    act("Ваши свистящие удары пропали втуне, не задев $N3.", FALSE, ch, 0, vict, TO_CHAR);
    Damage dmg(SkillDmg(SKILL_SHORTS), ZERO_DMG, PHYS_DMG);
    dmg.process(ch, vict);
    ApplyNoFleeAffect(ch, 2);
    return;
  }

  act("$n сделал$g неуловимое движение и на мгновение исчез$q из вида.", FALSE, ch, 0, vict, TO_VICT);
  act("$n сделал$g неуловимое движение, сместившись за спину $N1.",
      TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
  hit(ch, vict, ESkill::SKILL_UNDEF, FightSystem::MAIN_HAND);
  hit(ch, vict, ESkill::SKILL_UNDEF, FightSystem::OFFHAND);

  AFFECT_DATA<EApplyLocation> AffectImmunPhysic;
  AffectImmunPhysic.type = SPELL_EXPEDIENT;
  AffectImmunPhysic.location = EApplyLocation::APPLY_PR;
  AffectImmunPhysic.modifier = 100;
  AffectImmunPhysic.duration = pc_duration(ch, 3, 0, 0, 0, 0);
  AffectImmunPhysic.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
  affect_join(ch, AffectImmunPhysic, FALSE, FALSE, FALSE, FALSE);
  AFFECT_DATA<EApplyLocation> AffectImmunMagic;
  AffectImmunMagic.type = SPELL_EXPEDIENT;
  AffectImmunMagic.location = EApplyLocation::APPLY_MR;
  AffectImmunMagic.modifier = 100;
  AffectImmunMagic.duration = pc_duration(ch, 3, 0, 0, 0, 0);
  AffectImmunMagic.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
  affect_join(ch, AffectImmunMagic, FALSE, FALSE, FALSE, FALSE);

  ApplyNoFleeAffect(ch, 3);
}

void SetExtraAttackCutShorts(CHAR_DATA *ch, CHAR_DATA *victim) {
  if (!isHaveNoExtraAttack(ch)) {
    return;
  }

  if (!pk_agro_action(ch, victim)) {
    return;
  }

  if (!ch->get_fighting()) {
    act("Ваше оружие свистнуло, когда вы бросились на $N3, применив \"порез\".", FALSE, ch, 0, victim, TO_CHAR);
    set_fighting(ch, victim);
    ch->set_extra_attack(EXTRA_ATTACK_CUT_SHORTS, victim);
  } else {
    act("Хорошо. Вы попытаетесь порезать $N3.", FALSE, ch, 0, victim, TO_CHAR);
    ch->set_extra_attack(EXTRA_ATTACK_CUT_SHORTS, victim);
  }
}

void SetExtraAttackCutPick(CHAR_DATA *ch, CHAR_DATA *victim) {
  if (!isHaveNoExtraAttack(ch)) {
    return;
  }
  if (!pk_agro_action(ch, victim)) {
    return;
  }

  if (!ch->get_fighting()) {
    act("Вы перехватили оружие обратным хватом и проскользнули за спину $N1.", FALSE, ch, 0, victim, TO_CHAR);
    set_fighting(ch, victim);
    ch->set_extra_attack(EXTRA_ATTACK_CUT_PICK, victim);
  } else {
    act("Хорошо. Вы попытаетесь порезать $N3.", FALSE, ch, 0, victim, TO_CHAR);
    ch->set_extra_attack(EXTRA_ATTACK_CUT_PICK, victim);
  }
}

ESkill GetExpedientCutSkill(CHAR_DATA *ch) {
  ESkill skill = SKILL_INVALID;

  if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD)) {
    skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_WIELD));
    if (skill != GET_OBJ_SKILL(GET_EQ(ch, WEAR_HOLD))) {
      send_to_char("Для этого приема в обеих руках нужно держать оружие одого типа!\r\n", ch);
      return SKILL_INVALID;
    }
  } else if (GET_EQ(ch, WEAR_BOTHS)) {
    skill = static_cast<ESkill>GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS));
  } else {
    send_to_char("Для этого приема вам надо использовать одинаковое оружие в обеих руках либо двуручное.\r\n", ch);
    return SKILL_INVALID;
  }

  if (!can_use_feat(ch, FindWeaponMasterBySkill(skill)) && !IS_IMPL(ch)) {
    send_to_char("Вы недостаточно искусны в обращении с этим видом оружия.\r\n", ch);
    return SKILL_INVALID;
  }

  return skill;
}

void do_expedient_cut(CHAR_DATA *ch, char *argument, int/* cmd*/, int /*subcmd*/) {

  ESkill skill;

  if (IS_NPC(ch) || (!can_use_feat(ch, EXPEDIENT_CUT_FEAT) && !IS_IMPL(ch))) {
    send_to_char("Вы не владеете таким приемом.\r\n", ch);
    return;
  }

  if (ch->ahorse()) {
    send_to_char("Верхом это сделать затруднительно.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Вам стоит встать на ноги.\r\n", ch);
    return;
  }

  if (!isHaveNoExtraAttack(ch)) {
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT)
      || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кого вы хотите порезать?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Вы таки да? Ой-вей, но тут таки Древняя Русь, а не Палестина!\r\n", ch);
    return;
  }
  if (ch->get_fighting() && vict->get_fighting() != ch) {
    act("$N не сражается с вами, не трогайте $S.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  skill = GetExpedientCutSkill(ch);
  if (skill == SKILL_INVALID)
    return;

  switch (skill) {
    case SKILL_SHORTS:SetExtraAttackCutShorts(ch, vict);
      break;
    case SKILL_SPADES:SetExtraAttackCutShorts(ch, vict);
      break;
    case SKILL_LONGS:
    case SKILL_BOTHHANDS:
      send_to_char("Порез мечом (а тем более двуручником или копьем) - это сурьезно. Но пока невозможно.\r\n",
                   ch);
      break;
    default:send_to_char("Ваше оружие не позволяет провести такой прием.\r\n", ch);
  }

}
