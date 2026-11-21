// issue #3847: feat conditions read from cfg/feats.xml -- <prerequisites> to learn a feat, <requirements>
// to use it. Parsing and evaluation are exercised through inline XML and plain lookups, no world boot.

#include "gameplay/abilities/feat_conditions.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <functional>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using feats::FeatConditions;
using EKind = FeatConditions::EKind;

const char *kPrereqSrc = "feat_conditions_test.xml";

// Builds the conditions of the given kind of the single <feat> in `feat_xml`.
FeatConditions ParsePrereqs(const std::string &feat_xml, EKind kind = EKind::kPrerequisites,
							EFeat owner = EFeat::kTactician) {
	{
		std::ofstream f(kPrereqSrc);
		f << "<feats>" << feat_xml << "</feats>";
	}
	parser_wrapper::DataNode doc(kPrereqSrc);
	FeatConditions result{kind};
	for (auto &feat : doc.Children()) {
		result.Build(feat, owner);
	}
	std::remove(kPrereqSrc);
	return result;
}

struct PrereqChar {
	explicit PrereqChar(std::map<ESkill, int> skills_ = {}, std::set<EFeat> feats_ = {},
						std::map<EBaseStat, int> stats_ = {})
		: skills{std::move(skills_)}, feats{std::move(feats_)}, stats{std::move(stats_)} {}

	std::map<ESkill, int> skills;
	std::set<EFeat> feats;
	std::map<EBaseStat, int> stats;
	int level{0};
	int remorts{0};

	[[nodiscard]] int Skill(ESkill id) const {
		const auto it = skills.find(id);
		return it == skills.end() ? 0 : it->second;
	}
	[[nodiscard]] bool HasFeat(EFeat id) const { return feats.contains(id); }
	[[nodiscard]] int Stat(EBaseStat id) const {
		const auto it = stats.find(id);
		return it == stats.end() ? 0 : it->second;
	}
	[[nodiscard]] int Level() const { return level; }
	[[nodiscard]] int Remorts() const { return remorts; }

	[[nodiscard]] bool Meets(const FeatConditions &p) const { return p.IsMetBy(*this); }
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
	FeatConditions p{EKind::kPrerequisites};
	std::vector<bool> empty_after_build;
	for (auto &feat : doc.Children()) {
		p.Build(feat, EFeat::kTactician);
		empty_after_build.push_back(p.IsEmpty());
	}
	EXPECT_EQ(empty_after_build, (std::vector<bool>{false, true}));
	std::remove(kPrereqSrc);
}

// <requirements> is read the same way, with its own tags; each kind ignores the other's section.
TEST(FeatRequirements, OwnSection) {
	const std::string xml = R"(<feat id="kTactician">)"
							R"(<prerequisites><prerequisite><skill id="kParry"/></prerequisite></prerequisites>)"
							R"(<requirements><requirement><skill id="kMultiparry" min="10"/></requirement></requirements>)"
							R"(</feat>)";
	const auto prerequisites = ParsePrereqs(xml, EKind::kPrerequisites);
	const auto requirements = ParsePrereqs(xml, EKind::kRequirements);
	EXPECT_EQ(requirements.GetKind(), EKind::kRequirements);
	ASSERT_EQ(prerequisites.GetAlternatives().size(), 1u);
	EXPECT_EQ(prerequisites.GetAlternatives().front().skills.front().id, ESkill::kParry);
	ASSERT_EQ(requirements.GetAlternatives().size(), 1u);
	EXPECT_EQ(requirements.GetAlternatives().front().skills.front().id, ESkill::kMultiparry);

	EXPECT_TRUE(ParsePrereqs(R"(<feat id="kTactician"><prerequisites/></feat>)", EKind::kRequirements).IsEmpty());
}

TEST(FeatRequirements, WrongAlternativeTag) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements>)"
								R"(<prerequisite><skill id="kParry"/></prerequisite>)"
								R"(</requirements></feat>)", EKind::kRequirements);
	EXPECT_FALSE(p.IsEmpty());
	EXPECT_FALSE((PrereqChar{{{ESkill::kParry, 200}}, {}, {}}.Meets(p)));
}

TEST(FeatConditionsStat, MinIsInclusive) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements><requirement>)"
								R"(<stat id="kStr" min="20"/>)"
								R"(</requirement></requirements></feat>)", EKind::kRequirements);
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	const auto &stat = p.GetAlternatives().front().stats.front();
	EXPECT_EQ(stat.id, EBaseStat::kStr);
	EXPECT_EQ(stat.min, 20);
	EXPECT_FALSE(stat.has_above);
	EXPECT_FALSE((PrereqChar{{}, {}, {{EBaseStat::kStr, 19}}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{}, {}, {{EBaseStat::kStr, 20}}}.Meets(p)));
}

TEST(FeatConditionsStat, AboveIsStrict) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements><requirement>)"
								R"(<stat id="kDex" above="kStr"/>)"
								R"(</requirement></requirements></feat>)", EKind::kRequirements);
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	EXPECT_FALSE((PrereqChar{{}, {}, {{EBaseStat::kDex, 20}, {EBaseStat::kStr, 20}}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{}, {}, {{EBaseStat::kDex, 21}, {EBaseStat::kStr, 20}}}.Meets(p)));
}

// Weapon finesse: Dex above Str and at least 18.
TEST(FeatConditionsStat, MinAndAbove) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements><requirement>)"
								R"(<stat id="kDex" min="18" above="kStr"/>)"
								R"(</requirement></requirements></feat>)", EKind::kRequirements);
	EXPECT_FALSE((PrereqChar{{}, {}, {{EBaseStat::kDex, 17}, {EBaseStat::kStr, 10}}}.Meets(p)));
	EXPECT_FALSE((PrereqChar{{}, {}, {{EBaseStat::kDex, 25}, {EBaseStat::kStr, 25}}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{}, {}, {{EBaseStat::kDex, 18}, {EBaseStat::kStr, 17}}}.Meets(p)));
}

TEST(FeatConditionsStat, AllowedInPrerequisites) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><prerequisites><prerequisite>)"
								R"(<stat id="kCha" min="15"/>)"
								R"(</prerequisite></prerequisites></feat>)");
	EXPECT_FALSE((PrereqChar{{}, {}, {{EBaseStat::kCha, 14}}}.Meets(p)));
	EXPECT_TRUE((PrereqChar{{}, {}, {{EBaseStat::kCha, 15}}}.Meets(p)));
}

TEST(FeatConditionsStat, InvalidStatsAreDropped) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements>)"
								R"(<requirement><stat id="kDex"/></requirement>)"
								R"(<requirement><stat id="kDex" above="kDex"/></requirement>)"
								R"(<requirement><stat id="kLuck" min="10"/></requirement>)"
								R"(<requirement><stat id="kDex" above="kLuck"/></requirement>)"
								R"(<requirement><stat id="kDex" min="0"/></requirement>)"
								R"(<requirement><stat min="10"/></requirement>)"
								R"(<requirement><stat id="kWis" min="12"/></requirement>)"
								R"(</requirements></feat>)", EKind::kRequirements);
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	EXPECT_EQ(p.GetAlternatives().front().stats.front().id, EBaseStat::kWis);
}

// Skill, feat and stat conditions in one alternative must all hold.
TEST(FeatConditionsStat, MixedAlternative) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements><requirement>)"
								R"(<feat id="kBowsFocus"/><skill id="kBows" min="40"/><stat id="kDex" min="16"/>)"
								R"(</requirement></requirements></feat>)", EKind::kRequirements);
	const PrereqChar all{{{ESkill::kBows, 40}}, {EFeat::kBowsFocus}, {{EBaseStat::kDex, 16}}};
	EXPECT_TRUE(all.Meets(p));
	auto low_dex = all;
	low_dex.stats[EBaseStat::kDex] = 15;
	EXPECT_FALSE(low_dex.Meets(p));
	auto no_focus = all;
	no_focus.feats.clear();
	EXPECT_FALSE(no_focus.Meets(p));
	auto low_skill = all;
	low_skill.skills[ESkill::kBows] = 39;
	EXPECT_FALSE(low_skill.Meets(p));
}

namespace {

FeatConditions ParseLevelRequirement(const std::string &level_xml) {
	return ParsePrereqs(R"(<feat id="kTactician"><requirements><requirement>)" + level_xml +
						R"(</requirement></requirements></feat>)", EKind::kRequirements);
}

PrereqChar AtLevel(int level, int remorts = 0) {
	PrereqChar ch;
	ch.level = level;
	ch.remorts = remorts;
	return ch;
}

} // namespace

TEST(FeatConditionsLevel, Defaults) {
	const auto p = ParseLevelRequirement(R"(<level min="20"/>)");
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	const auto &level = p.GetAlternatives().front().levels.front();
	EXPECT_EQ(level.min, 20);
	EXPECT_FALSE(level.has_max);
	EXPECT_EQ(level.remort_divisor, 0);
}

// The magic user: level below 25.
TEST(FeatConditionsLevel, MaxIsInclusive) {
	const auto p = ParseLevelRequirement(R"(<level max="24"/>)");
	EXPECT_TRUE(AtLevel(1).Meets(p));
	EXPECT_TRUE(AtLevel(24).Meets(p));
	EXPECT_FALSE(AtLevel(25).Meets(p));
	EXPECT_TRUE(AtLevel(24, 60).Meets(p));	// no remort_divisor: remorts do not count
}

TEST(FeatConditionsLevel, MinAndMax) {
	const auto p = ParseLevelRequirement(R"(<level min="10" max="20"/>)");
	EXPECT_FALSE(AtLevel(9).Meets(p));
	EXPECT_TRUE(AtLevel(10).Meets(p));
	EXPECT_TRUE(AtLevel(20).Meets(p));
	EXPECT_FALSE(AtLevel(21).Meets(p));
}

// The skilled trader: level + remorts / 3 above 19.
TEST(FeatConditionsLevel, RemortsAddToLevel) {
	const auto p = ParseLevelRequirement(R"(<level min="20" remort_divisor="3"/>)");
	EXPECT_FALSE(AtLevel(19).Meets(p));
	EXPECT_TRUE(AtLevel(20).Meets(p));
	EXPECT_FALSE(AtLevel(19, 2).Meets(p));
	EXPECT_TRUE(AtLevel(19, 3).Meets(p));
	EXPECT_TRUE(AtLevel(10, 30).Meets(p));
	EXPECT_FALSE(AtLevel(10, 29).Meets(p));
}

TEST(FeatConditionsLevel, RemortsIgnoredWithoutDivisor) {
	const auto p = ParseLevelRequirement(R"(<level min="20"/>)");
	EXPECT_FALSE(AtLevel(19, 100).Meets(p));
}

TEST(FeatConditionsLevel, InvalidLevelsAreDropped) {
	const auto p = ParsePrereqs(R"(<feat id="kTactician"><requirements>)"
								R"(<requirement><level/></requirement>)"
								R"(<requirement><level remort_divisor="3"/></requirement>)"
								R"(<requirement><level min="0"/></requirement>)"
								R"(<requirement><level max="0"/></requirement>)"
								R"(<requirement><level min="30" max="20"/></requirement>)"
								R"(<requirement><level min="20" remort_divisor="-1"/></requirement>)"
								R"(<requirement><level min="high"/></requirement>)"
								R"(<requirement><level min="5" max="5"/></requirement>)"
								R"(</requirements></feat>)", EKind::kRequirements);
	ASSERT_EQ(p.GetAlternatives().size(), 1u);
	EXPECT_EQ(p.GetAlternatives().front().levels.front().min, 5);
	EXPECT_EQ(p.GetAlternatives().front().levels.front().max, 5);
}

// The prerequisites in the shipped cfg/feats.xml must give the same answers as the rules that used to be
// hard-coded in CanGetFeat (the weapon focus / master counters are feat categories, see feats.categories.cpp).
namespace {

using SkillOf = std::function<int(ESkill)>;
using HasFeat = std::function<bool(EFeat)>;
using LegacyRule = std::function<bool(const SkillOf &, const HasFeat &)>;
using StatOf = std::function<int(EBaseStat)>;

// Plain lookups as the subject of the conditions; level and remorts are not part of these rules.
struct FnSubject {
	SkillOf skill;
	std::function<bool(EFeat)> feat;
	StatOf stat;

	[[nodiscard]] int Skill(ESkill id) const { return skill(id); }
	[[nodiscard]] bool HasFeat(EFeat id) const { return feat(id); }
	[[nodiscard]] int Stat(EBaseStat id) const { return stat(id); }
	[[nodiscard]] int Level() const { return 0; }
	[[nodiscard]] int Remorts() const { return 0; }
};

struct LegacyCase {
	LegacyRule rule;
	std::vector<ESkill> skills;
	std::vector<EFeat> feats;
};

std::map<EFeat, LegacyCase> LegacyRules() {
	std::map<EFeat, LegacyCase> rules;
	auto any_skill = [&rules](EFeat feat, std::vector<ESkill> skills) {
		rules[feat] = {[skills](const SkillOf &sk, const HasFeat &) {
			for (const auto id : skills) {
				if (sk(id)) {
					return true;
				}
			}
			return false;
		}, skills, {}};
	};
	auto any_feat = [&rules](EFeat feat, std::vector<EFeat> feats) {
		rules[feat] = {[feats](const SkillOf &, const HasFeat &hf) {
			for (const auto id : feats) {
				if (hf(id)) {
					return true;
				}
			}
			return false;
		}, {}, feats};
	};
	// feat + skill above `above` (the old code compared with '>').
	auto feat_and_skill = [&rules](EFeat feat, EFeat need, ESkill skill, int above) {
		rules[feat] = {[need, skill, above](const SkillOf &sk, const HasFeat &hf) {
			return hf(need) && sk(skill) > above;
		}, {skill}, {need}};
	};
	auto skill_above = [&rules](EFeat feat, ESkill skill, int above) {
		rules[feat] = {[skill, above](const SkillOf &sk, const HasFeat &) {
			return sk(skill) > above;
		}, {skill}, {}};
	};

	any_skill(EFeat::kParryArrow, {ESkill::kMultiparry, ESkill::kParry});
	any_skill(EFeat::kConnoiseur, {ESkill::kIdentify});
	any_skill(EFeat::kExorcist, {ESkill::kTurnUndead});
	any_skill(EFeat::kHealer, {ESkill::kFirstAid});
	any_skill(EFeat::kStealthy, {ESkill::kHide, ESkill::kSneak, ESkill::kDisguise});
	any_skill(EFeat::kTracker, {ESkill::kTrack, ESkill::kSense});
	any_skill(EFeat::kNimbleFingers, {ESkill::kSteal, ESkill::kPickLock});
	any_skill(EFeat::kScirmisher, {ESkill::kRescue});
	any_feat(EFeat::kWarriorSpirit, {EFeat::kGreatFortitude});
	any_feat(EFeat::kGreatPowerAttack, {EFeat::kPowerAttack});
	any_feat(EFeat::kGreatAimingAttack, {EFeat::kAimingAttack});
	any_feat(EFeat::kSerratedBlade, {EFeat::kCutting});
	any_feat(EFeat::kCutting, {EFeat::kShortsMaster, EFeat::kPicksMaster, EFeat::kLongsMaster, EFeat::kSpadesMaster});
	feat_and_skill(EFeat::kDoubleShot, EFeat::kBowsFocus, ESkill::kBows, 39);
	feat_and_skill(EFeat::kShadowThrower, EFeat::kPowerThrow, ESkill::kDarkMagic, 120);
	feat_and_skill(EFeat::kShadowDagger, EFeat::kShadowThrower, ESkill::kDarkMagic, 130);
	feat_and_skill(EFeat::kShadowSpear, EFeat::kShadowThrower, ESkill::kDarkMagic, 130);
	feat_and_skill(EFeat::kShadowClub, EFeat::kShadowThrower, ESkill::kDarkMagic, 130);
	feat_and_skill(EFeat::kDoubleThrower, EFeat::kPowerThrow, ESkill::kThrow, 100);
	feat_and_skill(EFeat::kTripleThrower, EFeat::kDeadlyThrow, ESkill::kThrow, 130);
	feat_and_skill(EFeat::kDeadlyThrow, EFeat::kPowerThrow, ESkill::kThrow, 110);
	skill_above(EFeat::kJeweller, ESkill::kJewelry, 59);
	skill_above(EFeat::kTactician, ESkill::kLeadership, 99);
	skill_above(EFeat::kPowerThrow, ESkill::kThrow, 90);

	const std::vector<std::tuple<EFeat, EFeat, ESkill>> weapons = {
		{EFeat::kPunchFocus, EFeat::kPunchMaster, ESkill::kPunch},
		{EFeat::kClubsFocus, EFeat::kClubsMaster, ESkill::kClubs},
		{EFeat::kAxesFocus, EFeat::kAxesMaster, ESkill::kAxes},
		{EFeat::kLongsFocus, EFeat::kLongsMaster, ESkill::kLongBlades},
		{EFeat::kShortsFocus, EFeat::kShortsMaster, ESkill::kShortBlades},
		{EFeat::kNonstandartsFocus, EFeat::kNonstandartsMaster, ESkill::kNonstandart},
		{EFeat::kTwohandsFocus, EFeat::kTwohandsMaster, ESkill::kTwohands},
		{EFeat::kPicksFocus, EFeat::kPicksMaster, ESkill::kPicks},
		{EFeat::kSpadesFocus, EFeat::kSpadesMaster, ESkill::kSpades},
		{EFeat::kBowsFocus, EFeat::kBowsMaster, ESkill::kBows},
	};
	for (const auto &[focus, master, skill] : weapons) {
		any_skill(focus, {skill});
		feat_and_skill(master, focus, skill, 0);
	}
	return rules;
}

std::map<EFeat, FeatConditions> LoadShipped(EKind kind) {
	std::map<EFeat, FeatConditions> result;
	parser_wrapper::DataNode doc(std::filesystem::path("cfg/feats.xml"));
	for (auto &feat : doc.Children("feat")) {
		const auto id = ITEM_BY_NAME<EFeat>(feat.GetValue("id"));
		result.emplace(id, FeatConditions{kind}).first->second.Build(feat, id);
	}
	return result;
}

} // namespace

TEST(FeatPrerequisites, ShippedFileMatchesLegacyRules) {
	const auto shipped = LoadShipped(EKind::kPrerequisites);
	ASSERT_FALSE(shipped.empty()) << "cfg/feats.xml not found -- run the tests from the build directory";
	const auto legacy = LegacyRules();

	// Every feat with prerequisites in the file is one of the converted ones, and vice versa.
	for (const auto &[id, prereqs] : shipped) {
		if (!prereqs.IsEmpty()) {
			EXPECT_TRUE(legacy.contains(id)) << NAME_BY_ITEM<EFeat>(id) << " has prerequisites, but had no rule";
		}
	}

	const std::vector<int> values = {0, 1, 39, 40, 59, 60, 90, 91, 99, 100, 101, 110, 111, 120, 121, 130, 131, 200};
	for (const auto &[id, legacy_case] : legacy) {
		const auto it = shipped.find(id);
		ASSERT_NE(it, shipped.end()) << NAME_BY_ITEM<EFeat>(id) << " is missing from cfg/feats.xml";
		const auto &prereqs = it->second;
		ASSERT_FALSE(prereqs.IsEmpty()) << NAME_BY_ITEM<EFeat>(id) << " has no prerequisites";

		const auto &skills = legacy_case.skills;
		const auto &feats = legacy_case.feats;
		std::size_t skill_combos = 1;
		for (std::size_t i = 0; i < skills.size(); ++i) {
			skill_combos *= values.size();
		}
		const std::size_t feat_combos = std::size_t{1} << feats.size();
		for (std::size_t sc = 0; sc < skill_combos; ++sc) {
			std::map<ESkill, int> skill_values;
			auto rest = sc;
			for (const auto skill : skills) {
				skill_values[skill] = values[rest % values.size()];
				rest /= values.size();
			}
			for (std::size_t fc = 0; fc < feat_combos; ++fc) {
				std::set<EFeat> owned;
				for (std::size_t f = 0; f < feats.size(); ++f) {
					if (fc & (std::size_t{1} << f)) {
						owned.insert(feats[f]);
					}
				}
				const SkillOf skill_of = [&skill_values](ESkill s) {
					const auto found = skill_values.find(s);
					return found == skill_values.end() ? 0 : found->second;
				};
				const HasFeat has_feat = [&owned](EFeat f) { return owned.contains(f); };
				ASSERT_EQ(legacy_case.rule(skill_of, has_feat), prereqs.IsMetBy(FnSubject{skill_of, has_feat, [](EBaseStat) { return 0; }}))
					<< NAME_BY_ITEM<EFeat>(id) << ": skill combo " << sc << ", feat combo " << fc;
			}
		}
	}
}

// The requirements in the shipped cfg/feats.xml must give the same answers as the checks that used to be
// hard-coded in CanUseFeat (the feats with specific checks still in code are not part of this).
namespace {

using UseRule = std::function<bool(const SkillOf &, const HasFeat &, const StatOf &)>;

struct LegacyUseCase {
	UseRule rule;
	std::vector<ESkill> skills;
	std::vector<EFeat> feats;
	std::vector<EBaseStat> stats;
};

std::map<EFeat, LegacyUseCase> LegacyUseRules() {
	std::map<EFeat, LegacyUseCase> rules;
	auto stat_above = [&rules](EFeat feat, EBaseStat stat, int above) {
		rules[feat] = {[stat, above](const SkillOf &, const HasFeat &, const StatOf &st) {
			return st(stat) > above;
		}, {}, {}, {stat}};
	};
	auto skill_above = [&rules](EFeat feat, ESkill skill, int above) {
		rules[feat] = {[skill, above](const SkillOf &sk, const HasFeat &, const StatOf &) {
			return sk(skill) > above;
		}, {skill}, {}, {}};
	};
	auto finesse = [&rules](EFeat feat) {
		rules[feat] = {[](const SkillOf &, const HasFeat &, const StatOf &st) {
			return st(EBaseStat::kDex) > st(EBaseStat::kStr) && st(EBaseStat::kDex) > 17;
		}, {}, {}, {EBaseStat::kDex, EBaseStat::kStr}};
	};

	finesse(EFeat::kWeaponFinesse);
	finesse(EFeat::kFInesseShot);
	stat_above(EFeat::kParryArrow, EBaseStat::kDex, 15);
	stat_above(EFeat::kPowerAttack, EBaseStat::kStr, 19);
	stat_above(EFeat::kGreatPowerAttack, EBaseStat::kStr, 21);
	stat_above(EFeat::kAimingAttack, EBaseStat::kDex, 15);
	stat_above(EFeat::kGreatAimingAttack, EBaseStat::kDex, 17);
	rules[EFeat::kDoubleShot] = {[](const SkillOf &sk, const HasFeat &hf, const StatOf &) {
		return hf(EFeat::kBowsFocus) && sk(ESkill::kBows) > 39;
	}, {ESkill::kBows}, {EFeat::kBowsFocus}, {}};
	rules[EFeat::kSlashMaster] = {[](const SkillOf &, const HasFeat &hf, const StatOf &) {
		return hf(EFeat::kTwohandsFocus);
	}, {}, {EFeat::kTwohandsFocus}, {}};
	skill_above(EFeat::kJeweller, ESkill::kJewelry, 59);
	skill_above(EFeat::kLiveShield, ESkill::kRescue, 124);
	skill_above(EFeat::kShadowThrower, ESkill::kDarkMagic, 120);
	skill_above(EFeat::kAnimalMaster, ESkill::kMindMagic, 79);
	return rules;
}

} // namespace

TEST(FeatRequirements, ShippedFileMatchesLegacyRules) {
	const auto shipped = LoadShipped(EKind::kRequirements);
	ASSERT_FALSE(shipped.empty()) << "cfg/feats.xml not found -- run the tests from the build directory";
	const auto legacy = LegacyUseRules();

	for (const auto &[id, reqs] : shipped) {
		if (!reqs.IsEmpty()) {
			EXPECT_TRUE(legacy.contains(id)) << NAME_BY_ITEM<EFeat>(id) << " has requirements, but had no rule";
		}
	}

	const std::vector<int> skill_values = {0, 1, 39, 40, 59, 60, 79, 80, 120, 121, 124, 125, 200};
	const std::vector<int> stat_values = {1, 10, 15, 16, 17, 18, 19, 20, 21, 22, 25, 50};
	for (const auto &[id, legacy_case] : legacy) {
		const auto it = shipped.find(id);
		ASSERT_NE(it, shipped.end()) << NAME_BY_ITEM<EFeat>(id) << " is missing from cfg/feats.xml";
		const auto &reqs = it->second;
		ASSERT_FALSE(reqs.IsEmpty()) << NAME_BY_ITEM<EFeat>(id) << " has no requirements";

		// Odometer over every skill value x stat value x owned-feat subset.
		const std::size_t dims = legacy_case.skills.size() + legacy_case.stats.size();
		std::vector<std::size_t> digits(dims, 0);
		const std::size_t feat_combos = std::size_t{1} << legacy_case.feats.size();
		while (true) {
			std::map<ESkill, int> skill_map;
			std::map<EBaseStat, int> stat_map;
			for (std::size_t i = 0; i < legacy_case.skills.size(); ++i) {
				skill_map[legacy_case.skills[i]] = skill_values[digits[i]];
			}
			for (std::size_t i = 0; i < legacy_case.stats.size(); ++i) {
				stat_map[legacy_case.stats[i]] = stat_values[digits[legacy_case.skills.size() + i]];
			}
			for (std::size_t fc = 0; fc < feat_combos; ++fc) {
				std::set<EFeat> owned;
				for (std::size_t f = 0; f < legacy_case.feats.size(); ++f) {
					if (fc & (std::size_t{1} << f)) {
						owned.insert(legacy_case.feats[f]);
					}
				}
				const SkillOf skill_of = [&skill_map](ESkill s) {
					const auto found = skill_map.find(s);
					return found == skill_map.end() ? 0 : found->second;
				};
				const HasFeat has_feat = [&owned](EFeat f) { return owned.contains(f); };
				const StatOf stat_of = [&stat_map](EBaseStat s) {
					const auto found = stat_map.find(s);
					return found == stat_map.end() ? 0 : found->second;
				};
				ASSERT_EQ(legacy_case.rule(skill_of, has_feat, stat_of), reqs.IsMetBy(FnSubject{skill_of, has_feat, stat_of}))
					<< NAME_BY_ITEM<EFeat>(id) << ": feat combo " << fc;
			}

			std::size_t pos = 0;
			while (pos < dims) {
				const std::size_t base = pos < legacy_case.skills.size() ? skill_values.size() : stat_values.size();
				if (++digits[pos] < base) {
					break;
				}
				digits[pos] = 0;
				++pos;
			}
			if (pos == dims) {
				break;
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
