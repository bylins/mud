// issue #3847: feat categories from the <feat_categories> section of cfg/feats.xml -- how many feats of
// one category (weapon focus, weapon master) a character may hold at once.

#include "gameplay/abilities/feat_categories.h"
#include "gameplay/abilities/feats_constants.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>

namespace {

using feats::FeatCategories;

const char *kCategoriesSrc = "feat_categories_test.xml";

FeatCategories ParseCategories(const std::string &categories_xml) {
	{
		std::ofstream f(kCategoriesSrc);
		f << "<feats>" << categories_xml << R"(<feat id="kTactician"/></feats>)";
	}
	parser_wrapper::DataNode doc(kCategoriesSrc);
	FeatCategories result;
	result.Build(doc);
	std::remove(kCategoriesSrc);
	return result;
}

FeatCategories::Category MakeCategory(int base, int divisor, int cap) {
	return FeatCategories::Category{"test", base, divisor, cap};
}

} // namespace

TEST(FeatCategories, LimitBaseOnly) {
	const auto category = MakeCategory(3, 0, FeatCategories::kNoCap);
	EXPECT_EQ(category.CalcLimit(0), 3);
	EXPECT_EQ(category.CalcLimit(100), 3);
}

TEST(FeatCategories, LimitGrowsWithRemorts) {
	const auto category = MakeCategory(2, 6, FeatCategories::kNoCap);
	EXPECT_EQ(category.CalcLimit(0), 2);
	EXPECT_EQ(category.CalcLimit(5), 2);
	EXPECT_EQ(category.CalcLimit(6), 3);
	EXPECT_EQ(category.CalcLimit(60), 12);
	EXPECT_EQ(category.CalcLimit(-6), 2);
}

TEST(FeatCategories, LimitIsCapped) {
	const auto category = MakeCategory(1, 2, 3);
	EXPECT_EQ(category.CalcLimit(0), 1);
	EXPECT_EQ(category.CalcLimit(2), 2);
	EXPECT_EQ(category.CalcLimit(4), 3);
	EXPECT_EQ(category.CalcLimit(40), 3);
}

TEST(FeatCategories, NoSection) {
	const auto categories = ParseCategories("");
	EXPECT_EQ(categories.Size(), 0u);
	EXPECT_EQ(categories.Find("weapon_focus"), nullptr);
}

TEST(FeatCategories, Defaults) {
	const auto categories = ParseCategories(R"(<feat_categories><category id="a" base="2"/></feat_categories>)");
	const auto *category = categories.Find("a");
	ASSERT_NE(category, nullptr);
	EXPECT_EQ(category->id, "a");
	EXPECT_EQ(category->base, 2);
	EXPECT_EQ(category->remort_divisor, 0);
	EXPECT_EQ(category->cap, FeatCategories::kNoCap);
}

TEST(FeatCategories, AllAttributes) {
	const auto categories = ParseCategories(
		R"(<feat_categories><category id="a" base="1" remort_divisor="7" cap="4"/></feat_categories>)");
	const auto *category = categories.Find("a");
	ASSERT_NE(category, nullptr);
	EXPECT_EQ(category->base, 1);
	EXPECT_EQ(category->remort_divisor, 7);
	EXPECT_EQ(category->cap, 4);
}

TEST(FeatCategories, InvalidCategoriesAreDropped) {
	const auto categories = ParseCategories(
		R"(<feat_categories>)"
		R"(<category base="1"/>)"
		R"(<category id="no_base"/>)"
		R"(<category id="negative_base" base="-1"/>)"
		R"(<category id="negative_divisor" base="1" remort_divisor="-2"/>)"
		R"(<category id="cap_below_base" base="3" cap="2"/>)"
		R"(<category id="bad_number" base="two"/>)"
		R"(<group id="wrong_tag" base="1"/>)"
		R"(<category id="ok" base="0" cap="0"/>)"
		R"(</feat_categories>)");
	EXPECT_EQ(categories.Size(), 1u);
	EXPECT_NE(categories.Find("ok"), nullptr);
	for (const auto *id : {"no_base", "negative_base", "negative_divisor", "cap_below_base", "bad_number", "wrong_tag"}) {
		EXPECT_EQ(categories.Find(id), nullptr) << id;
	}
}

// Which of two definitions was meant is unknown, so both go.
TEST(FeatCategories, DuplicatesAreDropped) {
	const auto categories = ParseCategories(
		R"(<feat_categories>)"
		R"(<category id="a" base="1"/><category id="b" base="1"/><category id="a" base="5"/>)"
		R"(</feat_categories>)");
	EXPECT_EQ(categories.Find("a"), nullptr);
	EXPECT_NE(categories.Find("b"), nullptr);
}

TEST(FeatCategories, RebuildResets) {
	auto categories = ParseCategories(R"(<feat_categories><category id="a" base="1"/></feat_categories>)");
	{
		std::ofstream f(kCategoriesSrc);
		f << R"(<feats><feat id="kTactician"/></feats>)";
	}
	parser_wrapper::DataNode doc(kCategoriesSrc);
	categories.Build(doc);
	std::remove(kCategoriesSrc);
	EXPECT_EQ(categories.Size(), 0u);
}

// The shipped feats.xml keeps the old hard-coded counters: 2 + remorts / 6 weapon focus feats and
// 1 + remorts / 7 weapon master feats, and puts exactly those feats into the categories.
TEST(FeatCategories, ShippedFileMatchesLegacyCounters) {
	parser_wrapper::DataNode doc(std::filesystem::path("cfg/feats.xml"));
	FeatCategories categories;
	categories.Build(doc);
	const auto *focus = categories.Find("weapon_focus");
	const auto *master = categories.Find("weapon_master");
	ASSERT_NE(focus, nullptr) << "cfg/feats.xml not found -- run the tests from the build directory";
	ASSERT_NE(master, nullptr);
	for (int remorts = 0; remorts <= 100; ++remorts) {
		EXPECT_EQ(focus->CalcLimit(remorts), 2 + remorts / 6) << remorts;
		EXPECT_EQ(master->CalcLimit(remorts), 1 + remorts / 7) << remorts;
	}

	std::map<std::string, std::set<EFeat>> members;
	for (auto &feat : doc.Children("feat")) {
		const std::string category = feat.GetValue("category");
		if (!category.empty()) {
			EXPECT_NE(categories.Find(category), nullptr) << feat.GetValue("id") << ": unknown category " << category;
			members[category].insert(ITEM_BY_NAME<EFeat>(feat.GetValue("id")));
		}
	}
	std::set<EFeat> legacy_focus;
	for (auto id = EFeat::kPunchFocus; id <= EFeat::kBowsFocus; ++id) {
		legacy_focus.insert(id);
	}
	std::set<EFeat> legacy_master;
	for (auto id = EFeat::kPunchMaster; id <= EFeat::kBowsMaster; ++id) {
		legacy_master.insert(id);
	}
	EXPECT_EQ(members.size(), 2u);
	EXPECT_EQ(members["weapon_focus"], legacy_focus);
	EXPECT_EQ(members["weapon_master"], legacy_master);
}

TEST(FeatCategories, DescribeLimit) {
	EXPECT_EQ(MakeCategory(3, 0, FeatCategories::kNoCap).DescribeLimit(), "3");
	EXPECT_EQ(MakeCategory(2, 6, FeatCategories::kNoCap).DescribeLimit(), "2 + remorts/6");
	EXPECT_EQ(MakeCategory(1, 2, 5).DescribeLimit(), "1 + remorts/2, capped at 5");
	EXPECT_EQ(MakeCategory(1, 1, FeatCategories::kNoCap).DescribeLimit(), "1 + remorts");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
