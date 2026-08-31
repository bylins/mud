// Команда "статус" со своим набором букв (issue: статус все/нет работали, свой набор -- нет).
//
// Аргумент разбирался побайтно, а под UTF-8 русская буква занимает два байта: после каждой
// буквы шаг в один байт приводил к её хвостовому байту, тот не совпадал ни с одной меткой и
// уводил разбор в default -- команда печатала справку и не ставила ничего. Латиница из одного
// байта при этом работала, поэтому баг выглядел как "русские буквы не принимаются".

#include "engine/ui/cmd/do_display.h"

#include "simulator/character_builder.h"
#include "engine/entities/char_data.h"
#include "gameplay/core/constants.h"

#include <gtest/gtest.h>

#include <cstring>

namespace {

CharData::shared_ptr MakePlayer(simulator::CharacterBuilder &builder) {
	builder.make_basic_player(static_cast<short>(ECharClass::kSorcerer), 30);
	return builder.get();
}

void RunStatus(const CharData::shared_ptr &ch, const char *argument) {
	char buffer[64];
	strncpy(buffer, argument, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	do_display(ch.get(), buffer, 0, 0);
}

}  // namespace

TEST(StatusCommand, CyrillicLettersSetTheirOwnFlags) {
	simulator::CharacterBuilder builder;
	auto ch = MakePlayer(builder);

	RunStatus(ch, "жзв");

	EXPECT_TRUE(ch->IsFlagged(EPrf::kDispHp)) << "Ж -- жизнь";
	EXPECT_TRUE(ch->IsFlagged(EPrf::kDispMana)) << "З -- запас сил";
	EXPECT_TRUE(ch->IsFlagged(EPrf::kDispExits)) << "В -- выходы";
	EXPECT_FALSE(ch->IsFlagged(EPrf::kDispMoney)) << "Д не набирали";
}

TEST(StatusCommand, LatinLettersStillWork) {
	simulator::CharacterBuilder builder;
	auto ch = MakePlayer(builder);

	RunStatus(ch, "hw");

	EXPECT_TRUE(ch->IsFlagged(EPrf::kDispHp));
	EXPECT_TRUE(ch->IsFlagged(EPrf::kDispMana));
	EXPECT_FALSE(ch->IsFlagged(EPrf::kDispExits));
}

TEST(StatusCommand, UnknownLetterStopsParsing) {
	simulator::CharacterBuilder builder;
	auto ch = MakePlayer(builder);

	RunStatus(ch, "жяв");

	// Поведение как было: неизвестная буква печатает справку и обрывает разбор, но то, что
	// успело примениться до неё, остаётся.
	EXPECT_TRUE(ch->IsFlagged(EPrf::kDispHp)) << "Ж до неизвестной буквы";
	EXPECT_FALSE(ch->IsFlagged(EPrf::kDispExits)) << "В после неё уже не разбирается";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
