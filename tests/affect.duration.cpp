// CalcDuration для игрока считает срок целыми игровыми часами и только потом переводит в тики
// (час = kSecsPerMudHour, тик = kSecsPerPlayerAffect). Отсюда ловушка: бонус меньше делителя
// обнуляется, и аффект с base=0 снимается на первом же тике. Так слетала кража у новичка -- на
// это и стоит нижняя граница min=1 у "подкрасться" и "маскировки".
// Бонус от навыка тут не считаем намеренно: kUndefined обходит GetSkill, которому нужны комната
// и таблицы умений. Проверяется сама арифметика -- границы и единицы измерения.

#include "char.utilities.hpp"

#include <gtest/gtest.h>

#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_contants.h"   // kSecsPerPlayerAffect
#include "gameplay/mechanics/weather.h"         // kSecsPerMudHour
#include "engine/entities/char_data.h"

namespace affect_duration_test {

// Сколько тиков в одном игровом часе: столько даёт срок в один час.
constexpr int kTicksPerHour = kSecsPerMudHour / kSecsPerPlayerAffect;

CharData::shared_ptr MakePlayer() {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	return builder.get();
}

TEST(AffectDuration, NoBaseAndNoFloorGivesZero) {
	auto ch = MakePlayer();
	// Как было у "подкрасться": base=0, нижней границы нет -- при нулевом бонусе срок нулевой,
	// и аффект не живёт ни одного тика.
	EXPECT_EQ(0, CalcDuration(ch.get(), ch.get(), ESkill::kUndefined, 0, 20, 0, 1));
}

TEST(AffectDuration, FloorOfOneHourSurvivesATick) {
	auto ch = MakePlayer();
	// Так стало: min=1 поднимает срок до часа, то есть до kTicksPerHour тиков.
	EXPECT_EQ(kTicksPerHour, CalcDuration(ch.get(), ch.get(), ESkill::kUndefined, 0, 20, 1, 1));
}

TEST(AffectDuration, MaxBoundsTheTotal) {
	auto ch = MakePlayer();
	// Верхняя граница режет итог, а не только бонус: база 5 часов при max=2 даёт два часа.
	EXPECT_EQ(2 * kTicksPerHour, CalcDuration(ch.get(), ch.get(), ESkill::kUndefined, 5, 15, 1, 2));
}

TEST(AffectDuration, BaseWithoutClampsConvertsToTicks) {
	auto ch = MakePlayer();
	// Вызов без делителя и границ -- плоский срок: три часа это 3 * kTicksPerHour тиков.
	EXPECT_EQ(3 * kTicksPerHour, CalcDuration(ch.get(), ch.get(), ESkill::kUndefined, 3, 0, 0, 0));
}

} // namespace affect_duration_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
