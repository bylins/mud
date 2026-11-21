// issue #3847: feat prerequisites read from the <prerequisites> section of cfg/feats.xml.
// Parsing and evaluation are exercised through inline XML and plain skill/feat lookups, no world boot.

#include "gameplay/abilities/feat_prerequisites.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

using feats::FeatPrerequisites;

const char *kPrereqSrc = "feat_prerequisites_test.xml";

// Builds the prerequisites of the single <feat> in `feat_xml`.
FeatPrerequisites ParsePrereqs(const std::string &feat_xml, EFeat owner = EFeat::kTactician) {
	{
		std::ofstream f(kPrereqSrc);
		f << "<feats>" << feat_xml << "</feats>";
	}
	parser_wrapper::DataNode doc(kPrereqSrc);
	FeatPrerequisites result;
	for (auto &feat : doc.Children()) {
		result.Build(feat, owner);
	}
	std::remove(kPrereqSrc);
	return result;
}

struct PrereqChar {
	std::map<ESkill, int> skills;
	std::set<EFeat> feats;

	[[nodiscard]] bool Meets(const FeatPrerequisites &p) const {
		return p.IsMet(
			[this](ESkill id) {
				const auto it = skills.find(id);
				return it == skills.end() ? 0 : it->second;
			},
			[this](EFeat id) { return feats.contains(id); });
	}
};

} // namespace

TEST(FeatPrerequisites, NoSectionMeansUnrestricted) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"/>)");
	EXPECT_TRUE(p.IsEmpty());
	EXPECT_TRUE(PrereqChar{}.Meets(p));
}

TEST(FeatPrerequisites, EmptySectionMeansUnrestricted) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites/></feat>)");
	EXPECT_TRUE(p.IsEmpty());
	EXPECT_TRUE(PrereqChar{}.Meets(p));
}

TEST(FeatPrerequisites, Defaults) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites><prerequisite>)"
						 R"(<skill id="kParry"/><feat id="kPowerAttack"/>)"
						 R"(</prerequisite></prerequisites></feat>)");
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	const auto &alt = p.GetAlternatives().front();
	ASSERT_EQ(alt.skills.size(), 1u);
	EXPECT_EQ(alt.skills.front().id, ESkill::kParry);
	EXPECT_EQ(alt.skills.front().min, 1);
	ASSERT_EQ(alt.feats.size(), 1u);
	EXPECT_EQ(alt.feats.front().id, EFeat::kPowerAttack);
	EXPECT_FALSE(alt.feats.front().absence);
}

// All conditions inside one <prerequisite> must hold; any one <prerequisite> is enough.
TEST(FeatPrerequisites, AndWithinOrAcross) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites>)"
						 R"(<prerequisite><skill id="kParry" min="1"/><feat id="kPowerAttack"/></prerequisite>)"
						 R"(<prerequisite><skill id="kMultiparry" min="85"/></prerequisite>)"
						 R"(</prerequisites></feat>)");
	ASSERT_EQ(p.GetAlternatives().size(), 2u);

	EXPECT_FALSE(PrereqChar{}.Meets(p));
	EXPECT_FALSE((PrereqChar{{{ESkill::kParry, 50}}, {}}.Meets(p)));
	EXPECT_FALSE((PrereqChar{{}, {EFeat::kPowerAttack}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{{ESkill::kParry, 50}}, {EFeat::kPowerAttack}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{{ESkill::kMultiparry, 85}}, {}}.Meets(p)));
}

TEST(FeatPrerequisites, MinIsInclusive) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites><prerequisite>)"
						 R"(<skill id="kLeadership" min="100"/>)"
						 R"(</prerequisite></prerequisites></feat>)");
	EXPECT_FALSE((PrereqChar{{{ESkill::kLeadership, 99}}, {}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{{ESkill::kLeadership, 100}}, {}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{{ESkill::kLeadership, 101}}, {}}.Meets(p)));
}

TEST(FeatPrerequisites, Absence) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites><prerequisite>)"
						 R"(<feat id="kGreatFortitude" absence="Y"/>)"
						 R"(</prerequisite></prerequisites></feat>)");
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	EXPECT_TRUE(p.GetAlternatives().front().feats.front().absence);
	EXPECT_TRUE(PrereqChar{}.Meets(p));
	EXPECT_FALSE((PrereqChar{{}, {EFeat::kGreatFortitude}}.Meets(p)));
}

TEST(FeatPrerequisites, AbsenceNoIsPresence) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites><prerequisite>)"
						 R"(<feat id="kGreatFortitude" absence="N"/>)"
						 R"(</prerequisite></prerequisites></feat>)");
	EXPECT_FALSE(PrereqChar{}.Meets(p));
	EXPECT_TRUE((PrereqChar{{}, {EFeat::kGreatFortitude}}.Meets(p)));
}

// A broken <prerequisite> is dropped as a whole; the valid ones still count.
TEST(FeatPrerequisites, InvalidAlternativeIsDropped) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites>)"
						 R"(<prerequisite><skill id="kNoSuchSkill"/></prerequisite>)"
						 R"(<prerequisite><skill id="kParry"/><feat id="kNoSuchFeat"/></prerequisite>)"
						 R"(<prerequisite><skill id="kParry" min="0"/></prerequisite>)"
						 R"(<prerequisite><skill id="kParry" min="many"/></prerequisite>)"
						 R"(<prerequisite><skill min="10"/></prerequisite>)"
						 R"(<prerequisite><stat id="kStr"/></prerequisite>)"
						 R"(<prerequisite/>)"
						 R"(<requisite><skill id="kParry"/></requisite>)"
						 R"(<prerequisite><feat id="kTactician"/></prerequisite>)"
						 R"(<prerequisite><skill id="kMultiparry" min="85"/></prerequisite>)"
						 R"(</prerequisites></feat>)");
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	EXPECT_EQ(p.GetAlternatives().front().skills.front().id, ESkill::kMultiparry);
	EXPECT_FALSE((PrereqChar{{{ESkill::kParry, 200}}, {}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{{ESkill::kMultiparry, 85}}, {}}.Meets(p)));
}

// A feat may require its own absence (pointless but harmless); only requiring itself is an error.
TEST(FeatPrerequisites, OwnAbsenceIsAllowed) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites><prerequisite>)"
						 R"(<feat id="kTactician" absence="Y"/>)"
						 R"(</prerequisite></prerequisites></feat>)");
	EXPECT_EQ(p.GetAlternatives().size(), 1u);
}

// If no alternative survives, the feat cannot be learned at all -- a mistake never opens it up.
TEST(FeatPrerequisites, AllAlternativesInvalidLocksTheFeat) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites>)"
						 R"(<prerequisite><skill id="kNoSuchSkill"/></prerequisite>)"
						 R"(<prerequisite/>)"
						 R"(</prerequisites></feat>)");
	EXPECT_TRUE(p.GetAlternatives().empty());
	EXPECT_FALSE(p.IsEmpty());
	EXPECT_FALSE(PrereqChar{}.Meets(p));
	EXPECT_FALSE((PrereqChar{{{ESkill::kParry, 200}}, {EFeat::kPowerAttack}}.Meets(p)));
}

// Build replaces whatever was there before (feats reload rebuilds every FeatInfo).
TEST(FeatPrerequisites, RebuildResets) {
	std::ofstream(kPrereqSrc) << R"(<feats><feat id="kTactician"><prerequisites>)"
						   R"(<prerequisite><skill id="kNoSuchSkill"/></prerequisite>)"
						   R"(</prerequisites></feat><feat id="kTactician"/></feats>)";
	parser_wrapper::DataNode doc(kPrereqSrc);
	FeatPrerequisites p;
	std::vector<bool> empty_after_build;
	for (auto &feat : doc.Children()) {
		p.Build(feat, EFeat::kTactician);
		empty_after_build.push_back(p.IsEmpty());
	}
	EXPECT_EQ(empty_after_build, (std::vector<bool>{false, true}));
	std::remove(kPrereqSrc);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
