#include "learn.h"

#include "handler.h"
#include "classes/spell.slots.hpp"
#include "skills_info.h"
#include "spells.info.h"

class OBJ_DATA;

void book_upgrd_fail_message(CHAR_DATA *ch, OBJ_DATA *obj) {
  send_to_char(ch, "Изучив %s от корки до корки вы так и не узнали ничего нового.\r\n",
               obj->get_PName(3).c_str());
  act("$n с интересом принял$u читать $o3.\r\n"
      "Постепенно $s интерес начал угасать, и $e, плюясь, сунул$g $o3 обратно.",
      FALSE, ch, obj, 0, TO_ROOM);
}

void do_learn(CHAR_DATA *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
  using PlayerClass::slot_for_char;

  OBJ_DATA *obj;
  int spellnum = 0, addchance = 10, rcpt = -1;
  im_rskill *rs = NULL;
  const char *spellname = "";

  const char *stype0[] =
      {
          "секрет",
          "еще несколько секретов"
      };

  const char *stype1[] =
      {
          "заклинание",
          "умение",
          "умение",
          "рецепт",
          "рецепт",
          "способность"
      };

  const char *stype2[] =
      {
          "заклинания",
          "умения",
          "умения",
          "рецепта",
          "способности"
      };

  if (IS_NPC(ch))
    return;

  // get: blank, spell name, target name
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Вы принялись внимательно изучать свои ногти. Да, пора бы и подстричь.\r\n", ch);
    act("$n удивленно уставил$u на свои ногти. Подстриг бы их кто-нибудь $m.",
        FALSE,
        ch,
        0,
        0,
        TO_ROOM | TO_ARENA_LISTEN);
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    send_to_char("А у вас этого нет.\r\n", ch);
    return;
  }

  if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_BOOK) {
    act("Вы уставились на $o3, как баран на новые ворота.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n начал$g внимательно изучать устройство $o1.", FALSE, ch, obj, 0, TO_ROOM);
    return;
  }

  if (GET_OBJ_VAL(obj, 0) != BOOK_SPELL && GET_OBJ_VAL(obj, 0) != BOOK_SKILL &&
      GET_OBJ_VAL(obj, 0) != BOOK_UPGRD && GET_OBJ_VAL(obj, 0) != BOOK_RECPT &&
      GET_OBJ_VAL(obj, 0) != BOOK_FEAT) {
    act("НЕВЕРНЫЙ ТИП КНИГИ - сообщите Богам!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (GET_OBJ_VAL(obj, 0) == BOOK_SPELL
      && slot_for_char(ch, 1) <= 0) {
    send_to_char("Далась вам эта магия! Пошли-бы, водочки выпили...\r\n", ch);
    return;
  }

  if (GET_OBJ_VAL(obj, 2) < 1
      && GET_OBJ_VAL(obj, 0) != BOOK_UPGRD
      && GET_OBJ_VAL(obj, 0) != BOOK_SPELL
      && GET_OBJ_VAL(obj, 0) != BOOK_FEAT
      && GET_OBJ_VAL(obj, 0) != BOOK_RECPT) {
    send_to_char("НЕКОРРЕКТНЫЙ УРОВЕНЬ - сообщите Богам!\r\n", ch);
    return;
  }

  if (GET_OBJ_VAL(obj, 0) == BOOK_RECPT) {
    rcpt = im_get_recipe(GET_OBJ_VAL(obj, 1));
  }

  if ((GET_OBJ_VAL(obj, 0) == BOOK_SKILL
      || GET_OBJ_VAL(obj, 0) == BOOK_UPGRD)
      && GET_OBJ_VAL(obj, 1) < 1
      && GET_OBJ_VAL(obj, 1) > MAX_SKILL_NUM) {
    send_to_char("УМЕНИЕ НЕ ОПРЕДЕЛЕНО - сообщите Богам!\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(obj, 0) == BOOK_RECPT
      && rcpt < 0) {
    send_to_char("РЕЦЕПТ НЕ ОПРЕДЕЛЕН - сообщите Богам!\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(obj, 0) == BOOK_SPELL
      && (GET_OBJ_VAL(obj, 1) < 1
          || GET_OBJ_VAL(obj, 1) > SPELLS_COUNT)) {
    send_to_char("МАГИЯ НЕ ОПРЕДЕЛЕНА - сообщите Богам!\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(obj, 0) == BOOK_FEAT
      && (GET_OBJ_VAL(obj, 1) < 1
          || GET_OBJ_VAL(obj, 1) >= MAX_FEATS)) {
    send_to_char("СПОСОБНОСТЬ НЕ ОПРЕДЕЛЕНА - сообщите Богам!\r\n", ch);
    return;
  }

  //	skill_info[GET_OBJ_VAL(obj, 1)].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] == KNOW_SKILL)
  if (GET_OBJ_VAL(obj, 0) == BOOK_SKILL && can_get_skill_with_req(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2))) {
    spellnum = GET_OBJ_VAL(obj, 1);
    spellname = skill_info[spellnum].name;
  } else if (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD && ch->get_trained_skill(static_cast<ESkill>(GET_OBJ_VAL(obj, 1)))) {
    spellnum = GET_OBJ_VAL(obj, 1);
    spellname = skill_info[spellnum].name;
  } else if (GET_OBJ_VAL(obj, 0) == BOOK_SPELL
      && can_get_spell_with_req(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2))) {
    spellnum = GET_OBJ_VAL(obj, 1);
    spellname = spell_info[spellnum].name;
  } else if (GET_OBJ_VAL(obj, 0) == BOOK_RECPT
      && imrecipes[rcpt].classknow[(int) GET_CLASS(ch)] == KNOW_RECIPE
      && MAX(GET_OBJ_VAL(obj, 2), imrecipes[rcpt].level) <= GET_LEVEL(ch) && imrecipes[rcpt].remort <= GET_REMORT(ch)) {
    spellnum = rcpt;
    rs = im_get_char_rskill(ch, spellnum);
    spellname = imrecipes[spellnum].name;
    if (imrecipes[spellnum].level == -1 || imrecipes[spellnum].remort == -1) {
      send_to_char("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
      return;
    }
  } else if (GET_OBJ_VAL(obj, 0) == BOOK_FEAT && can_get_feat(ch, GET_OBJ_VAL(obj, 1))) {
    spellnum = GET_OBJ_VAL(obj, 1);
    spellname = feat_info[spellnum].name;
  }

  if ((GET_OBJ_VAL(obj, 0) == BOOK_SKILL && ch->get_skill(static_cast<ESkill>(spellnum)))
      || (GET_OBJ_VAL(obj, 0) == BOOK_SPELL && GET_SPELL_TYPE(ch, spellnum) & SPELL_KNOW)
      || (GET_OBJ_VAL(obj, 0) == BOOK_FEAT && HAVE_FEAT(ch, spellnum))
      || (GET_OBJ_VAL(obj, 0) == BOOK_RECPT && rs)) {
    sprintf(buf, "Вы открыли %s и принялись с интересом\r\n"
                 "изучать. Каким же было разочарование, когда прочитав %s,\r\n"
                 "Вы поняли, что это %s \"%s\".\r\n",
            obj->get_PName(3).c_str(),
            number(0, 1) ? "несколько абзацев" :
            number(0, 1) ? "пару строк" : "почти до конца",
            stype1[GET_OBJ_VAL(obj, 0)],
            spellname);
    send_to_char(buf, ch);
    act("$n с интересом принял$u читать $o3.\r\n"
        "Постепенно $s интерес начал угасать, и $e, плюясь, сунул$g $o3 обратно.",
        FALSE, ch, obj, 0, TO_ROOM);
    return;
  }

  if (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD) {
    // апгрейд скилла без учета макс.скилла плеера (до макс в книге)
    if (GET_OBJ_VAL(obj, 3) > 0
        && ch->get_trained_skill(static_cast<ESkill>(spellnum)) >= GET_OBJ_VAL(obj, 3)) {
      book_upgrd_fail_message(ch, obj);
      return;
    }

    // апгрейд скилла до макс.скилла плеера (без макса в книге)
    if (GET_OBJ_VAL(obj, 3) <= 0
        && ch->get_trained_skill(static_cast<ESkill>(spellnum)) >= kSkillCapOnZeroRemort + GET_REMORT(ch) * 5) {
      book_upgrd_fail_message(ch, obj);
      return;
    }
  }

  if (!spellnum) {
    const char *where = number(0, 1) ? "вон та" : (number(0, 1) ? "вот эта" : "пятая справа");
    const char *what = number(0, 1) ? "жука" : (number(0, 1) ? "бабочку" : "русалку");
    const char *whom = obj->get_sex() == ESex::SEX_FEMALE ? "нее" : (obj->get_sex() == ESex::SEX_POLY ? "них" : "него");
    sprintf(buf,
            "- \"Какие интересные буковки ! Особенно %s, похожая на %s\".\r\n"
            "Полюбовавшись еще несколько минут на сию красоту, вы с чувством выполненного\r\n"
            "долга закрыли %s. До %s вы еще не доросли.\r\n",
            where, what, obj->get_PName(3).c_str(), whom);
    send_to_char(buf, ch);
    act("$n с интересом осмотрел$g $o3, крякнул$g от досады и положил$g обратно.", FALSE, ch, obj, 0, TO_ROOM);
    return;
  }

  addchance = (IS_CLERIC(ch) && ROOM_FLAGGED(ch->in_room, ROOM_CLERIC)) ||
      (IS_MAGE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_MAGE)) ||
      (IS_PALADINE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_PALADINE)) ||
      (IS_THIEF(ch) && ROOM_FLAGGED(ch->in_room, ROOM_THIEF)) ||
      (IS_ASSASINE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_ASSASINE)) ||
      (IS_WARRIOR(ch) && ROOM_FLAGGED(ch->in_room, ROOM_WARRIOR)) ||
      (IS_RANGER(ch) && ROOM_FLAGGED(ch->in_room, ROOM_RANGER)) ||
      (IS_GUARD(ch) && ROOM_FLAGGED(ch->in_room, ROOM_GUARD)) ||
      (IS_SMITH(ch) && ROOM_FLAGGED(ch->in_room, ROOM_SMITH)) ||
      (IS_DRUID(ch) && ROOM_FLAGGED(ch->in_room, ROOM_DRUID)) ||
      (IS_MERCHANT(ch) && ROOM_FLAGGED(ch->in_room, ROOM_MERCHANT)) ? 10 : 0;
  addchance += (GET_OBJ_VAL(obj, 0) == BOOK_SPELL) ? 0 : 10;

  if (!obj->get_extra_flag(EExtraFlag::ITEM_NO_FAIL)
      && number(1, 100) > int_app[POSI(GET_REAL_INT(ch))].spell_aknowlege + addchance) {
    sprintf(buf, "Вы взяли в руки %s и начали изучать. Непослушные\r\n"
                 "буквы никак не хотели выстраиваться в понятные и доступные фразы.\r\n"
                 "Промучившись несколько минут, вы бросили это унылое занятие,\r\n"
                 "с удивлением отметив исчезновение %s.\r\n", obj->get_PName(3).c_str(), obj->get_PName(1).c_str());
    send_to_char(buf, ch);
  } else {
    sprintf(buf, "Вы взяли в руки %s и начали изучать. Постепенно,\r\n"
                 "незнакомые доселе, буквы стали складываться в понятные слова и фразы.\r\n"
                 "Буквально через несколько минут вы узнали %s %s \"%s\".\r\n",
            obj->get_PName(3).c_str(),
            (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD) ? stype0[1] : stype0[0],
            stype2[GET_OBJ_VAL(obj, 0)],
            spellname);
    send_to_char(buf, ch);
    sprintf(buf,
            "LEARN: Игрок %s выучил %s %s \"%s\"",
            GET_NAME(ch),
            (GET_OBJ_VAL(obj, 0) == BOOK_UPGRD) ? stype0[1] : stype0[0],
            stype2[GET_OBJ_VAL(obj, 0)],
            spellname);
    log("%s", buf);
    switch (GET_OBJ_VAL(obj, 0)) {
      case BOOK_SPELL: GET_SPELL_TYPE(ch, spellnum) |= SPELL_KNOW;
        break;

      case BOOK_SKILL: ch->set_skill(static_cast<ESkill>(spellnum), 1);
        break;

      case BOOK_UPGRD: {
        const ESkill skill = static_cast<ESkill>(spellnum);
        const int left_skill_level = ch->get_trained_skill(static_cast<ESkill>(spellnum)) + GET_OBJ_VAL(obj, 2);
        if (GET_OBJ_VAL(obj, 3) > 0) {
          ch->set_skill(skill, MIN(left_skill_level, GET_OBJ_VAL(obj, 3)));
        } else {
          ch->set_skill(skill, MIN(left_skill_level, kSkillCapOnZeroRemort + GET_REMORT(ch) * 5));
        }
      }
        break;

      case BOOK_RECPT: CREATE(rs, 1);
        rs->rid = spellnum;
        rs->link = GET_RSKILL(ch);
        GET_RSKILL(ch) = rs;
        rs->perc = 5;
        break;
      case BOOK_FEAT: SET_FEAT(ch, spellnum);
        break;
    }
  }
  extract_obj(obj);
}

