#include "warcry.h"

#include "handler.h"
#include "screen.h"
#include "magic.utils.hpp"
#include "skills_info.h"
#include "spells.info.h"

#include <boost/tokenizer.hpp>

void do_warcry(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  int spellnum, cnt;

  if (IS_NPC(ch) && AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
    return;

  if (!ch->get_skill(SKILL_WARCRY)) {
    send_to_char("Но вы не знаете как.\r\n", ch);
    return;
  }

  if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
    send_to_char("Вы не смогли вымолвить и слова.\r\n", ch);
    return;
  }

  std::string buffer = argument;
  typedef boost::tokenizer<pred_separator> tokenizer;
  pred_separator sep;
  tokenizer tok(buffer, sep);
  tokenizer::iterator tok_iter = tok.begin();

  if (tok_iter == tok.end()) {
    sprintf(buf, "Вам доступны :\r\n");
    for (cnt = spellnum = 1; spellnum <= SPELLS_COUNT; spellnum++) {
      const char *realname = spell_info[spellnum].name
                                 && *spell_info[spellnum].name ? spell_info[spellnum].name : spell_info[spellnum].syn
                                                                                                 && *spell_info[spellnum].syn
                                                                                             ? spell_info[spellnum].syn
                                                                                             : NULL;

      if (realname
          && IS_SET(spell_info[spellnum].routines, MAG_WARCRY)
          && ch->get_skill(SKILL_WARCRY) >= spell_info[spellnum].mana_change) {
        if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
          continue;
        sprintf(buf + strlen(buf), "%s%2d%s) %s%s%s\r\n",
                KGRN, cnt++, KNRM,
                spell_info[spellnum].violent ? KIRED : KIGRN, realname, KNRM);
      }
    }
    send_to_char(buf, ch);
    return;
  }

  std::string wc_name = *(tok_iter++);

  if (CompareParam(wc_name, "of")) {
    if (tok_iter == tok.end()) {
      send_to_char("Какой клич вы хотите использовать?\r\n", ch);
      return;
    } else
      wc_name = "warcry of " + *(tok_iter++);
  } else
    wc_name = "клич " + wc_name;

  spellnum = fix_name_and_find_spell_num(wc_name);

  // Unknown warcry
  if (spellnum < 1 || spellnum > MAX_SPELLS
      || (ch->get_skill(SKILL_WARCRY) < spell_info[spellnum].mana_change)
      || !IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP)) {
    send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
    return;
  }

  CHAR_DATA *tch;
  OBJ_DATA *tobj;
  ROOM_DATA *troom;
  //афтар сотворил клона функции, а остальное не перевёл :(

  int target = find_cast_target(spellnum, tok_iter == tok.end() ? "" : (*tok_iter).c_str(), ch, &tch, &tobj, &troom);

  if (!target) {
    send_to_char("Тяжеловато найти цель!\r\n", ch);
    return;
  }

  if (tch == ch && spell_info[spellnum].violent) {
    send_to_char("Не накликайте беды!\r\n", ch);
    return;
  }

  if (tch && IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !spell_info[spellnum].violent)
    return;

  if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(spell_info[spellnum].targets, TAR_SELF_ONLY)) {
    send_to_char("Этот клич предназначен только для вас!\r\n", ch);
    return;
  }

  if (tch == ch && IS_SET(spell_info[spellnum].targets, TAR_NOT_SELF)) {
    send_to_char("Да вы с ума сошли!\r\n", ch);
    return;
  }

  struct timed_type timed;
  timed.skill = SKILL_WARCRY;
  timed.time = timed_by_skill(ch, SKILL_WARCRY) + HOURS_PER_WARCRY;

  if (timed.time > HOURS_PER_DAY) {
    send_to_char("Вы охрипли и не можете кричать.\r\n", ch);
    return;
  }

  if (GET_MOVE(ch) < spell_info[spellnum].mana_max) {
    send_to_char("У вас не хватит сил для этого.\r\n", ch);
    return;
  }

  say_spell(ch, spellnum, tch, tobj);

  if (call_magic(ch, tch, tobj, troom, spellnum, GET_LEVEL(ch)) >= 0) {
    if (!WAITLESS(ch)) {
      if (!CHECK_WAIT(ch))
        WAIT_STATE(ch, PULSE_VIOLENCE);
      timed_to_char(ch, &timed);
      GET_MOVE(ch) -= spell_info[spellnum].mana_max;
    }
    TrainSkill(ch, SKILL_WARCRY, true, tch);
  }
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
