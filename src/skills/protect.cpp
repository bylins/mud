#include "protect.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.hpp"
#include "handler.h"
#include "spells.h"
#include "skills_info.h"

// ************** PROTECT PROCEDURES
void go_protect(CHAR_DATA *ch, CHAR_DATA *vict) {
  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  ch->set_protecting(vict);
  act("Вы попытаетесь прикрыть $N3 от нападения.", FALSE, ch, 0, vict, TO_CHAR);
  SET_AF_BATTLE(ch, EAF_PROTECT);
}

void do_protect(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  one_argument(argument, arg);
  if (!*arg) {
    if (ch->get_protecting()) {
      CLR_AF_BATTLE(ch, EAF_PROTECT);
      ch->set_protecting(0);
      send_to_char("Вы перестали прикрывать своего товарища.\r\n", ch);
    } else {
      send_to_char("Вы никого не прикрываете.\r\n", ch);
    }
    return;
  }

  if (IS_NPC(ch) || !ch->get_skill(SKILL_PROTECT)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_PROTECT)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  CHAR_DATA *vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  if (!vict) {
    send_to_char("И кто так сильно мил вашему сердцу?\r\n", ch);
    return;
  };

  if (vict == ch) {
    send_to_char("Попробуйте парировать удары или защищаться щитом.\r\n", ch);
    return;
  }

  if (ch->get_fighting() == vict) {
    send_to_char("Вы явно пацифист, или мазохист.\r\n", ch);
    return;
  }

  CHAR_DATA *tch = nullptr;
  for (const auto i : world[ch->in_room]->people) {
    if (i->get_fighting() == vict) {
      tch = i;
      break;
    }
  }

  if (IS_NPC(vict) && tch
      && (!IS_NPC(tch)
          || (AFF_FLAGGED(tch, EAffectFlag::AFF_CHARM)
              && tch->has_master()
              && !IS_NPC(tch->get_master())))
      && (!IS_NPC(ch)
          || (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
              && ch->has_master()
              && !IS_NPC(ch->get_master())))) {
    send_to_char("Вы пытаетесь прикрыть чужого противника.\r\n", ch);
    return;
  }

  for (const auto tch : world[ch->in_room]->people) {
    if (tch->get_fighting() == vict && !may_kill_here(ch, tch, argument)) {
      return;
    }
  }

  go_protect(ch, vict);
}

CHAR_DATA *try_protect(CHAR_DATA *victim, CHAR_DATA *ch) {
  int percent = 0;
  int prob = 0;
  bool protect = false;

  if (ch->get_fighting() == victim)
    return victim;

  for (const auto vict : world[IN_ROOM(victim)]->people) {
    if (vict->get_protecting() == victim
        && !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
        && !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
        && !AFF_FLAGGED(vict, EAffectFlag::AFF_BLIND)
        && !GET_MOB_HOLD(vict)
        && GET_POS(vict) >= POS_FIGHTING) {
      if (vict == ch) {
        act("Вы попытались напасть на того, кого прикрывали, и замерли в глубокой задумчивости.",
            FALSE,
            vict,
            0,
            victim,
            TO_CHAR);
        act("$N пытается напасть на вас! Лучше бы вам отойти.", FALSE, victim, 0, vict, TO_CHAR);
        vict->set_protecting(0);
        vict->BattleAffects.unset(EAF_PROTECT);
        WAIT_STATE(vict, PULSE_VIOLENCE);
        AFFECT_DATA<EApplyLocation> af;
        af.type = SPELL_BATTLE;
        af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
        af.location = EApplyLocation::APPLY_NONE;
        af.modifier = 0;
        af.duration = pc_duration(vict, 1, 0, 0, 0, 0);
        af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
        affect_join(vict, af, TRUE, FALSE, TRUE, FALSE);
        return victim;
      }

      if (protect) {
        send_to_char(vict, "Чьи-то широкие плечи помешали вам прикрыть %s.\r\n", GET_PAD(vict->get_protecting(), 3));
        continue;
      }

      protect = true;
      percent = number(1, skill_info[SKILL_PROTECT].difficulty);
      prob = CalculateCurrentSkill(vict, SKILL_PROTECT, victim);
      prob = prob * 8 / 10;
      if (vict->haveCooldown(SKILL_PROTECT)) {
        prob /= 2;
      };
      if (GET_GOD_FLAG(vict, GF_GODSCURSE)) {
        prob = 0;
      }
      bool success = prob >= percent;
      ImproveSkill(vict, SKILL_PROTECT, success, ch);
      SendSkillBalanceMsg(ch, skill_info[SKILL_PROTECT].name, percent, prob, success);

      if ((vict->get_fighting() != ch) && (ch != victim)) {
        // агрим жертву после чего можно будет проверить возможно ли его здесь прикрыть(костыли конечно)
        if (!pk_agro_action(ch, victim))
          return victim;
        if (!may_kill_here(vict, ch, NoArgument))
          continue;
        // Вписываемся в противника прикрываемого ...
        stop_fighting(vict, FALSE);
        set_fighting(vict, ch);
      }

      if (!success) {
        act("Вы не смогли прикрыть $N3.", FALSE, vict, 0, victim, TO_CHAR);
        act("$N не смог$Q прикрыть вас.", FALSE, victim, 0, vict, TO_CHAR);
        act("$n не смог$q прикрыть $N3.", TRUE, vict, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
        //set_wait(vict, 3, TRUE);
        setSkillCooldownInFight(vict, SKILL_GLOBAL_COOLDOWN, 3);
      } else {
        if (!pk_agro_action(vict, ch))
          return victim; // по аналогии с реском прикрывая кого-то можно пофлагаться
        act("Вы героически прикрыли $N3, приняв удар на себя.", FALSE, vict, 0, victim, TO_CHAR);
        act("$N героически прикрыл$G вас, приняв удар на себя.", FALSE, victim, 0, vict, TO_CHAR);
        act("$n героически прикрыл$g $N3, приняв удар на себя.", TRUE, vict, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
        //set_wait(vict, 1, TRUE);
        setSkillCooldownInFight(vict, SKILL_GLOBAL_COOLDOWN, 1);
        setSkillCooldownInFight(vict, SKILL_PROTECT, 1);
        return vict;
      }
    }
  }
  return victim;
}
