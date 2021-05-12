#include "parry.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/common.h"
#include "spells.h"

using namespace FightSystem;

// **************** MULTYPARRY PROCEDURES
void go_multyparry(CHAR_DATA *ch) {
  if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  SET_AF_BATTLE(ch, EAF_MULTYPARRY);
  send_to_char("Вы попробуете использовать веерную защиту.\r\n", ch);
}

void do_multyparry(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch) || !ch->get_skill(SKILL_MULTYPARRY)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_MULTYPARRY)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };
  if (!ch->get_fighting()) {
    send_to_char("Но вы ни с кем не сражаетесь?\r\n", ch);
    return;
  }

  OBJ_DATA *primary = GET_EQ(ch, WEAR_WIELD), *offhand = GET_EQ(ch, WEAR_HOLD);
  if (!(IS_NPC(ch)
      || (primary
          && GET_OBJ_TYPE(primary) == OBJ_DATA::ITEM_WEAPON
          && offhand
          && GET_OBJ_TYPE(offhand) == OBJ_DATA::ITEM_WEAPON)
      || IS_IMMORTAL(ch)
      || GET_GOD_FLAG(ch, GF_GODSLIKE))) {
    send_to_char("Вы не можете отражать атаки безоружным.\r\n", ch);
    return;
  }
  if (GET_AF_BATTLE(ch, EAF_STUPOR)) {
    send_to_char("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
    return;
  }
  go_multyparry(ch);
}

// **************** PARRY PROCEDURES
void go_parry(CHAR_DATA *ch) {
  if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  SET_AF_BATTLE(ch, EAF_PARRY);
  send_to_char("Вы попробуете отклонить следующую атаку.\r\n", ch);
}

void do_parry(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch) || !ch->get_skill(SKILL_PARRY)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_PARRY)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  if (!ch->get_fighting()) {
    send_to_char("Но вы ни с кем не сражаетесь?\r\n", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, GF_GODSLIKE)) {
    if (GET_EQ(ch, WEAR_BOTHS)) {
      send_to_char("Вы не можете отклонить атаку двуручным оружием.\r\n", ch);
      return;
    }

    bool prim = 0, offh = 0;
    if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON) {
      prim = 1;
    }
    if (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON) {
      offh = 1;
    }

    if (!prim && !offh) {
      send_to_char("Вы не можете отклонить атаку безоружным.\r\n", ch);
      return;
    } else if (!prim || !offh) {
      send_to_char("Вы можете отклонить атаку только с двумя оружиями в руках.\r\n", ch);
      return;
    }
  }

  if (GET_AF_BATTLE(ch, EAF_STUPOR)) {
    send_to_char("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
    return;
  }
  go_parry(ch);
}

void parry_override(CHAR_DATA *ch) {
  std::string message = "";
  if (GET_AF_BATTLE(ch, EAF_BLOCK)) {
    message = "Вы прекратили прятаться за щит и бросились в бой.";
    CLR_AF_BATTLE(ch, EAF_BLOCK);
  }
  if (GET_AF_BATTLE(ch, EAF_PARRY)) {
    message = "Вы прекратили парировать атаки и бросились в бой.";
    CLR_AF_BATTLE(ch, EAF_PARRY);
  }
  if (GET_AF_BATTLE(ch, EAF_MULTYPARRY)) {
    message = "Вы забыли о защите и бросились в бой.";
    CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
  }
  act(message.c_str(), FALSE, ch, 0, 0, TO_CHAR);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :