#include "backstab.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/start.fight.h"
#include "handler.h"
#include "protect.h"
#include "skills_info.h"

using namespace FightSystem;

// делегат обработки команды заколоть
void do_backstab(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (ch->get_skill(SKILL_BACKSTAB) < 1) {
    send_to_char("Вы не знаете как.\r\n", ch);
    return;
  }
  if (ch->haveCooldown(SKILL_BACKSTAB)) {
    send_to_char("Вам нужно набраться сил.\r\n", ch);
    return;
  };

  if (ch->ahorse()) {
    send_to_char("Верхом это сделать затруднительно.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Вам стоит встать на ноги.\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  CHAR_DATA *vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  if (!vict) {
    send_to_char("Кого вы так сильно ненавидите, что хотите заколоть?\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Вы, определенно, садомазохист!\r\n", ch);
    return;
  }

  if (!GET_EQ(ch, WEAR_WIELD) && (!IS_NPC(ch) || IS_CHARMICE(ch))) {
    send_to_char("Требуется держать оружие в правой руке.\r\n", ch);
    return;
  }

  if ((!IS_NPC(ch) || IS_CHARMICE(ch)) && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != type_pierce) {
    send_to_char("ЗаКОЛоть можно только КОЛющим оружием!\r\n", ch);
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || dontCanAct(ch)) {
    send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
    return;
  }

  if (vict->get_fighting() && !can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
    send_to_char("Ваша цель слишком быстро движется - вы можете пораниться!\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument))
    return;
  if (!check_pkill(ch, vict, arg))
    return;
  go_backstab(ch, vict);
}

// *********************** BACKSTAB VICTIM
// Проверка на стаб в бою происходит до вызова этой функции
void go_backstab(CHAR_DATA *ch, CHAR_DATA *vict) {
  int percent, prob;

  if (ch->isHorsePrevents())
    return;

  vict = try_protect(vict, ch);

  if (!pk_agro_action(ch, vict))
    return;

  if ((MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) && !IS_GOD(ch)) {
    act("Вы заметили, что $N попытал$u вас заколоть!", FALSE, vict, nullptr, ch, TO_CHAR);
    act("$n заметил$g вашу попытку заколоть $s!", FALSE, vict, nullptr, ch, TO_VICT);
    act("$n заметил$g попытку $N1 заколоть $s!", FALSE, vict, nullptr, ch, TO_NOTVICT | TO_ARENA_LISTEN);
    set_hit(vict, ch);
    return;
  }

  percent = number(1, skill_info[SKILL_BACKSTAB].difficulty);
  prob = CalcCurrentSkill(ch, SKILL_BACKSTAB, vict);

  if (can_use_feat(ch, SHADOW_STRIKE_FEAT)) {
    prob = prob + prob * 20 / 100;
  };

  if (vict->get_fighting())
    prob = prob * (GET_REAL_DEX(ch) + 50) / 100;

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE))
    prob += 5;
  if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOOB_REGEN))
    prob += 5;
  if (GET_MOB_HOLD(vict))
    prob = prob * 5 / 4;
  if (GET_GOD_FLAG(vict, GF_GODSCURSE))
    prob = percent;
  if (GET_GOD_FLAG(vict, GF_GODSLIKE) || GET_GOD_FLAG(ch, GF_GODSCURSE))
    prob = 0;
  bool success = percent <= prob;
  TrainSkill(ch, SKILL_BACKSTAB, success, vict);

  SendSkillBalanceMsg(ch, skill_info[SKILL_BACKSTAB].name, percent, prob, success);
  if (!success) {
    Damage dmg(SkillDmg(SKILL_BACKSTAB), ZERO_DMG, PHYS_DMG);
    dmg.process(ch, vict);
  } else {
    hit(ch, vict, SKILL_BACKSTAB, FightSystem::MAIN_HAND);
  }
  setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 1);
  setSkillCooldownInFight(ch, SKILL_BACKSTAB, 2);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
