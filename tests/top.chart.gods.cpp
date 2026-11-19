// Чарт "рекордов" (команда "рекорды <профессия>") не должен держать богов.
//
// Признак бога брался только из privilege::IsImmortal, то есть из базы cfg/privilege.xml.
// А чарт набивается при старте мада, из ActualizePlayersIndex -> TopPlayer::Refresh, и
// база богов к этому моменту еще не прочитана: GetDb() пуста, FindGod() не находит
// никого, IsImmortal() отвечает "нет" про каждого. Единственное исключение -- владелец,
// его опознают по имени, без базы. В итоге в "рекордах" среди игроков стояли боги, а
// владельца там не было. Порядок загрузки поправлен, но проверка по уровню оставлена
// отдельно -- она от порядка не зависит: kMaxPlayerLevel == 30 < kLvlImmortal == 31.
//
// Внутренности TopPlayer закрыты, поэтому смотрим на длину списка по профессии: бог
// ее не меняет, смертный увеличивает на единицу.

#include "char.utilities.hpp"

#include "gameplay/statistics/top.h"
#include "engine/entities/char_data.h"
#include "engine/structs/structs.h"

#include <gtest/gtest.h>

#include <string>

namespace {

constexpr ECharClass kChartClass = ECharClass::kSorcerer;

size_t ChartSize() {
	const auto &chart = TopPlayer::Chart();
	const auto it = chart.find(kChartClass);
	return it == chart.end() ? 0 : it->second.size();
}

// Персонаж, пригодный для чарта: имя, уид, профессия, опыт и уровень.
CharData::shared_ptr MakeChartCandidate(test_utils::CharacterBuilder &builder,
										const std::string &name, int level, int uid) {
	builder.create_new();
	builder.set_class(static_cast<short>(kChartClass));
	builder.set_level(level);
	builder.set_name(name);
	auto ch = builder.get();
	ch->set_uid(uid);
	ch->set_exp(1000);
	return ch;
}

}  // namespace

TEST(TopChart, GodIsNotAddedToPlayerRecords) {
	test_utils::CharacterBuilder builder;
	auto god = MakeChartCandidate(builder, "Тестобог", kLvlGod, 900001);

	const size_t before = ChartSize();
	TopPlayer::Refresh(god.get(), true);
	EXPECT_EQ(before, ChartSize());

	TopPlayer::Remove(god.get());   // на случай, если запись все же появилась
}

// Нижняя граница божеского диапазона -- тоже бог
TEST(TopChart, ImmortalLevelIsNotAddedToPlayerRecords) {
	test_utils::CharacterBuilder builder;
	auto imm = MakeChartCandidate(builder, "Тестоимм", kLvlImmortal, 900002);

	const size_t before = ChartSize();
	TopPlayer::Refresh(imm.get(), true);
	EXPECT_EQ(before, ChartSize());

	TopPlayer::Remove(imm.get());
}

TEST(TopChart, MortalIsAddedToPlayerRecords) {
	test_utils::CharacterBuilder builder;
	auto mortal = MakeChartCandidate(builder, "Тестоигрок", kMaxPlayerLevel, 900003);

	const size_t before = ChartSize();
	TopPlayer::Refresh(mortal.get(), true);
	EXPECT_EQ(before + 1, ChartSize());

	TopPlayer::Remove(mortal.get());
	EXPECT_EQ(before, ChartSize());
}

// Смертного подняли в боги -- место в чарте за ним оставаться не должно
TEST(TopChart, PromotedMortalLosesHisChartEntry) {
	test_utils::CharacterBuilder builder;
	auto ch = MakeChartCandidate(builder, "Тестовознесенный", kMaxPlayerLevel, 900004);

	const size_t before = ChartSize();
	TopPlayer::Refresh(ch.get());
	ASSERT_EQ(before + 1, ChartSize());

	ch->set_level(kLvlGod);
	TopPlayer::Refresh(ch.get());
	EXPECT_EQ(before, ChartSize());

	TopPlayer::Remove(ch.get());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
