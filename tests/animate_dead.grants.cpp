// issue #4038: "оживить скелет" выдаёт свите даровые аффекты по порогу магии тьмы. Пороги
// лежат в animate_dead.xml блоком <grants>, и тут закреплён именно разбор этого блока
// (inline-XML, без загрузки мира; по образцу affects.loader.cpp).

#include <gtest/gtest.h>

#include "gameplay/mechanics/animate_dead.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>

namespace {

const char *kSrc = "animate_dead_grants_src.xml";

void Write(const char *body) {
	std::ofstream f(kSrc);
	f << "<animate_dead><control skill=\"kDarkMagic\" budget_cap=\"75\" />"
	  << body
	  << "<creature vnum=\"0\" id=\"kSkeleton\" proto_vnum=\"3001\" weight=\"8\">"
	     "<cost corpse_max_level=\"5\" min_rating=\"0\" /></creature>"
	     "</animate_dead>";
}

}  // namespace

TEST(AnimateDead_Grants, ParsesSpellThresholdAndAffect) {
	Write(R"(<grants>)"
		  R"(<grant spell="kAnimateSkeleton" min_skill="100" affect="kDetectInvisible" />)"
		  R"(<grant spell="kAnimateSkeleton" min_skill="120" affect="kFly" />)"
		  R"(<grant spell="kAnimateDead" affect="kIceShield" />)"
		  R"(</grants>)");
	parser_wrapper::DataNode doc(kSrc);
	animate_dead::AnimateDeadInfo info;
	info.Load(doc);

	const auto &grants = info.Grants();
	ASSERT_EQ(grants.size(), 3u);

	EXPECT_EQ(grants[0].spell, ESpell::kAnimateSkeleton);
	EXPECT_EQ(grants[0].min_skill, 100);
	EXPECT_EQ(grants[0].affect, EAffect::kDetectInvisible);

	EXPECT_EQ(grants[1].spell, ESpell::kAnimateSkeleton);
	EXPECT_EQ(grants[1].min_skill, 120);
	EXPECT_EQ(grants[1].affect, EAffect::kFly);

	// Порог не обязателен: без min_skill аффект достаётся любому заклинателю.
	EXPECT_EQ(grants[2].spell, ESpell::kAnimateDead);
	EXPECT_EQ(grants[2].min_skill, 0);
	EXPECT_EQ(grants[2].affect, EAffect::kIceShield);

	// Ярусы блок <grants> не трогает.
	EXPECT_EQ(info.Creatures().size(), 1u);

	std::remove(kSrc);
}

TEST(AnimateDead_Grants, SkipsIncompleteAndUnknownEntries) {
	Write(R"(<grants>)"
		  R"(<grant min_skill="100" affect="kFly" />)"                        // нет заклинания
		  R"(<grant spell="kAnimateSkeleton" min_skill="100" />)"             // нет аффекта
		  R"(<grant spell="kNoSuchSpell" affect="kFly" />)"                   // неизвестное заклинание
		  R"(<grant spell="kAnimateSkeleton" affect="kNoSuchAffect" />)"      // неизвестный аффект
		  R"(<grant spell="kAnimateSkeleton" min_skill="140" affect="kDetectLife" />)"
		  R"(</grants>)");
	parser_wrapper::DataNode doc(kSrc);
	animate_dead::AnimateDeadInfo info;
	info.Load(doc);

	// Кривая строка не должна уносить с собой остальные.
	ASSERT_EQ(info.Grants().size(), 1u);
	EXPECT_EQ(info.Grants()[0].affect, EAffect::kDetectLife);
	EXPECT_EQ(info.Grants()[0].min_skill, 140);

	std::remove(kSrc);
}

TEST(AnimateDead_Grants, ReloadReplacesInsteadOfAppending) {
	Write(R"(<grants><grant spell="kAnimateSkeleton" min_skill="100" affect="kFly" /></grants>)");
	parser_wrapper::DataNode doc(kSrc);
	animate_dead::AnimateDeadInfo info;
	info.Load(doc);
	ASSERT_EQ(info.Grants().size(), 1u);

	// `reload animatedead` зовёт тот же Load: список должен замениться, а не удвоиться.
	parser_wrapper::DataNode again(kSrc);
	info.Load(again);
	EXPECT_EQ(info.Grants().size(), 1u);

	std::remove(kSrc);
}

TEST(AnimateDead_Grants, AbsentBlockLeavesNoGrants) {
	Write("");
	parser_wrapper::DataNode doc(kSrc);
	animate_dead::AnimateDeadInfo info;
	info.Load(doc);
	EXPECT_TRUE(info.Grants().empty());

	std::remove(kSrc);
}
