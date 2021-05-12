#include "mixture.h"

#include "spells.h"
#include "magic.utils.hpp"
#include "handler.h"
#include "privilege.hpp"
#include "spells.info.h"

void do_mixture(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd) {
  if (IS_NPC(ch))
    return;
  if (IS_IMMORTAL(ch) && !Privilege::check_flag(ch, Privilege::USE_SKILLS)) {
    send_to_char("Не положено...\r\n", ch);
    return;
  }

  CHAR_DATA *tch;
  OBJ_DATA *tobj;
  ROOM_DATA *troom;
  char *s, *t;
  int spellnum, target = 0;

  // get: blank, spell name, target name
  s = strtok(argument, "'*!");
  if (!s) {
    if (subcmd == SCMD_RUNES)
      send_to_char("Что вы хотите сложить?\r\n", ch);
    else if (subcmd == SCMD_ITEMS)
      send_to_char("Что вы хотите смешать?\r\n", ch);
    return;
  }
  s = strtok(NULL, "'*!");
  if (!s) {
    send_to_char("Название вызываемой магии смеси должно быть заключено в символы : ' или * или !\r\n", ch);
    return;
  }
  t = strtok(NULL, "\0");

  spellnum = fix_name_and_find_spell_num(s);

  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char("И откуда вы набрались рецептов?\r\n", ch);
    return;
  }

  if (((!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_ITEMS)
      && subcmd == SCMD_ITEMS)
      || (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_RUNES)
          && subcmd == SCMD_RUNES)) && !IS_GOD(ch)) {
    send_to_char("Это блюдо вам явно не понравится.\r\n" "Научитесь его правильно готовить.\r\n", ch);
    return;
  }

  if (!check_recipe_values(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, FALSE))
    return;

  if (!check_recipe_items(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, FALSE)) {
    if (subcmd == SCMD_ITEMS)
      send_to_char("У вас нет нужных ингредиентов!\r\n", ch);
    else if (subcmd == SCMD_RUNES)
      send_to_char("У вас нет нужных рун!\r\n", ch);
    return;
  }

  // Find the target
  if (t != NULL)
    one_argument(t, arg);
  else
    *arg = '\0';

  target = find_cast_target(spellnum, arg, ch, &tch, &tobj, &troom);

  if (target && (tch == ch) && spell_info[spellnum].violent) {
    send_to_char("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
    return;
  }

  if (!target) {
    send_to_char("Тяжеловато найти цель для вашей магии!\r\n", ch);
    return;
  }

  if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(spell_info[spellnum].targets, TAR_SELF_ONLY)) {
    send_to_char("Вы можете колдовать это только на себя!\r\n", ch);
    return;
  }

  if (IS_MANA_CASTER(ch)) {
    if (GET_LEVEL(ch) < spell_create_level(ch, spellnum)) {
      send_to_char("Вы еще слишком малы, чтобы колдовать такое.\r\n", ch);
      return;
    }

    if (GET_MANA_STORED(ch) < GET_MANA_COST(ch, spellnum)) {
      send_to_char("У вас маловато магической энергии!\r\n", ch);
      return;
    } else {
      GET_MANA_STORED(ch) = GET_MANA_STORED(ch) - GET_MANA_COST(ch, spellnum);
    }
  }

  if (check_recipe_items(ch, spellnum, subcmd == SCMD_ITEMS ? SPELL_ITEMS : SPELL_RUNES, TRUE, tch)) {
    if (!spell_use_success(ch, tch, SAVING_NONE, spellnum)) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      if (!tch || !SendSkillMessages(0, ch, tch, spellnum)) {
        if (subcmd == SCMD_ITEMS)
          send_to_char("Вы неправильно смешали ингредиенты!\r\n", ch);
        else if (subcmd == SCMD_RUNES)
          send_to_char("Вы не смогли правильно истолковать значение рун!\r\n", ch);
      }
    } else {
      if (call_magic(ch, tch, tobj, world[ch->in_room], spellnum, GET_LEVEL(ch)) >= 0) {
        if (!(WAITLESS(ch) || CHECK_WAIT(ch)))
          WAIT_STATE(ch, PULSE_VIOLENCE);
      }
    }
  }
}
