#include "do_forget.h"

#include <string>

#include "gameplay/magic/spells_info.h"
#include "gameplay/magic/magic_utils.h"
#include "utils/utils_string.h"
#include "gameplay/mechanics/mem_queue.h"
#include "engine/core/handler.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"

inline bool in_mem(char *arg) {
	return (strlen(arg) != 0) &&
		(!strn_cmp("часослов", arg, strlen(arg)) ||
			!strn_cmp("резы", arg, strlen(arg)) || !strn_cmp("book", arg, strlen(arg)));
}

void do_forget(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int is_in_mem;

	// проверка на аргумент рецепт|отвар
	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar("Что вы хотите забыть?\r\n", ch);
		return;
	}

	size_t i = strlen(arg);
	if (!strn_cmp(arg, "recipe", i) || !strn_cmp(arg, "рецепт", i) ||
		!strn_cmp(arg, "отвар", i)) {
		forget_recipe(ch, argument, 0);
		return;
	}

	if (!strn_cmp(arg, "все", i) || !strn_cmp(arg, "all", i)) {
		char arg2[kMaxInputLength];
		two_arguments(argument, arg, arg2);
		if (in_mem(arg2)) {
			MemQ_flush(ch);
			SendMsgToChar("Вы вычеркнули все заклинания из своего списка для запоминания.\r\n", ch);
		} else {
			for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
				GET_SPELL_MEM(ch, spell_id) = 0;
			}
			sprintf(buf,
					"Вы удалили все заклинания из %s.\r\n",
					GET_RELIGION(ch) == kReligionMono ? "своего часослова" : "своих рез");
			SendMsgToChar(buf, ch);
		}
		return;
	}
	if (ch->IsImmortal()) {
		SendMsgToChar("Господи, тебе лень набрать skillset?\r\n", ch);
		return;
	}
	if (!*argument) {
		SendMsgToChar("Какое заклинание вы хотите забыть?\r\n", ch);
		return;
	}
	std::string arg_str(argument);
	auto quote1 = arg_str.find_first_of("'*!");
	if (quote1 == std::string::npos) {
		SendMsgToChar("Название заклинания должно быть заключено в символы : * или !\r\n", ch);
		return;
	}
	auto quote2 = arg_str.find_first_of("'*!", quote1 + 1);
	std::string spell_name_str;
	if (quote2 != std::string::npos) {
		spell_name_str = arg_str.substr(quote1 + 1, quote2 - quote1 - 1);
	} else {
		spell_name_str = arg_str.substr(quote1 + 1);
	}
	auto spell_id = FixNameAndFindSpellId(spell_name_str);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}
	if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
		SendMsgToChar("Трудно забыть то, чего не знаешь...\r\n", ch);
		return;
	}
	is_in_mem = 0;
	if (quote2 != std::string::npos && quote2 + 1 < arg_str.size()) {
		std::string rest_str = arg_str.substr(quote2 + 1);
		utils::TrimLeft(rest_str);
		one_argument(rest_str.data(), arg);
		is_in_mem = in_mem(arg);
	}
	if (!is_in_mem)
		if (!GET_SPELL_MEM(ch, spell_id)) {
			SendMsgToChar("Прежде чем забыть что-то ненужное, следует заучить что-то ненужное...\r\n", ch);
			return;
		} else {
			--GET_SPELL_MEM(ch, spell_id);
			ch->caster_level -= MUD::Spell(spell_id).GetDanger();
			sprintf(buf, "Вы удалили заклинание '%s%s%s' из %s.\r\n",
					kColorBoldCyn, MUD::Spell(spell_id).GetCName(),
					kColorNrm, GET_RELIGION(ch) == kReligionMono ? "своего часослова" : "своих рез");
			SendMsgToChar(buf, ch);
		}
	else
		MemQ_forget(ch, spell_id);
}
