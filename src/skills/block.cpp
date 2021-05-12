#include "block.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight_hit.hpp"
#include "spells.h"

using namespace FightSystem;

// ******************* BLOCK PROCEDURES
void go_block(CHAR_DATA *ch) {
  if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT)) {
    send_to_char("Ваша рука парализована.\r\n", ch);
    return;
  }
  SET_AF_BATTLE(ch, EAF_BLOCK);
  send_to_char("Хорошо, вы попробуете отразить щитом следующую атаку.\r\n", ch);
}

void do_block(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch) || !ch->get_skill(SKILL_BLOCK)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_BLOCK)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };
  if (!ch->get_fighting()) {
    send_to_char("Но вы ни с кем не сражаетесь!\r\n", ch);
    return;
  };
  if (!(IS_NPC(ch)
      || GET_EQ(ch, WEAR_SHIELD)
      || IS_IMMORTAL(ch)
      || GET_GOD_FLAG(ch, GF_GODSLIKE))) {
    send_to_char("Вы не можете сделать это без щита.\r\n", ch);
    return;
  }
  if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
    send_to_char("Вы уже прикрываетесь щитом!\r\n", ch);
    return;
  }
  go_block(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
