#include "kick.h"
#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/common.h"
#include "spells.h"
#include "handler.h"
#include "protect.h"
#include "skills_info.h"

using namespace FightSystem;

// ******************  KICK PROCEDURES
void go_kick(CHAR_DATA *ch, CHAR_DATA *vict) {
  const char *to_char = NULL, *to_vict = NULL, *to_room = NULL;

  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_KICK)) {
    send_to_char("Вы уже все ноги себе отбили, отдохните слегка.\r\n", ch);
    return;
  };

  vict = try_protect(vict, ch);

  int percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + number(1, skill_info[SKILL_KICK].difficulty);
  int prob = CalcCurrentSkill(ch, SKILL_KICK, vict);
  if (GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_MOB_HOLD(vict)) {
    prob = percent;
  }
  if (GET_GOD_FLAG(ch, GF_GODSCURSE) || (!ch->ahorse() && vict->ahorse())) {
    prob = 0;
  }

  if (check_spell_on_player(ch, SPELL_WEB)) {
    prob /= 3;
  }

  bool success = percent <= prob;
  TrainSkill(ch, SKILL_KICK, success, vict);
  SendSkillBalanceMsg(ch, skill_info[SKILL_KICK].name, percent, prob, success);
  if (!success) {
    Damage dmg(SkillDmg(SKILL_KICK), ZERO_DMG, PHYS_DMG);
    dmg.process(ch, vict);
    prob = 2;
  } else {
    int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GET_REAL_DR(ch) + GET_LEVEL(ch) / 6;
    if (!IS_NPC(ch) || (IS_NPC(ch) && GET_EQ(ch, WEAR_FEET))) {
      int modi = MAX(0, (ch->get_skill(SKILL_KICK) + 4) / 5);
      dam += number(0, modi * 2);
      modi = 5 * (10 + (GET_EQ(ch, WEAR_FEET) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_FEET)) : 0));
      dam = modi * dam / 100;
    }
    if (ch->ahorse() && (ch->get_skill(SKILL_HORSE) >= 150) && (ch->get_skill(SKILL_KICK) >= 150)) {
      AFFECT_DATA<EApplyLocation> af;
      af.location = APPLY_NONE;
      af.type = SPELL_BATTLE;
      af.modifier = 0;
      af.battleflag = 0;
      float modi = ((ch->get_skill(SKILL_KICK) + GET_REAL_STR(ch) * 5)
          + (GET_EQ(ch, WEAR_FEET) ? GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_FEET)) : 0) * 3) / float(GET_SIZE(vict));
      if (number(1, 1000) < modi * 10) {
        switch (number(0, (ch->get_skill(SKILL_KICK) - 150) / 10)) {
          case 0:
          case 1:
            if (!AFF_FLAGGED(vict, EAffectFlag::AFF_STOPRIGHT)) {
              to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
              to_vict = "Мощный удар ноги $n1 изуродовал вам правую руку.";
              to_room = "След сапога $n1 надолго запомнится $N2, если конечно он выживет.";
              af.type = SPELL_BATTLE;
              af.bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
              af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 4, 0, 0, 0, 0);
              af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
            } else if (!AFF_FLAGGED(vict, EAffectFlag::AFF_STOPLEFT)) {
              to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
              to_vict = "Мощный удар ноги $n1 изуродовал вам левую руку.";
              to_room = "След сапога $n1 надолго запомнится $N2, если конечно он выживет.";
              af.bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
              af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 4, 0, 0, 0, 0);
              af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
            } else {
              to_char = "Каблук вашего сапога надолго запомнится $N2, $M теперь даже бить вас нечем.";
              to_vict = "Мощный удар ноги $n1 вывел вас из строя.";
              to_room = "Каблук сапога $n1 надолго запомнится $N2, $M теперь даже биться нечем.";
              af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
              af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 4, 0, 0, 0, 0);
              af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
            }
            dam *= 2;
            break;
          case 2:
          case 3:to_char = "Сильно пнув в челюсть, вы заставили $N3 замолчать.";
            to_vict = "Мощный удар ноги $n1 попал вам точно в челюсть, заставив вас замолчать.";
            to_room = "Сильно пнув ногой в челюсть $N3, $n заставил$q $S замолчать.";
            af.type = SPELL_BATTLE;
            af.bitvector = to_underlying(EAffectFlag::AFF_SILENCE);
            af.duration = pc_duration(vict, 3 + GET_REMORT(ch) / 5, 0, 0, 0, 0);
            af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
            dam *= 2;
            break;
          case 4:
          case 5:WAIT_STATE(vict, number(2, 5) * PULSE_VIOLENCE);
            if (GET_POS(vict) > POS_SITTING) {
              GET_POS(vict) = POS_SITTING;
            }
            to_char = "Ваш мощный пинок выбил пару зубов $N2, усадив $S на землю!";
            to_vict = "Мощный удар ноги $n1 попал точно в голову, свалив вас с ног.";
            to_room = "Мощный пинок $n1 выбил пару зубов $N2, усадив $S на землю!";
            dam *= 2;
            break;
          default:break;
        }
      } else if (number(1, 1000) < (ch->get_skill(SKILL_HORSE) / 2)) {
        dam *= 2;
        if (!IS_NPC(ch))
          send_to_char("Вы привстали на стременах.\r\n", ch);
      }

      if (to_char) {
        if (!IS_NPC(ch)) {
          sprintf(buf, "&G&q%s&Q&n", to_char);
          act(buf, FALSE, ch, 0, vict, TO_CHAR);
          sprintf(buf, "%s", to_room);
          act(buf, TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
        }
      }
      if (to_vict) {
        if (!IS_NPC(vict)) {
          sprintf(buf, "&R&q%s&Q&n", to_vict);
          act(buf, FALSE, ch, 0, vict, TO_VICT);
        }
      }
      affect_join(vict, af, TRUE, FALSE, TRUE, FALSE);
    }

    if (GET_AF_BATTLE(vict, EAF_AWAKE)) {
      dam >>= (2 - (ch->ahorse() ? 1 : 0));
    }
    Damage dmg(SkillDmg(SKILL_KICK), dam, PHYS_DMG);
    dmg.process(ch, vict);
    prob = 2;
  }
  setSkillCooldownInFight(ch, SKILL_KICK, prob);
  setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
}

void do_kick(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch) || !ch->get_skill(SKILL_KICK)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_KICK)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кто это так сильно путается под вашими ногами?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Вы БОЛЬНО пнули себя! Поздравляю, вы сошли с ума...\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  if (IS_IMPL(ch) || !ch->get_fighting()) {
    go_kick(ch, vict);
  } else if (isHaveNoExtraAttack(ch)) {
    act("Хорошо. Вы попытаетесь пнуть $N3.", FALSE, ch, 0, vict, TO_CHAR);
    ch->set_extra_attack(EXTRA_ATTACK_KICK, vict);
  }
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
