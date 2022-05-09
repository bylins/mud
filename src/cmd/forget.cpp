#include "forget.h"

#include "game_magic/spells_info.h"
#include "game_mechanics/mem_queue.h"
#include "handler.h"
#include "color.h"

inline bool in_mem(char *arg) {
	return (strlen(arg) != 0) &&
		(!strn_cmp("часослов", arg, strlen(arg)) ||
			!strn_cmp("резы", arg, strlen(arg)) || !strn_cmp("book", arg, strlen(arg)));
}

void do_forget(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char *s = nullptr, *t = nullptr;
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
	// get: blank, spell name, target name
	if (IS_IMMORTAL(ch)) {
		SendMsgToChar("Господи, тебе лень набрать skillset?\r\n", ch);
		return;
	}
	s = strtok(argument, "'*!");
	if (s == nullptr) {
		SendMsgToChar("Какое заклинание вы хотите забыть?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (s == nullptr) {
		SendMsgToChar("Название заклинания должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}

	auto spell_id = FixNameAndFindSpellId(s);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались таких выражений?\r\n", ch);
		return;
	}

	if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
		SendMsgToChar("Трудно забыть то, чего не знаешь...\r\n", ch);
		return;
	}
	t = strtok(nullptr, "\0");
	is_in_mem = 0;
	if (t != nullptr) {
		one_argument(t, arg);
		is_in_mem = in_mem(arg);
	}
	if (!is_in_mem)
		if (!GET_SPELL_MEM(ch, spell_id)) {
			SendMsgToChar("Прежде чем забыть что-то ненужное, следует заучить что-то ненужное...\r\n", ch);
			return;
		} else {
			--GET_SPELL_MEM(ch, spell_id);
			ch->caster_level -= spell_info[spell_id].danger;
			sprintf(buf, "Вы удалили заклинание '%s%s%s' из %s.\r\n",
					KICYN, spell_info[spell_id].name,
					KNRM, GET_RELIGION(ch) == kReligionMono ? "своего часослова" : "своих рез");
			SendMsgToChar(buf, ch);
		}
	else
		MemQ_forget(ch, spell_id);
}
