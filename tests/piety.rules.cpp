// issue #2689: piety rules (cfg/mechanics/piety.xml) -- what an offering may be, how a worn item loses
// value, and which gift raises piety.

#include "engine/entities/obj_data.h"
#include "gameplay/mechanics/piety.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace {

using piety::EMeasure;
using piety::Gift;

const char *kRulesSrc = "piety_rules_test.xml";

piety::Rules ParseRules(const std::string &xml) {
	{
		std::ofstream f(kRulesSrc);
		f << xml;
	}
	parser_wrapper::DataNode doc(kRulesSrc);
	piety::Rules rules;
	rules.Load(doc);
	std::remove(kRulesSrc);
	return rules;
}

const char *kRules = R"(<piety>
	<sacrifice>
		<items types="kFood" value="none"/>
		<items types="kTreasure" value="cost"/>
		<items types="kArmor|kWeapon" value="ilvl"/>
		<currency id="kKuna" min="10"/>
	</sacrifice>
	<prayer default_currency="kKuna" default_amount="10">
		<items types="kWeapon" value="ilvl"/>
		<currency id="kKuna"/>
		<currency id="kSilverGrivna" min="2"/>
	</prayer>
	<growth fresh_timer="80">
		<tier piety="50" ilvl="10" cost="500"><currency id="kKuna" min="500"/></tier>
		<tier piety="0" ilvl="5"><currency id="kKuna" min="100"/></tier>
	</growth>
</piety>)";

} // namespace

TEST(PietyRules, ReadsOfferings) {
	const auto rules = ParseRules(kRules);
	const auto &sacrifice = rules.Sacrifice();
	EXPECT_EQ(sacrifice.ItemMeasure(EObjType::kFood), EMeasure::kNone);
	EXPECT_EQ(sacrifice.ItemMeasure(EObjType::kTreasure), EMeasure::kCost);
	EXPECT_EQ(sacrifice.ItemMeasure(EObjType::kArmor), EMeasure::kIlvl);
	EXPECT_EQ(sacrifice.ItemMeasure(EObjType::kWeapon), EMeasure::kIlvl);
	EXPECT_FALSE(sacrifice.ItemMeasure(EObjType::kContainer));
	EXPECT_EQ(sacrifice.CurrencyMinimum("kKuna"), 10);
	EXPECT_FALSE(sacrifice.CurrencyMinimum("kGlory"));
	EXPECT_TRUE(sacrifice.default_currency.empty());

	const auto &prayer = rules.Prayer();
	EXPECT_EQ(prayer.default_currency, "kKuna");
	EXPECT_EQ(prayer.default_amount, 10);
	EXPECT_FALSE(prayer.ItemMeasure(EObjType::kFood));
	EXPECT_EQ(prayer.CurrencyMinimum("kKuna"), 1);   // no min -> 1
	EXPECT_EQ(prayer.CurrencyMinimum("kSilverGrivna"), 2);
	EXPECT_DOUBLE_EQ(rules.FreshTimerShare(), 0.8);
}

TEST(PietyRules, TierForPiety) {
	const auto rules = ParseRules(kRules);
	ASSERT_EQ(rules.Tiers().size(), 2u);
	EXPECT_EQ(rules.Tiers()[0].piety, 0);   // sorted
	EXPECT_EQ(rules.TierFor(0)->piety, 0);
	EXPECT_EQ(rules.TierFor(49)->piety, 0);
	EXPECT_EQ(rules.TierFor(50)->piety, 50);
	EXPECT_EQ(rules.TierFor(1000)->piety, 50);
	EXPECT_EQ(rules.TierFor(-1), nullptr);
}

TEST(PietyRules, WorthyGiftReachesTheThreshold) {
	const auto rules = ParseRules(kRules);
	EXPECT_TRUE(rules.IsWorthy(10, Gift{EMeasure::kIlvl, {}, 5.0}));
	EXPECT_FALSE(rules.IsWorthy(10, Gift{EMeasure::kIlvl, {}, 4.9}));
	EXPECT_FALSE(rules.IsWorthy(60, Gift{EMeasure::kIlvl, {}, 9.0}));
	EXPECT_TRUE(rules.IsWorthy(60, Gift{EMeasure::kCurrency, "kKuna", 500.0}));
	EXPECT_FALSE(rules.IsWorthy(60, Gift{EMeasure::kCurrency, "kKuna", 499.0}));
	// A measure the tier does not list never raises piety.
	EXPECT_FALSE(rules.IsWorthy(10, Gift{EMeasure::kCost, {}, 1e9}));
	EXPECT_TRUE(rules.IsWorthy(60, Gift{EMeasure::kCost, {}, 500.0}));
	EXPECT_FALSE(rules.IsWorthy(60, Gift{EMeasure::kCurrency, "kSilverGrivna", 1e9}));
	EXPECT_FALSE(rules.IsWorthy(10, Gift{EMeasure::kNone, {}, 1e9}));
}

TEST(PietyRules, NoTiersNoGrowth) {
	const auto rules = ParseRules("<piety/>");
	EXPECT_EQ(rules.TierFor(100), nullptr);
	EXPECT_FALSE(rules.IsWorthy(100, Gift{EMeasure::kIlvl, {}, 100.0}));
}

// A worn item loses value below the fresh share of its timer, in proportion; without a finite timer
// it keeps it.
TEST(PietyRules, TimerFactor) {
	EXPECT_DOUBLE_EQ(piety::TimerFactor(100, 100, 0.8), 1.0);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(80, 100, 0.8), 1.0);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(40, 100, 0.8), 0.5);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(0, 100, 0.8), 0.0);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(-5, 100, 0.8), 0.0);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(150, 100, 0.8), 1.0);   // a timer above the prototype's
	EXPECT_DOUBLE_EQ(piety::TimerFactor(10, 0, 0.8), 1.0);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(10, ObjData::UNLIMITED_TIMER, 0.8), 1.0);
	EXPECT_DOUBLE_EQ(piety::TimerFactor(ObjData::UNLIMITED_TIMER, 100, 0.8), 1.0);
}

// The shipped rules: a heathen's food only lays the favour, a Christian's prayer without a gift costs a
// 10-kuna candle, both accept equipment and kunas, and every tier asks more than the one before.
TEST(PietyRules, Shipped) {
	parser_wrapper::DataNode doc(std::string("cfg/mechanics/piety.xml"));
	piety::Rules rules;
	rules.Load(doc);
	EXPECT_EQ(rules.Sacrifice().ItemMeasure(EObjType::kFood), EMeasure::kNone);
	EXPECT_EQ(rules.Sacrifice().ItemMeasure(EObjType::kTreasure), EMeasure::kCost);
	for (const auto *offerings : {&rules.Sacrifice(), &rules.Prayer()}) {
		EXPECT_EQ(offerings->ItemMeasure(EObjType::kArmor), EMeasure::kIlvl);
		EXPECT_EQ(offerings->ItemMeasure(EObjType::kWeapon), EMeasure::kIlvl);
		EXPECT_TRUE(offerings->CurrencyMinimum("kKuna"));
	}
	EXPECT_EQ(rules.Prayer().default_currency, "kKuna");
	EXPECT_EQ(rules.Prayer().default_amount, 10);
	EXPECT_TRUE(rules.Sacrifice().default_currency.empty());

	ASSERT_FALSE(rules.Tiers().empty());
	EXPECT_EQ(rules.Tiers().front().piety, 0);
	for (std::size_t i = 1; i < rules.Tiers().size(); ++i) {
		const auto &lower = rules.Tiers()[i - 1];
		const auto &upper = rules.Tiers()[i];
		EXPECT_LT(lower.piety, upper.piety);
		ASSERT_TRUE(lower.ilvl && upper.ilvl);
		EXPECT_LT(*lower.ilvl, *upper.ilvl) << upper.piety;
		EXPECT_LT(lower.currencies.at("kKuna"), upper.currencies.at("kKuna")) << upper.piety;
	}
	// The candle lays the favour but does not raise piety even at the start.
	EXPECT_FALSE(rules.IsWorthy(10, Gift{EMeasure::kCurrency, "kKuna", 10.0}));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
