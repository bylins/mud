// Процентный бонус к урону (`percent_physdam_add`, `percent_spellpower_add`) работает в обе
// стороны. Физ. урон раньше учитывал процент, только если тот больше нуля, так что белена,
// покалеченная конечность и костяная дань физ. урон не резали вовсе (#4095).

#include <gtest/gtest.h>

#include "gameplay/fight/common.h"

namespace damage_percent_test {

constexpr int kRolls = 2000;

TEST(RollDamagePercent, BonusRollsOneToTwicePercent) {
	// +25%: от +1% до +50%.
	int min_seen = 1000;
	int max_seen = 0;
	for (int i = 0; i < kRolls; ++i) {
		const int dam = RollDamagePercent(100, 25);
		min_seen = std::min(min_seen, dam);
		max_seen = std::max(max_seen, dam);
	}
	EXPECT_GE(min_seen, 101);
	EXPECT_LE(max_seen, 150);
	EXPECT_LT(min_seen, 110) << "бросок не доходит до низа";
	EXPECT_GT(max_seen, 140) << "бросок не доходит до верха";
}

TEST(RollDamagePercent, PenaltyMirrorsBonus) {
	// -20%: от -1% до -40%.
	int min_seen = 1000;
	int max_seen = 0;
	for (int i = 0; i < kRolls; ++i) {
		const int dam = RollDamagePercent(100, -20);
		min_seen = std::min(min_seen, dam);
		max_seen = std::max(max_seen, dam);
	}
	EXPECT_GE(min_seen, 60);
	EXPECT_LE(max_seen, 99);
	EXPECT_LT(min_seen, 65) << "бросок не доходит до низа";
	EXPECT_GT(max_seen, 95) << "бросок не доходит до верха";
}

TEST(RollDamagePercent, AveragesAboutPercent) {
	long long sum = 0;
	for (int i = 0; i < kRolls; ++i) {
		sum += RollDamagePercent(1000, -30);
	}
	EXPECT_NEAR(static_cast<double>(sum) / kRolls, 700.0, 15.0);
}

TEST(RollDamagePercent, ZeroPercentKeepsDamage) {
	for (const int dam : {0, 1, 17, 1000}) {
		EXPECT_EQ(RollDamagePercent(dam, 0), dam);
	}
}

TEST(RollDamagePercent, NeverGoesBelowZero) {
	// Минусы складываются (белена + конечность + дань), а бросок удваивает процент.
	for (int i = 0; i < kRolls; ++i) {
		EXPECT_GE(RollDamagePercent(100, -80), 0);
	}
	EXPECT_EQ(RollDamagePercent(0, -50), 0);
}

TEST(AverageDamagePercent, AppliesExactPercent) {
	EXPECT_EQ(AverageDamagePercent(100, 25), 125);
	EXPECT_EQ(AverageDamagePercent(40, 10), 44);
	EXPECT_EQ(AverageDamagePercent(100, -20), 80);
	EXPECT_EQ(AverageDamagePercent(150, -30), 105);
	EXPECT_EQ(AverageDamagePercent(17, 0), 17);
}

TEST(AverageDamagePercent, NeverGoesBelowZero) {
	EXPECT_EQ(AverageDamagePercent(100, -100), 0);
	EXPECT_EQ(AverageDamagePercent(100, -150), 0);
}

}   // namespace damage_percent_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
