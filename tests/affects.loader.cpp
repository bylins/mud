// issue.affects-improve (P2): affects.xml now carries each affect's own stat-change applies
// (<apply location><modifier></apply>), parsed by AffectsLoader into the per-affect table read back
// via affects::AffectApplies(). These tests pin that parse round-trip (inline XML, no world boot;
// mirrors guilds.loader.cpp). The impose path does not read this yet (that is P3).

#include <gtest/gtest.h>

#include "gameplay/affects/affects_loader.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/affects/affect_contants.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>

namespace {
const char *kAffectsSrc = "affects_applies_src.xml";
void WriteAffects() {
	std::ofstream f(kAffectsSrc);
	// kBlind: two applies (one full formula incl. cap/stack/random, one minimal).
	// kSilence: flags only, no <apply> -> AffectApplies must be empty.
	f << R"(<affects>)"
	     R"(<affect id="kBlind" buff="N"><flags val="kAfBattledec"/>)"
	     R"(<apply location="kHitroll" random="Y"><modifier min="2.0" beta="1.5" weight="0.3" factor="-1" cap="20" stack="3"/></apply>)"
	     R"(<apply location="kAc"><modifier min="10.0" beta="0" factor="1"/></apply>)"
	     R"(</affect>)"
	     R"(<affect id="kSilence" buff="N"><flags val="kAfBattledec"/></affect>)"
	     R"(</affects>)";
}
} // namespace

TEST(AffectsLoader_Applies, ParsesPerAffectApplyListAndFormula) {
	WriteAffects();
	parser_wrapper::DataNode doc(kAffectsSrc);
	affects::AffectsLoader loader;
	loader.Load(doc);

	const auto &blind = affects::AffectApplies(EAffect::kBlind);
	ASSERT_EQ(blind.size(), 2u);

	EXPECT_EQ(blind[0].location, EApply::kHitroll);
	EXPECT_DOUBLE_EQ(blind[0].min, 2.0);
	EXPECT_DOUBLE_EQ(blind[0].beta, 1.5);
	EXPECT_DOUBLE_EQ(blind[0].weight, 0.3);  // issue.potency-noise: replaces dices_weight/alpha
	EXPECT_EQ(blind[0].factor, -1);
	EXPECT_EQ(blind[0].cap, 20);
	EXPECT_EQ(blind[0].stack, 3);
	EXPECT_TRUE(blind[0].random);

	EXPECT_EQ(blind[1].location, EApply::kAc);
	EXPECT_DOUBLE_EQ(blind[1].min, 10.0);
	EXPECT_EQ(blind[1].factor, 1);
	EXPECT_EQ(blind[1].cap, 0);     // absent -> default
	EXPECT_EQ(blind[1].stack, 1);   // absent -> default
	EXPECT_FALSE(blind[1].random);  // absent -> default

	// A flags-only affect has no applies.
	EXPECT_TRUE(affects::AffectApplies(EAffect::kSilence).empty());

	std::remove(kAffectsSrc);
}
