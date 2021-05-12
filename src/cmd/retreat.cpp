#include "retreat.h"
#include "fightsystem/fight.h"

// ***************** STOPFIGHT
void do_retreat(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  if (!ch->get_fighting() || IS_NPC(ch)) {
    send_to_char("Но вы же ни с кем не сражаетесь.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Из этого положения отступать невозможно.\r\n", ch);
    return;
  }

  if (PRF_FLAGS(ch).get(PRF_IRON_WIND) || AFF_FLAGGED(ch, EAffectFlag::AFF_LACKY)) {
    send_to_char("Вы не желаете отступать, не расправившись со всеми врагами!\r\n", ch);
    return;
  }

  CHAR_DATA *tmp_ch = nullptr;
  for (const auto i : world[ch->in_room]->people) {
    if (i->get_fighting() == ch) {
      tmp_ch = i;
      break;
    }
  }

  if (tmp_ch) {
    send_to_char("Невозможно, вы сражаетесь за свою жизнь.\r\n", ch);
    return;
  } else {
    stop_fighting(ch, TRUE);
    if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
      WAIT_STATE(ch, PULSE_VIOLENCE);
    send_to_char("Вы отступили из битвы.\r\n", ch);
    act("$n выбыл$g из битвы.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
  }
}