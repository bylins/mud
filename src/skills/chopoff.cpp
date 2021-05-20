#include "chopoff.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/start.fight.h"
#include "handler.h"
#include "spells.h"
#include "random.hpp"
#include "screen.h"
#include "skills_info.h"

using namespace FightSystem;

// ************************* CHOPOFF PROCEDURES
void go_chopoff(CHAR_DATA *ch, CHAR_DATA *vict) {
  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
    send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
    return;
  }

  if (ch->isHorsePrevents())
    return;

  if ((GET_POS(vict) < POS_FIGHTING)) {
    if (number(1, 100) < ch->get_skill(SKILL_CHOPOFF)) {
      send_to_char("Вы приготовились провести подсечку, но вовремя остановились.\r\n", ch);
      //set_wait(ch, 1, FALSE);
      //setSkillCooldown(ch, SKILL_CHOPOFF, 1);
      ch->setSkillCooldown(SKILL_CHOPOFF, PULSE_VIOLENCE / 6);
      return;
    }
  }

  if (!pk_agro_action(ch, vict))
    return;

  int percent = number(1, skill_info[SKILL_CHOPOFF].difficulty);
  int prob = CalculateCurrentSkill(ch, SKILL_CHOPOFF, vict);

  if (check_spell_on_player(ch, SPELL_WEB)) {
    prob /= 3;
  }
  if (GET_GOD_FLAG(ch, GF_GODSLIKE)
      || GET_MOB_HOLD(vict)
      || GET_GOD_FLAG(vict, GF_GODSCURSE)) {
    prob = percent;
  }

  if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
      GET_GOD_FLAG(vict, GF_GODSLIKE) ||
      vict->ahorse() || GET_POS(vict) < POS_FIGHTING || MOB_FLAGGED(vict, MOB_NOTRIP) || IS_IMMORTAL(vict))
    prob = 0;

  bool success = percent <= prob;
  TrainSkill(ch, SKILL_CHOPOFF, success, vict);
  SendSkillBalanceMsg(ch, skill_info[SKILL_CHOPOFF].name, percent, prob, success);
  if (!success) {
    sprintf(buf, "%sВы попытались подсечь $N3, но упали сами...%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    act("$n попытал$u подсечь вас, но упал$g сам$g.", FALSE, ch, 0, vict, TO_VICT);
    act("$n попытал$u подсечь $N3, но упал$g сам$g.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    GET_POS(ch) = POS_SITTING;
    prob = 3;
    if (can_use_feat(ch, EVASION_FEAT)) {
      AFFECT_DATA<EApplyLocation> af;
      af.type = SPELL_EXPEDIENT;
      af.location = EApplyLocation::APPLY_PR;
      af.modifier = 50;
      af.duration = pc_duration(ch, 3, 0, 0, 0, 0);
      af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
      affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
      af.location = EApplyLocation::APPLY_AR;
      affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
      af.location = EApplyLocation::APPLY_MR;
      affect_join(ch, af, FALSE, FALSE, FALSE, FALSE);
      send_to_char(ch,
                   "%sВы покатились по земле, пытаясь избежать атак %s.%s\r\n",
                   CCIGRN(ch, C_NRM),
                   GET_PAD(vict, 1),
                   CCNRM(ch, C_NRM));
      act("$n покатил$u по земле, пытаясь избежать ваших атак.", FALSE, ch, 0, vict, TO_VICT);
      act("$n покатил$u по земле, пытаясь избежать атак $N1.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    }
  } else {
    send_to_char(ch,
                 "%sВы провели подсечку, ловко усадив %s на землю.%s\r\n",
                 CCIBLU(ch, C_NRM),
                 GET_PAD(vict, 3),
                 CCNRM(ch, C_NRM));
    act("$n ловко подсек$q вас, усадив на попу.", FALSE, ch, 0, vict, TO_VICT);
    act("$n ловко подсек$q $N3, уронив $S на землю.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    set_wait(vict, 3, FALSE);

    if (ch->isInSameRoom(vict)) {
      GET_POS(vict) = POS_SITTING;
    }

    if (IS_HORSE(vict) && vict->get_master()->ahorse()) {
      vict->drop_from_horse();
    }
    prob = 1;
  }

  appear(ch);
  if (IS_NPC(vict) && CAN_SEE(vict, ch) && vict->have_mind() && vict->get_wait() <= 0) {
    set_hit(vict, ch);
  }

  if (!success) {
    set_wait(ch, prob, FALSE);
  } else {
    setSkillCooldownInFight(ch, SKILL_CHOPOFF, prob);
    setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
  }
}

void do_chopoff(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (ch->get_skill(SKILL_CHOPOFF) < 1) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_CHOPOFF)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  if (ch->ahorse()) {
    send_to_char("Верхом это сделать затруднительно.\r\n", ch);
    return;
  }

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кого вы собираетесь подсечь?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Вы можете воспользоваться командой <сесть>.\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  if (IS_IMPL(ch) || !ch->get_fighting())
    go_chopoff(ch, vict);
  else if (isHaveNoExtraAttack(ch)) {
    if (!IS_NPC(ch))
      act("Хорошо. Вы попытаетесь подсечь $N3.", FALSE, ch, 0, vict, TO_CHAR);
    ch->set_extra_attack(EXTRA_ATTACK_CHOPOFF, vict);
  }
}
