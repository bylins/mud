// issue #3847: the delay of timed feats from the <timed> tag in cfg/feats.xml.

#include "gameplay/abilities/feat_timer.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>

namespace {

using feats::FeatTimer;

const char *kTimerSrc = "feat_timer_test.xml";

std::optional<FeatTimer> ParseTimer(const std::string &feat_body) {
	{
		std::ofstream f(kTimerSrc);
		f << R"(<feats><feat id="kBerserker">)" << feat_body << "</feat></feats>";
	}
	parser_wrapper::DataNode doc(kTimerSrc);
	std::optional<FeatTimer> result;
	for (auto &feat : doc.Children("feat")) {
		result = FeatTimer::Parse(feat, EFeat::kBerserker);
	}
	std::remove(kTimerSrc);
	return result;
}

FeatTimer MakeTimer(int base, int min, int divisor) {
	FeatTimer timer;
	timer.has_base = true;
	timer.base = base;
	timer.min = min;
	timer.remort_divisor = divisor;
	return timer;
}

} // namespace

TEST(FeatTimer, NoTag) {
	EXPECT_FALSE(ParseTimer("").has_value());
}

TEST(FeatTimer, Marker) {
	const auto timer = ParseTimer("<timed/>");
	ASSERT_TRUE(timer.has_value());
	EXPECT_FALSE(timer->has_base);
}

TEST(FeatTimer, Defaults) {
	const auto timer = ParseTimer(R"(<timed base="6"/>)");
	ASSERT_TRUE(timer.has_value());
	EXPECT_TRUE(timer->has_base);
	EXPECT_EQ(timer->base, 6);
	EXPECT_EQ(timer->min, 1);
	EXPECT_EQ(timer->remort_divisor, 0);
}

TEST(FeatTimer, AllAttributes) {
	const auto timer = ParseTimer(R"(<timed base="18" min="3" remort_divisor="1"/>)");
	ASSERT_TRUE(timer.has_value());
	EXPECT_EQ(timer->base, 18);
	EXPECT_EQ(timer->min, 3);
	EXPECT_EQ(timer->remort_divisor, 1);
}

TEST(FeatTimer, InvalidTagIsDropped) {
	for (const auto *tag : {R"(<timed base="0"/>)", R"(<timed base="256"/>)", R"(<timed base="5" min="6"/>)",
							R"(<timed base="5" min="0"/>)", R"(<timed base="5" remort_divisor="-1"/>)",
							R"(<timed min="2"/>)", R"(<timed remort_divisor="2"/>)", R"(<timed base="long"/>)"}) {
		EXPECT_FALSE(ParseTimer(tag).has_value()) << tag;
	}
	EXPECT_TRUE(ParseTimer(R"(<timed base="255" min="255"/>)").has_value());
}

TEST(FeatTimer, DelayWithoutRemorts) {
	const auto timer = MakeTimer(24, 1, 0);
	EXPECT_EQ(timer.CalcDelay(0), 24);
	EXPECT_EQ(timer.CalcDelay(100), 24);
}

TEST(FeatTimer, RemortsShortenDownToMin) {
	const auto timer = MakeTimer(12, 4, 3);
	EXPECT_EQ(timer.CalcDelay(0), 12);
	EXPECT_EQ(timer.CalcDelay(2), 12);
	EXPECT_EQ(timer.CalcDelay(3), 11);
	EXPECT_EQ(timer.CalcDelay(24), 4);
	EXPECT_EQ(timer.CalcDelay(100), 4);
	EXPECT_EQ(timer.CalcDelay(-3), 12);
}

// The shipped feats.xml marks exactly the feats that had a delay in code, with the same values.
TEST(FeatTimer, ShippedFileMatchesLegacyDelays) {
	parser_wrapper::DataNode doc(std::filesystem::path("cfg/feats.xml"));
	std::map<EFeat, FeatTimer> timers;
	for (auto &feat : doc.Children("feat")) {
		const auto id = ITEM_BY_NAME<EFeat>(feat.GetValue("id"));
		if (const auto timer = FeatTimer::Parse(feat, id)) {
			timers.emplace(id, *timer);
		}
	}
	ASSERT_EQ(timers.size(), 5u) << "cfg/feats.xml not found or wrong set of timed feats";
	ASSERT_TRUE(timers.contains(EFeat::kSpellCapabler));
	EXPECT_FALSE(timers.at(EFeat::kSpellCapabler).has_base);

	const std::map<EFeat, int> fixed = {{EFeat::kBerserker, 4}, {EFeat::kLightWalk, 24}, {EFeat::kShadowThrower, 6}};
	for (int remorts = 0; remorts <= 100; ++remorts) {
		for (const auto &[id, delay] : fixed) {
			ASSERT_TRUE(timers.contains(id)) << NAME_BY_ITEM<EFeat>(id);
			ASSERT_TRUE(timers.at(id).has_base) << NAME_BY_ITEM<EFeat>(id);
			EXPECT_EQ(timers.at(id).CalcDelay(remorts), delay) << NAME_BY_ITEM<EFeat>(id) << ", remorts " << remorts;
		}
		ASSERT_TRUE(timers.contains(EFeat::kRelocate));
		EXPECT_EQ(timers.at(EFeat::kRelocate).CalcDelay(remorts), 18 - std::min(remorts, 15)) << "remorts " << remorts;
	}
}

TEST(FeatTimer, Describe) {
	EXPECT_EQ(MakeTimer(24, 1, 0).Describe(), "24 h");
	EXPECT_EQ(MakeTimer(18, 3, 1).Describe(), "18 h, -1 h per 1 remorts, at least 3 h");
	EXPECT_EQ(FeatTimer{}.Describe(), "set in code");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
