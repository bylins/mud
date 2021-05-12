#include "mighthit.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/common.h"
#include "skills/parry.h"
#include "spells.h"
#include "protect.h"

using namespace FightSystem;

// ************************* MIGHTHIT PROCEDURES
void go_mighthit(CHAR_DATA *ch, CHAR_DATA *victim) {
  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
    send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
    return;
  }

  victim = try_protect(victim, ch);

  if (!ch->get_fighting()) {
    SET_AF_BATTLE(ch, EAF_MIGHTHIT);
    hit(ch, victim, SKILL_MIGHTHIT, FightSystem::MAIN_HAND);
    if (ch->getSkillCooldown(SKILL_MIGHTHIT) > 0) {
      setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
    }
    //set_wait(ch, 2, TRUE);
    return;
  }

  if ((victim->get_fighting() != ch) && (ch->get_fighting() != victim)) {
    act("$N не сражается с вами, не трогайте $S.", FALSE, ch, 0, victim, TO_CHAR);
  } else {
    act("Вы попытаетесь нанести богатырский удар по $N2.", FALSE, ch, 0, victim, TO_CHAR);
    if (ch->get_fighting() != victim) {
      stop_fighting(ch, 2);
      set_fighting(ch, victim);
      setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 2);
      //set_wait(ch, 2, TRUE);
    }
    SET_AF_BATTLE(ch, EAF_MIGHTHIT);
  }
}

void do_mighthit(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (ch->get_skill(SKILL_MIGHTHIT) < 1) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_MIGHTHIT)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кого вы хотите СИЛЬНО ударить?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Вы СИЛЬНО ударили себя. Но вы и не спали.\r\n", ch);
    return;
  }

  if (GET_AF_BATTLE(ch, EAF_TOUCH)) {
    if (!IS_NPC(ch))
      send_to_char("Невозможно. Вы сосредоточены на захвате противника.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
      && (GET_EQ(ch, WEAR_BOTHS)
          || GET_EQ(ch, WEAR_WIELD)
          || GET_EQ(ch, WEAR_HOLD)
          || GET_EQ(ch, WEAR_SHIELD)
          || GET_EQ(ch, WEAR_LIGHT))) {
    send_to_char("Ваша экипировка мешает вам нанести удар.\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  parry_override(ch);

  go_mighthit(ch, vict);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
