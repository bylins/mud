#include "create.h"

#include "im.h"
#include "spells.h"
#include "comm.h"
#include "magic.utils.hpp"
#include "handler.h"

void do_create(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd) {
  char *s;
  int spellnum, itemnum = 0;

  if (IS_NPC(ch))
    return;

  // get: blank, spell name, target name
  argument = one_argument(argument, arg);

  if (!*arg) {
    if (subcmd == SCMD_RECIPE)
      send_to_char("Состав ЧЕГО вы хотите узнать?\r\n", ch);
    else
      send_to_char("ЧТО вы хотите создать?\r\n", ch);
    return;
  }

  size_t i = strlen(arg);
  if (!strn_cmp(arg, "potion", i) || !strn_cmp(arg, "напиток", i))
    itemnum = SPELL_POTION;
  else if (!strn_cmp(arg, "wand", i) || !strn_cmp(arg, "палочка", i))
    itemnum = SPELL_WAND;
  else if (!strn_cmp(arg, "scroll", i) || !strn_cmp(arg, "свиток", i))
    itemnum = SPELL_SCROLL;
  else if (!strn_cmp(arg, "recipe", i) || !strn_cmp(arg, "рецепт", i) ||
      !strn_cmp(arg, "отвар", i)) {
    if (subcmd != SCMD_RECIPE) {
      send_to_char("Магическую смесь необходимо СМЕШАТЬ.\r\n", ch);
      return;
    }
//		itemnum = SPELL_ITEMS;
    compose_recipe(ch, argument, 0);
    return;
  } else if (!strn_cmp(arg, "runes", i) || !strn_cmp(arg, "руны", i)) {
    if (subcmd != SCMD_RECIPE) {
      send_to_char("Руны требуется сложить.\r\n", ch);
      return;
    }
    itemnum = SPELL_RUNES;
  } else {
    if (subcmd == SCMD_RECIPE)
      snprintf(buf, MAX_INPUT_LENGTH, "Состав '%s' уже давно утерян.\r\n", arg);
    else
      snprintf(buf, MAX_INPUT_LENGTH, "Создание '%s' доступно только Великим Богам.\r\n", arg);
    send_to_char(buf, ch);
    return;
  }

  s = strtok(argument, "'*!");
  if (s == NULL) {
    sprintf(buf, "Уточните тип состава!\r\n");
    send_to_char(buf, ch);
    return;
  }
  s = strtok(NULL, "'*!");
  if (s == NULL) {
    send_to_char("Название состава должно быть заключено в символы : ' или * или !\r\n", ch);
    return;
  }

  spellnum = fix_name_and_find_spell_num(s);

  // Unknown spell
  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char("И откуда вы набрались рецептов?\r\n", ch);
    return;
  }

  // Caster is don't know this recipe
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), itemnum) && !IS_IMMORTAL(ch)) {
    send_to_char("Было бы неплохо прежде всего выучить этот состав.\r\n", ch);
    return;
  }

  if (subcmd == SCMD_RECIPE) {
    check_recipe_values(ch, spellnum, itemnum, TRUE);
    return;
  }

  if (!check_recipe_values(ch, spellnum, itemnum, FALSE)) {
    send_to_char("Боги хранят в тайне этот состав.\r\n", ch);
    return;
  }

  if (!check_recipe_items(ch, spellnum, itemnum, TRUE)) {
    send_to_char("У вас нет нужных ингредиентов!\r\n", ch);
    return;
  }
}
