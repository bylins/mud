#include "strangle.h"
#include "chopoff.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.hpp"
#include "fightsystem/common.h"
#include "handler.h"
#include "spells.h"
#include "random.hpp"
#include "protect.h"
#include "skills_info.h"

using namespace FightSystem;

void go_strangle(CHAR_DATA *ch, CHAR_DATA *vict) {
  if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || dontCanAct(ch)) {
    send_to_char("Сейчас у вас не получится выполнить этот прием.\r\n", ch);
    return;
  }

  if (ch->get_fighting()) {
    send_to_char("Вы не можете делать это в бою!\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("Вам стоит встать на ноги.\r\n", ch);
    return;
  }

  vict = try_protect(vict, ch);
  if (!pk_agro_action(ch, vict)) {
    return;
  }

  act("Вы попытались накинуть удавку на шею $N2.\r\n", FALSE, ch, nullptr, vict, TO_CHAR);

  int prob = CalcCurrentSkill(ch, SKILL_STRANGLE, vict);
  int delay = 6 - MIN(4, (ch->get_skill(SKILL_STRANGLE) + 30) / 50);
  int percent = number(1, skill_info[SKILL_STRANGLE].difficulty);

  bool success = percent <= prob;
  TrainSkill(ch, SKILL_STRANGLE, success, vict);
  SendSkillBalanceMsg(ch, skill_info[SKILL_STRANGLE].name, percent, prob, success);
  if (!success) {
    Damage dmg(SkillDmg(SKILL_STRANGLE), ZERO_DMG, PHYS_DMG);
    dmg.flags.set(IGNORE_ARMOR);
    dmg.process(ch, vict);
    //set_wait(ch, 3, TRUE);
    setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 3);
  } else {
    AFFECT_DATA<EApplyLocation> af;
    af.type = SPELL_STRANGLE;
    af.duration = IS_NPC(vict) ? 8 : 15;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.battleflag = AF_SAME_TIME;
    af.bitvector = to_underlying(EAffectFlag::AFF_STRANGLED);
    affect_to_char(vict, af);

    int dam = (GET_MAX_HIT(vict) * GaussIntNumber((300 + 5 * ch->get_skill(SKILL_STRANGLE)) / 70, 7.0, 1, 30)) / 100;
    dam = (IS_NPC(vict) ? MIN(dam, 6 * GET_MAX_HIT(ch)) : MIN(dam, 2 * GET_MAX_HIT(ch)));
    Damage dmg(SkillDmg(SKILL_STRANGLE), dam, PHYS_DMG);
    dmg.flags.set(IGNORE_ARMOR);
    dmg.process(ch, vict);
    if (GET_POS(vict) > POS_DEAD) {
      set_wait(vict, 2, TRUE);
      //vict->setSkillCooldown(SKILL_GLOBAL_COOLDOWN, 2);
      if (vict->ahorse()) {
        act("Рванув на себя, $N стащил$G Вас на землю.", FALSE, vict, nullptr, ch, TO_CHAR);
        act("Рванув на себя, Вы стащили $n3 на землю.", FALSE, vict, nullptr, ch, TO_VICT);
        act("Рванув на себя, $N стащил$G $n3 на землю.", FALSE, vict, nullptr, ch, TO_NOTVICT | TO_ARENA_LISTEN);
        vict->drop_from_horse();
      }
      if (ch->get_skill(SKILL_CHOPOFF) && ch->isInSameRoom(vict)) {
        go_chopoff(ch, vict);
      }
      setSkillCooldownInFight(ch, SKILL_GLOBAL_COOLDOWN, 2);
    }
  }

  timed_type timed;
  timed.skill = SKILL_STRANGLE;
  timed.time = delay;
  timed_to_char(ch, &timed);
}

void do_strangle(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  if (!ch->get_skill(SKILL_STRANGLE)) {
    send_to_char("Вы не умеете этого.\r\n", ch);
    return;
  }

  if (timed_by_skill(ch, SKILL_STRANGLE) || ch->haveCooldown(SKILL_STRANGLE)) {
    send_to_char("Так часто душить нельзя - человеки кончатся.\r\n", ch);
    return;
  }

  CHAR_DATA *vict = findVictim(ch, argument);
  if (!vict) {
    send_to_char("Кого вы жаждете удавить?\r\n", ch);
    return;
  }

  if (AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)) {
    send_to_char("Ваша жертва хватается руками за горло - не подобраться!\r\n", ch);
    return;
  }

  if (IS_UNDEAD(vict)
      || GET_RACE(vict) == NPC_RACE_FISH
      || GET_RACE(vict) == NPC_RACE_PLANT
      || GET_RACE(vict) == NPC_RACE_THING) {
    send_to_char("Вы бы еще верстовой столб удавить попробовали...\r\n", ch);
    return;
  }

  if (vict == ch) {
    send_to_char("Воспользуйтесь услугами княжеского палача. Постоянным клиентам - скидки!\r\n", ch);
    return;
  }

  if (!may_kill_here(ch, vict, argument)) {
    return;
  }
  if (!check_pkill(ch, vict, arg)) {
    return;
  }
  go_strangle(ch, vict);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
