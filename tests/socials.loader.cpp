// issue.socials: SocialsLoader parses vnum-keyed socials from XML (keywords, EPosition gates, the six
// message slots with the &K..&n colour wrap), and find_action resolves a typed command to a social
// vnum by PREFIX over ALL keyword synonyms. ASCII-only fixture (the logic is encoding-agnostic).

#include <gtest/gtest.h>

#include "gameplay/communication/social.h"
#include "engine/db/global_objects.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>

TEST(Socials_Loader, ParsesFieldsAndPrefixSynonymSearch) {
	const char *src = "socials_test.xml";
	{
		std::ofstream f(src);
		f << R"(<socials>)"
		     R"(<social vnum="0" keywords="smile|grin" ch_min="kRest" ch_max="kStand" vict_min="kRest" vict_max="kStand">)"
		     R"(<message type="kCharNoArg" val="You smile."/>)"
		     R"(<message type="kCharFound" val="You smile at $N2."/>)"
		     R"(</social>)"
		     R"(<social vnum="5" keywords="nod" ch_min="kSit" ch_max="kStand" vict_min="kDead" vict_max="kStand">)"
		     R"(<message type="kCharNoArg" val="You nod."/>)"
		     R"(</social>)"
		     R"(</socials>)";
	}
	parser_wrapper::DataNode doc(src);
	communication::social::SocialsLoader loader;
	loader.Load(doc);   // populates MUD::Socials() + rebuilds the keyword index

	// find_action: prefix match against any synonym -> vnum; -1 if none.
	char c1[] = "smi";  EXPECT_EQ(find_action(c1), 0);   // prefix of "smile"
	char c2[] = "grin"; EXPECT_EQ(find_action(c2), 0);   // the second synonym of social 0
	char c3[] = "nod";  EXPECT_EQ(find_action(c3), 5);
	char c4[] = "xyz";  EXPECT_EQ(find_action(c4), -1);

	// parsed fields
	const auto &soc = MUD::Socials()[0];
	EXPECT_EQ(soc.GetChMinPos(), EPosition::kRest);
	EXPECT_EQ(soc.GetVictMaxPos(), EPosition::kStand);
	ASSERT_EQ(soc.GetKeywords().size(), 2u);
	EXPECT_EQ(soc.GetKeywords().front(), "smile");
	EXPECT_TRUE(soc.HasMsg(ESocialMsg::kCharFound));
	EXPECT_FALSE(soc.HasMsg(ESocialMsg::kNotFound));
	EXPECT_EQ(soc.GetMsg(ESocialMsg::kCharNoArg), "&KYou smile.&n");   // &K..&n wrap applied at build

	std::remove(src);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
