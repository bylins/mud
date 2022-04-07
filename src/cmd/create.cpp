#include "create.h"

#include "crafts/im.h"
#include "game_magic/spells.h"
#include "comm.h"
#include "game_magic/magic_utils.h"
#include "handler.h"

void do_create(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char *s;
	int spellnum, itemnum = 0;

	if (ch->is_npc())
		return;

	// get: blank, spell name, target name
	argument = one_argument(argument, arg);

	if (!*arg) {
		if (subcmd == SCMD_RECIPE)
			SendMsgToChar("Состав ЧЕГО вы хотите узнать?\r\n", ch);
		else
			SendMsgToChar("ЧТО вы хотите создать?\r\n", ch);
		return;
	}

	size_t i = strlen(arg);
	if (!strn_cmp(arg, "potion", i) || !strn_cmp(arg, "напиток", i))
		itemnum = kSpellPotion;
	else if (!strn_cmp(arg, "wand", i) || !strn_cmp(arg, "палочка", i))
		itemnum = kSpellWand;
	else if (!strn_cmp(arg, "scroll", i) || !strn_cmp(arg, "свиток", i))
		itemnum = kSpellScroll;
	else if (!strn_cmp(arg, "recipe", i) || !strn_cmp(arg, "рецепт", i) ||
		!strn_cmp(arg, "отвар", i)) {
		if (subcmd != SCMD_RECIPE) {
			SendMsgToChar("Магическую смесь необходимо СМЕШАТЬ.\r\n", ch);
			return;
		}
//		itemnum = SPELL_ITEMS;
		compose_recipe(ch, argument, 0);
		return;
	} else if (!strn_cmp(arg, "runes", i) || !strn_cmp(arg, "руны", i)) {
		if (subcmd != SCMD_RECIPE) {
			SendMsgToChar("Руны требуется сложить.\r\n", ch);
			return;
		}
		itemnum = kSpellRunes;
	} else {
		if (subcmd == SCMD_RECIPE)
			snprintf(buf, kMaxInputLength, "Состав '%s' уже давно утерян.\r\n", arg);
		else
			snprintf(buf, kMaxInputLength, "Создание '%s' доступно только Великим Богам.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}

	s = strtok(argument, "'*!");
	if (s == nullptr) {
		sprintf(buf, "Уточните тип состава!\r\n");
		SendMsgToChar(buf, ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (s == nullptr) {
		SendMsgToChar("Название состава должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	spellnum = FixNameAndFindSpellNum(s);

	// Unknown spell
	if (spellnum < 1 || spellnum > kSpellCount) {
		SendMsgToChar("И откуда вы набрались рецептов?\r\n", ch);
		return;
	}

	// Caster is don't know this recipe
	if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), itemnum) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Было бы неплохо прежде всего выучить этот состав.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_RECIPE) {
		CheckRecipeValues(ch, spellnum, itemnum, true);
		return;
	}

	if (!CheckRecipeValues(ch, spellnum, itemnum, false)) {
		SendMsgToChar("Боги хранят в тайне этот состав.\r\n", ch);
		return;
	}

	if (!CheckRecipeItems(ch, spellnum, itemnum, true)) {
		SendMsgToChar("У вас нет нужных ингредиентов!\r\n", ch);
		return;
	}
}
