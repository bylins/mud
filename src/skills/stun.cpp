#include "stun.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/start.fight.h"
#include "handler.h"
#include "spells.h"
#include "skills_info.h"

using namespace FightSystem;

void do_stun(CHAR_DATA *ch, char *argument, int, int) {
  if (ch->get_skill(SKILL_STUN) < 1) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_STUN)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  if (!ch->ahorse()) {
    send_to_char("Вы привстали на стременах и поняли: 'лошадь украли!!!'\r\n", ch);
    return;
  }
  if ((GET_SKILL(ch, SKILL_HORSE) < 151) && (!IS_NPC(ch))) {
    send_to_char("Вы слишком неуверенно управляете лошадью, чтоб на ней пытаться ошеломить противника.\r\n", ch);
    return;
  }
  if (timed_by_skill(ch, SKILL_STUN) && (!IS_NPC(ch))) {
    send_to_char("Ваш грозный вид не испугает даже мышь, попробуйте ошеломить попозже.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !(GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_BOTHS))) {
    send_to_char("Вы должны держать оружие в основной руке.\r\n", ch);
    return;
  }
  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кто это так сильно путается у вас под руками?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Вы БОЛЬНО стукнули себя по голове! 'А еще я туда ем', - подумали вы...\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;

  go_stun(ch, vict);
}

void go_stun(CHAR_DATA *ch, CHAR_DATA *vict) {
  timed_type timed;
  if (GET_SKILL(ch, SKILL_STUN) < 150) {
    ImproveSkill(ch, SKILL_STUN, TRUE, vict);
    timed.skill = SKILL_STUN;
    timed.time = 7;
    timed_to_char(ch, &timed);
    act("У вас не получилось ошеломить $N3, надо больше тренироваться!", FALSE, ch, 0, vict, TO_CHAR);
    act("$N3 попытал$U ошеломить вас, но не получилось.", FALSE, vict, 0, ch, TO_CHAR);
    act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.",
        TRUE,
        ch,
        0,
        vict,
        TO_NOTVICT | TO_ARENA_LISTEN);
    set_hit(ch, vict);
    return;
  }

  timed.skill = SKILL_STUN;
  timed.time = std::clamp(7 - (GET_SKILL(ch, SKILL_STUN) - 150) / 10, 2, 7);
  timed_to_char(ch, &timed);
  //weap_weight = GET_EQ(ch, WEAR_BOTHS)?  GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS)) : GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD));
  //float num = MIN(95, (pow(GET_SKILL(ch, SKILL_STUN), 2) + pow(weap_weight, 2) + pow(GET_REAL_STR(ch), 2)) /
  //(pow(GET_REAL_DEX(vict), 2) + (GET_REAL_CON(vict) - GET_SAVE(vict, SAVING_STABILITY)) * 30.0));

  int percent = number(1, skill_info[SKILL_STUN].difficulty);
  int prob = CalculateCurrentSkill(ch, SKILL_STUN, vict);
  bool success = percent <= prob;
  TrainSkill(ch, SKILL_STUN, success, vict);
  SendSkillBalanceMsg(ch, skill_info[SKILL_STUN].name, percent, prob, success);
  if (!success) {
    act("У вас не получилось ошеломить $N3, надо больше тренироваться!", FALSE, ch, 0, vict, TO_CHAR);
    act("$N3 попытал$U ошеломить вас, но не получилось.", FALSE, vict, 0, ch, TO_CHAR);
    act("$n попытал$u ошеломить $N3, но плохому танцору и тапки мешают.",
        true, ch, nullptr, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    set_hit(ch, vict);
  } else {
    if (GET_EQ(ch, WEAR_BOTHS) && GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS) {
      act("Точным выстрелом вы ошеломили $N3!", FALSE, ch, 0, vict, TO_CHAR);
      act("Точный выстрел $N1 повалил вас с ног и лишил сознания.", FALSE, vict, 0, ch, TO_CHAR);
      act("$n точным выстрелом ошеломил$g $N3!", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    } else {
      act("Мощным ударом вы ошеломили $N3!", FALSE, ch, 0, vict, TO_CHAR);
      act("Ошеломляющий удар $N1 сбил вас с ног и лишил сознания.", FALSE, vict, 0, ch, TO_CHAR);
      act("$n мощным ударом ошеломил$g $N3!", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
    }
    GET_POS(vict) = POS_INCAP;
    WAIT_STATE(vict, (2 + GET_REMORT(ch) / 5) * PULSE_VIOLENCE);
    //WAIT_STATE(ch, (3 * PULSE_VIOLENCE));
    ch->setSkillCooldown(SKILL_STUN, 3 * PULSE_VIOLENCE);
    set_hit(ch, vict);
  }
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
