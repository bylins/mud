#include "disarm.h"

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

// ************* DISARM PROCEDURES
void go_disarm(CHAR_DATA *ch, CHAR_DATA *vict) {
  OBJ_DATA *wielded = GET_EQ(vict, WEAR_WIELD) ? GET_EQ(vict, WEAR_WIELD) :
                      GET_EQ(vict, WEAR_BOTHS), *helded = GET_EQ(vict, WEAR_HOLD);

  if (dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  if (!((wielded && GET_OBJ_TYPE(wielded) != OBJ_DATA::ITEM_LIGHT)
      || (helded && GET_OBJ_TYPE(helded) != OBJ_DATA::ITEM_LIGHT))) {
    return;
  }
  int pos = 0;
  if (number(1, 100) > 30) {
    pos = wielded ? (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD) : WEAR_HOLD;
  } else {
    pos = helded ? WEAR_HOLD : (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD);
  }

  if (!pos || !GET_EQ(vict, pos))
    return;
  if (!pk_agro_action(ch, vict))
    return;
  int percent = number(1, skill_info[SKILL_DISARM].difficulty);
  int prob = CalcCurrentSkill(ch, SKILL_DISARM, vict);
  if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_GOD_FLAG(ch, GF_GODSLIKE))
    prob = percent;
  if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSLIKE)
      || can_use_feat(vict, STRONGCLUTCH_FEAT))
    prob = 0;

  bool success = percent <= prob;
  TrainSkill(ch, SKILL_DISARM, success, vict);
  SendSkillBalanceMsg(ch, skill_info[SKILL_DISARM].name, percent, prob, success);
  if (!success || GET_EQ(vict, pos)->get_extra_flag(EExtraFlag::ITEM_NODISARM)) {
    send_to_char(ch, "%sВы не сумели обезоружить %s...%s\r\n", CCWHT(ch, C_NRM), GET_PAD(vict, 3), CCNRM(ch, C_NRM));
    prob = 3;
  } else {
    wielded = GET_EQ(vict, pos);
    send_to_char(ch, "%sВы ловко выбили %s из рук %s!%s\r\n",
                 CCIBLU(ch, C_NRM), wielded->get_PName(3).c_str(), GET_PAD(vict, 1), CCNRM(ch, C_NRM));
    send_to_char(vict, "Ловкий удар %s выбил %s%s из ваших рук.\r\n",
                 GET_PAD(ch, 1), wielded->get_PName(3).c_str(), char_get_custom_label(wielded, vict).c_str());
    act("$n ловко выбил$g $o3 из рук $N1.", TRUE, ch, wielded, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    unequip_char(vict, pos);
    setSkillCooldown(ch, SKILL_GLOBAL_COOLDOWN, IS_NPC(vict) ? 1 : 2);
    prob = 2;

    if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA) || (!IS_MOB(vict)) || vict->has_master()) {
      obj_to_char(wielded, vict);
    } else {
      obj_to_room(wielded, IN_ROOM(vict));
      obj_decay(wielded);
    };
  }

  appear(ch);
  if (IS_NPC(vict) && CAN_SEE(vict, ch) && vict->have_mind() && GET_WAIT(ch) <= 0) {
    set_hit(vict, ch);
  }
  setSkillCooldown(ch, SKILL_DISARM, prob);
  setSkillCooldown(ch, SKILL_GLOBAL_COOLDOWN, 1);
}

void do_disarm(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (IS_NPC(ch) || !ch->get_skill(SKILL_DISARM)) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_DISARM)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кого обезоруживаем?\r\n", ch);
    return;
  }

  if (ch == vict) {
    send_to_char("Попробуйте набрать \"снять <название.оружия>\".\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  if (!((GET_EQ(vict, WEAR_WIELD)
      && GET_OBJ_TYPE(GET_EQ(vict, WEAR_WIELD)) != OBJ_DATA::ITEM_LIGHT)
      || (GET_EQ(vict, WEAR_HOLD)
          && GET_OBJ_TYPE(GET_EQ(vict, WEAR_HOLD)) != OBJ_DATA::ITEM_LIGHT)
      || (GET_EQ(vict, WEAR_BOTHS)
          && GET_OBJ_TYPE(GET_EQ(vict, WEAR_BOTHS)) != OBJ_DATA::ITEM_LIGHT))) {
    send_to_char("Вы не можете обезоружить безоружное создание.\r\n", ch);
    return;
  }

  if (IS_IMPL(ch) || !ch->get_fighting()) {
    go_disarm(ch, vict);
  } else if (isHaveNoExtraAttack(ch)) {
    act("Хорошо. Вы попытаетесь разоружить $N3.", FALSE, ch, 0, vict, TO_CHAR);
    ch->set_extra_attack(EXTRA_ATTACK_DISARM, vict);
  }
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
