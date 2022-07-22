#include "create.h"

#include "game_crafts/im.h"
#include "game_magic/magic_utils.h"
#include "handler.h"

void do_create(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char *s;

	if (ch->IsNpc())
		return;

	// get: blank, spell name, target name
	argument = one_argument(argument, arg);
	skip_spaces(&argument);

	if (!*arg) {
		if (subcmd == SCMD_RECIPE)
			SendMsgToChar("Состав ЧЕГО вы хотите узнать?\r\n", ch);
		else
			SendMsgToChar("ЧТО вы хотите создать?\r\n", ch);
		return;
	}

	size_t i = strlen(arg);
	ESpellType itemnum;
	if (!strn_cmp(arg, "potion", i) || !strn_cmp(arg, "напиток", i))
		itemnum = ESpellType::kPotionCast;
	else if (!strn_cmp(arg, "wand", i) || !strn_cmp(arg, "палочка", i))
		itemnum = ESpellType::kWandCast;
	else if (!strn_cmp(arg, "scroll", i) || !strn_cmp(arg, "свиток", i))
		itemnum = ESpellType::kScrollCast;
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
		itemnum = ESpellType::kRunes;
	} else {
		if (subcmd == SCMD_RECIPE)
			snprintf(buf, kMaxInputLength, "Состав '%s' уже давно утерян.\r\n", arg);
		else
			snprintf(buf, kMaxInputLength, "Создание '%s' доступно только Великим Богам.\r\n", arg);
		SendMsgToChar(buf, ch);
		return;
	}
	if (!*argument) {
		sprintf(buf, "Уточните тип состава!\r\n");
		SendMsgToChar(buf, ch);
		return;
	}
	s = strtok(argument, "'*!");
	if (!str_cmp(s, argument)) {
		SendMsgToChar("Название состава должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	auto spell_id = FixNameAndFindSpellId(s);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались рецептов?\r\n", ch);
		return;
	}
	// Caster is don't know this recipe
	if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), itemnum) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Было бы неплохо прежде всего выучить этот состав.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_RECIPE) {
		CheckRecipeValues(ch, spell_id, itemnum, true);
		return;
	}
	if (!CheckRecipeValues(ch, spell_id, itemnum, false)) {
		SendMsgToChar("Боги хранят в тайне этот состав.\r\n", ch);
		return;
	}
	if (!CheckRecipeItems(ch, spell_id, itemnum, true)) {
		SendMsgToChar("У вас нет нужных ингредиентов!\r\n", ch);
		return;
	}
}
