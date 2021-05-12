#include "forget.h"

#include "spells.info.h"
#include "handler.h"
#include "im.h"
#include "screen.h"

inline bool in_mem(char *arg) {
  return (strlen(arg) != 0) &&
      (!strn_cmp("часослов", arg, strlen(arg)) ||
          !strn_cmp("резы", arg, strlen(arg)) || !strn_cmp("book", arg, strlen(arg)));
}

void do_forget(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
  char *s = 0, *t = 0;
  int spellnum, is_in_mem;

  // проверка на аргумент рецепт|отвар
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Что вы хотите забыть?\r\n", ch);
    return;
  }

  size_t i = strlen(arg);
  if (!strn_cmp(arg, "recipe", i) || !strn_cmp(arg, "рецепт", i) ||
      !strn_cmp(arg, "отвар", i)) {
    forget_recipe(ch, argument, 0);
    return;
  }

  if (!strn_cmp(arg, "все", i) || !strn_cmp(arg, "all", i)) {
    char arg2[MAX_INPUT_LENGTH];
    two_arguments(argument, arg, arg2);
    if (in_mem(arg2)) {
      MemQ_flush(ch);
      send_to_char("Вы вычеркнули все заклинания из своего списка для запоминания.\r\n", ch);
    } else {
      for (i = 1; i <= MAX_SPELLS; i++) {
        GET_SPELL_MEM(ch, i) = 0;
      }
      sprintf(buf,
              "Вы удалили все заклинания из %s.\r\n",
              GET_RELIGION(ch) == RELIGION_MONO ? "своего часослова" : "своих рез");
      send_to_char(buf, ch);
    }
    return;
  }
  // get: blank, spell name, target name
  if (IS_IMMORTAL(ch)) {
    send_to_char("Господи, тебе лень набрать skillset?\r\n", ch);
    return;
  }
  s = strtok(argument, "'*!");
  if (s == NULL) {
    send_to_char("Какое заклинание вы хотите забыть?\r\n", ch);
    return;
  }
  s = strtok(NULL, "'*!");
  if (s == NULL) {
    send_to_char("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
    return;
  }
  spellnum = fix_name_and_find_spell_num(s);
  // Unknown spell
  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char("И откуда вы набрались таких выражений?\r\n", ch);
    return;
  }
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP)) {
    send_to_char("Трудно забыть то, чего не знаешь...\r\n", ch);
    return;
  }
  t = strtok(NULL, "\0");
  is_in_mem = 0;
  if (t != NULL) {
    one_argument(t, arg);
    is_in_mem = in_mem(arg);
  }
  if (!is_in_mem)
    if (!GET_SPELL_MEM(ch, spellnum)) {
      send_to_char("Прежде чем забыть что-то ненужное, следует заучить что-то ненужное...\r\n", ch);
      return;
    } else {
      GET_SPELL_MEM(ch, spellnum)--;
      GET_CASTER(ch) -= spell_info[spellnum].danger;
      sprintf(buf, "Вы удалили заклинание '%s%s%s' из %s.\r\n",
              KICYN, spell_info[spellnum].name,
              KNRM, GET_RELIGION(ch) == RELIGION_MONO ? "своего часослова" : "своих рез");
      send_to_char(buf, ch);
    }
  else
    MemQ_forget(ch, spellnum);
}
