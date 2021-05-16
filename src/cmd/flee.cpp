#include "flee.h"

#include "act.movement.h"
#include "random.hpp"

#include <math.h>

void reduce_exp_after_flee(CHAR_DATA *ch, CHAR_DATA *victim, room_rnum room) {
  if (can_use_feat(ch, RETREAT_FEAT) || ROOM_FLAGGED(room, ROOM_ARENA))
    return;

  const auto loss = MAX(1, GET_REAL_MAX_HIT(victim) - GET_HIT(victim)) * GET_LEVEL(victim);
  gain_exp(ch, -loss);
}

// ********************* FLEE PROCEDURE
void go_flee(CHAR_DATA *ch) {
  if (GET_MOB_HOLD(ch) || GET_WAIT(ch) > 0) {
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOFLEE) || AFF_FLAGGED(ch, EAffectFlag::AFF_LACKY)
      || PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
    send_to_char("Невидимые оковы мешают вам сбежать.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Вы не можете сбежать из этого положения.\r\n", ch);
    return;
  }

  if (!WAITLESS(ch))
    WAIT_STATE(ch, PULSE_VIOLENCE);

  if (ch->ahorse() && (GET_POS(ch->get_horse()) < POS_FIGHTING || GET_MOB_HOLD(ch->get_horse()))) {
    send_to_char("Ваш скакун не в состоянии вынести вас из боя!\r\n", ch);
    return;
  }

  int dirs[NUM_OF_DIRS];
  int correct_dirs = 0;

  for (auto i = 0; i < NUM_OF_DIRS; ++i) {
    if (legal_dir(ch, i, TRUE, FALSE) && !ROOM_FLAGGED(EXIT(ch, i)->to_room(), ROOM_DEATH)) {
      dirs[correct_dirs] = i;
      ++correct_dirs;
    }
  }

  if (correct_dirs > 0
      && !bernoulli_trial(std::pow((1.0 - static_cast<double>(correct_dirs) / NUM_OF_DIRS), NUM_OF_DIRS))) {
    const auto direction = dirs[number(0, correct_dirs - 1)];
    const auto was_fighting = ch->get_fighting();
    const auto was_in = ch->in_room;

    if (do_simple_move(ch, direction, TRUE, 0, true)) {
      act("$n запаниковал$g и пытал$u сбежать!", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
      send_to_char("Вы быстро убежали с поля битвы.\r\n", ch);
      if (ch->ahorse()) {
        act("Верн$W $N вынес$Q вас из боя.", FALSE, ch, 0, ch->get_horse(), TO_CHAR);
      }

      if (was_fighting && !IS_NPC(ch)) {
        reduce_exp_after_flee(ch, was_fighting, was_in);
      }
    } else {
      act("$n запаниковал$g и попытал$u убежать, но не смог$q!", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
      send_to_char("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
    }
  } else {
    act("$n запаниковал$g и попытал$u убежать, но не смог$q!", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
    send_to_char("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
  }
}

void go_dir_flee(CHAR_DATA *ch, int direction) {
  if (GET_MOB_HOLD(ch) || GET_WAIT(ch) > 0) {
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOFLEE) || AFF_FLAGGED(ch, EAffectFlag::AFF_LACKY)
      || PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
    send_to_char("Невидимые оковы мешают вам сбежать.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Вы не сможете сбежать из этого положения.\r\n", ch);
    return;
  }

  if (legal_dir(ch, direction, TRUE, FALSE)
      && !ROOM_FLAGGED(EXIT(ch, direction)->to_room(), ROOM_DEATH)) {
    if (do_simple_move(ch, direction, TRUE, 0, true)) {
      const auto was_in = ch->in_room;
      const auto was_fighting = ch->get_fighting();

      act("$n запаниковал$g и попытал$u убежать.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
      send_to_char("Вы быстро убежали с поля битвы.\r\n", ch);
      if (was_fighting && !IS_NPC(ch)) {
        reduce_exp_after_flee(ch, was_fighting, was_in);
      }

      if (!WAITLESS(ch)) {
        WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
      }
      return;
    }
  }
  go_flee(ch);
}

const char *FleeDirs[] = {"север",
                          "восток",
                          "юг",
                          "запад",
                          "вверх",
                          "вниз",
                          "\n"};

void do_flee(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  int direction = -1;
  if (!ch->get_fighting()) {
    send_to_char("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
    return;
  }
  if (can_use_feat(ch, CALMNESS_FEAT) || GET_GOD_FLAG(ch, GF_GODSLIKE)) {
    one_argument(argument, arg);
    if ((direction = search_block(arg, dirs, FALSE)) >= 0 ||
        (direction = search_block(arg, FleeDirs, FALSE)) >= 0) {
      go_dir_flee(ch, direction);
      return;
    }
  }
  go_flee(ch);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
