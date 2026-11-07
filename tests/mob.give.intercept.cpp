// issue #4161: "дать" из триггера моба идет своим путем (do_mgive) -- без проверки видимости
// получателя. Перехват работает не по точному слову, а через таблицу команд: в мире встречаются
// "дать" (1750 строк), "give" (460), "дат" (144) и "да" (6), и список написаний не наш.
//
// Тест стоит сторожем именно на этом: пока "дать"/"дата" стоят в cmd_info в своем порядке, все
// четыре написания обязаны попадать в перехват, а посторонние команды -- нет. Переставят команды
// местами -- узнаем здесь, а не из жалобы билдера, что награда опять не дошла.

#include <gtest/gtest.h>

#include "engine/ui/interpreter.h"

// Живет в dg_mobcmd.cpp рядом с перехватом -- своего заголовка у команд моба нет.
int FindGiveCommand(const char *word);

namespace mob_give_intercept_test {

TEST(MobGiveIntercept, EverySpellingOfGiveIsCaught) {
	for (const char *word : {"дать", "дат", "да", "give", "giv", "gi"}) {
		EXPECT_GE(FindGiveCommand(word), 0) << "написание \"" << word << "\" не попало в перехват";
	}
}

TEST(MobGiveIntercept, OtherCommandsAreNotCaught) {
	// "дата" -- та самая команда, из-за которой перехват нельзя делать по двум первым буквам.
	for (const char *word : {"дата", "сказать", "передать", "взять", "load", "", "ддд"}) {
		EXPECT_LT(FindGiveCommand(word), 0) << "команда \"" << word << "\" перехвачена напрасно";
	}
}

TEST(MobGiveIntercept, GiveIsStillInTheCommandTable) {
	// Перехват опирается на таблицу: если "дать" из нее уедет, молчать об этом нельзя.
	bool found_rus = false;
	bool found_eng = false;
	for (int cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd) {
		found_rus = found_rus || !strcmp(cmd_info[cmd].command, "дать");
		found_eng = found_eng || !strcmp(cmd_info[cmd].command, "give");
	}
	EXPECT_TRUE(found_rus) << "команда \"дать\" исчезла из cmd_info";
	EXPECT_TRUE(found_eng) << "команда \"give\" исчезла из cmd_info";
}

}  // namespace mob_give_intercept_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
