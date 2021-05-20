#include "bash.h"
#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "spells.h"
#include "protect.h"
#include "skills_info.h"

using namespace FightSystem;

// ************************* BASH PROCEDURES
void go_bash(CHAR_DATA *ch, CHAR_DATA *vict) {
  if (dontCanAct(ch) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  if (!(IS_NPC(ch) || GET_EQ(ch, WEAR_SHIELD) || IS_IMMORTAL(ch) || GET_MOB_HOLD(vict)
      || GET_GOD_FLAG(vict, GF_GODSCURSE))) {
    send_to_char("Вы не можете сделать этого без щита.\r\n", ch);
    return;
  };

  if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
    send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
    return;
  }

  if (ch->isHorsePrevents())
    return;

  if (ch == vict) {
    send_to_char("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Вам стоит встать на ноги.\r\n", ch);
    return;
  }

  vict = try_protect(vict, ch);

  int percent = number(1, skill_info[SKILL_BASH].difficulty);
  int prob = CalculateCurrentSkill(ch, SKILL_BASH, vict);

  if (GET_MOB_HOLD(vict) || GET_GOD_FLAG(vict, GF_GODSCURSE)) {
    prob = percent;
  }
  if (MOB_FLAGGED(vict, MOB_NOBASH) || GET_GOD_FLAG(ch, GF_GODSCURSE)) {
    prob = 0;
  }
  bool success = percent <= prob;
  TrainSkill(ch, SKILL_BASH, success, vict);

  SendSkillBalanceMsg(ch, skill_info[SKILL_BASH].name, percent, prob, success);
  if (!success) {
    Damage dmg(SkillDmg(SKILL_BASH), ZERO_DMG, PHYS_DMG);
    dmg.process(ch, vict);
    GET_POS(ch) = POS_SITTING;
    prob = 3;
  } else {
    //не дадим башить мобов в лаге которые спят, оглушены и прочее
    if (GET_POS(vict) <= POS_STUNNED && GET_WAIT(vict) > 0) {
      send_to_char("Ваша жертва и так слишком слаба, надо быть милосерднее.\r\n", ch);
      //set_wait(ch, 1, FALSE);
      ch->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, PULSE_VIOLENCE);
      return;
    }

    int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GET_REAL_DR(ch) +
        MAX(0, ch->get_skill(SKILL_BASH) / 10 - 5) + GET_LEVEL(ch) / 5;

//делаем блокирование баша
    if ((GET_AF_BATTLE(vict, EAF_BLOCK)
        || (can_use_feat(vict, DEFENDER_FEAT)
            && GET_EQ(vict, WEAR_SHIELD)
            && PRF_FLAGGED(vict, PRF_AWAKE)
            && vict->get_skill(SKILL_AWAKE)
            && vict->get_skill(SKILL_BLOCK)
            && GET_POS(vict) > POS_SITTING))
        && !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
        && !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
        && !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPLEFT)
        && GET_WAIT(vict) <= 0
        && GET_MOB_HOLD(vict) == 0) {
      if (!(GET_EQ(vict, WEAR_SHIELD) || IS_NPC(vict) || IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE)))
        send_to_char("У вас нечем отразить атаку противника.\r\n", vict);
      else {
        int range, prob2;
        range = number(1, skill_info[SKILL_BLOCK].difficulty);
        prob2 = CalculateCurrentSkill(vict, SKILL_BLOCK, ch);
        bool success2 = prob2 >= range;
        TrainSkill(vict, SKILL_BLOCK, success2, ch);
        if (!success2) {
          act("Вы не смогли блокировать попытку $N1 сбить вас.",
              FALSE, vict, 0, ch, TO_CHAR);
          act("$N не смог$Q блокировать вашу попытку сбить $S.",
              FALSE, ch, 0, vict, TO_CHAR);
          act("$n не смог$q блокировать попытку $N1 сбить $s.",
              TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
        } else {
          act("Вы блокировали попытку $N1 сбить вас с ног.",
              FALSE, vict, 0, ch, TO_CHAR);
          act("Вы хотели сбить $N1, но он$G блокировал$G Вашу попытку.",
              FALSE, ch, 0, vict, TO_CHAR);
          act("$n блокировал$g попытку $N1 сбить $s.",
              TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
          alt_equip(vict, WEAR_SHIELD, 30, 10);
          if (!ch->get_fighting()) {
            set_fighting(ch, vict);
            set_wait(ch, 1, TRUE);
            //setSkillCooldownInFight(ch, SKILL_BASH, 1);
          }
          return;
        }
      }
    }

    prob = 0; // если башем убил - лага не будет
    Damage dmg(SkillDmg(SKILL_BASH), dam, PHYS_DMG);
    dmg.flags.set(NO_FLEE_DMG);
    dam = dmg.process(ch, vict);

    if (dam > 0 || (dam == 0 && AFF_FLAGGED(vict, EAffectFlag::AFF_SHIELD))) {
      prob = 2;
      if (!vict->drop_from_horse()) {
        GET_POS(vict) = POS_SITTING;
        set_wait(vict, 3, FALSE);
      }
    }
  }
  set_wait(ch, prob, TRUE);
}

void do_bash(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if ((IS_NPC(ch) && (!AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER))) || !ch->get_skill(SKILL_BASH)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_BASH)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  if (ch->ahorse()) {
    send_to_char("Верхом это сделать затруднительно.\r\n", ch);
    return;
  }

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кого же вы так сильно желаете сбить?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  if (IS_IMPL(ch) || !ch->get_fighting()) {
    go_bash(ch, vict);
  } else if (isHaveNoExtraAttack(ch)) {
    if (!IS_NPC(ch))
      act("Хорошо. Вы попытаетесь сбить $N3.", FALSE, ch, 0, vict, TO_CHAR);
    ch->set_extra_attack(EXTRA_ATTACK_BASH, vict);
  }
}



// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
