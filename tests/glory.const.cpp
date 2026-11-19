// Цена вложенных в один стат очков постоянной славы набирается по возрастающей: первое очко
// 1000, каждое следующее на 200 дороже. Эта же формула стоит в цене покупки очка в редакторе
// (add_stat_cost/remove_stat_cost), поэтому сумма вложенного обязана считаться по ней же.
// Печать "слава2 информация" когда-то считала по-своему -- складывала цену только последнего
// очка -- и показывала игроку меньше, чем вложено (при трёх очках 3400 вместо 3600). Тест
// держит обе формулы вместе.

#include <gtest/gtest.h>

#include "gameplay/mechanics/glory_const.h"

namespace glory_const_test {

// Ожидаемая сумма в замкнутом виде: 1000n + 100n(n-1).
int ExpectedCost(int points) {
	return 1000 * points + 100 * points * (points - 1);
}

TEST(GloryConst_StatCost, NoPointsCostNothing) {
	EXPECT_EQ(0, GloryConst::glory_in_stat(0));
}

TEST(GloryConst_StatCost, NegativeAmountCostsNothing) {
	// Нулевые и отрицательные записи в списке статов встречались как след старой ошибки;
	// сумма от них не должна уезжать в минус.
	EXPECT_EQ(0, GloryConst::glory_in_stat(-3));
}

TEST(GloryConst_StatCost, FirstPointCostsThousand) {
	EXPECT_EQ(1000, GloryConst::glory_in_stat(1));
}

TEST(GloryConst_StatCost, EachNextPointCostsTwoHundredMore) {
	EXPECT_EQ(1000 + 1200, GloryConst::glory_in_stat(2));
	EXPECT_EQ(1000 + 1200 + 1400, GloryConst::glory_in_stat(3));
}

TEST(GloryConst_StatCost, MatchesClosedForm) {
	for (int points = 0; points <= 40; ++points) {
		EXPECT_EQ(ExpectedCost(points), GloryConst::glory_in_stat(points)) << "очков: " << points;
	}
}

}  // namespace glory_const_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
