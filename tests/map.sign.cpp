#include <gtest/gtest.h>

#include "engine/ui/mapsystem.h"

// Клетка карты -- строка вида "&K - &n": цвет, видимая часть, сброс. Сброс после каждой
// клетки разворачивается на проводе в семь байт, а клеток в карте сотни: у имма в городе
// карта весила под шесть килобайт на шаг. Разбор клетки на части позволяет печатать цвет
// только при смене, а сброс -- один раз в конце строки.

TEST(MapSign, SplitsColorAndGlyph) {
	EXPECT_EQ("&K", MapSystem::SignColor("&K - &n"));
	EXPECT_EQ(" - ", MapSystem::SignGlyph("&K - &n"));

	EXPECT_EQ("&C", MapSystem::SignColor("&C-=-&n"));
	EXPECT_EQ("-=-", MapSystem::SignGlyph("&C-=-&n"));

	EXPECT_EQ("&R", MapSystem::SignColor("&R|&n"));
	EXPECT_EQ("|", MapSystem::SignGlyph("&R|&n"));
}

TEST(MapSign, KeepsColorlessCellAsIs) {
	EXPECT_TRUE(MapSystem::SignColor("+").empty());
	EXPECT_EQ("+", MapSystem::SignGlyph("+"));
}

TEST(MapSign, EmptyCellGivesNothing) {
	EXPECT_TRUE(MapSystem::SignColor("").empty());
	EXPECT_TRUE(MapSystem::SignGlyph("").empty());
}

TEST(MapSign, DropsEveryTrailingReset) {
	EXPECT_EQ("*", MapSystem::SignGlyph("&G*&n&n"));
}

TEST(MapSign, RowOfSameColorCostsOneColorAndOneReset) {
	// Двадцать клеток одного цвета: было по цвету и сбросу на каждую, стало один цвет
	// на строку и один сброс в конце
	const std::string_view sign = "&K:&n";
	const std::size_t before = 20 * sign.size();

	std::string row;
	std::string_view color;
	for (int i = 0; i < 20; ++i) {
		if (MapSystem::SignColor(sign) != color) {
			color = MapSystem::SignColor(sign);
			row += color;
		}
		row += MapSystem::SignGlyph(sign);
	}
	row += "&n";

	EXPECT_EQ(100u, before);
	EXPECT_EQ(24u, row.size());
	EXPECT_EQ("&K::::::::::::::::::::&n", row);
}
