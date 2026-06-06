// issue.thing-names: GuildsLoader's Vedun overrides for vnum-keyed guilds. A <guild> has no `id`
// attribute -- its identity is the integer `vnum` -- so the loader overrides FindElementNode (match
// by `vnum`), CanonicalElementId (accept only integer keys, for create-by-vnum) and CreateElementNode
// (stamp the new vnum). Exercised through inline XML, no world boot (mirrors vedun.element_find.cpp).

#include <gtest/gtest.h>

#include "gameplay/mechanics/guilds.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>

namespace {

const char *kSrc = "guilds_vnum_src.xml";

void WriteGuilds() {
	std::ofstream f(kSrc);
	f << R"(<guilds>)"
	     R"(<guild text_id="kOldMan" vnum="6" name="x"><trainers vnums="4008"/></guild>)"
	     R"(<guild text_id="kSmith" vnum="42" name="y"/>)"
	     R"(</guilds>)";
}

} // namespace

// FindElementNode locates a guild by its `vnum` attribute (not an `id`, which guilds lack).
TEST(GuildsLoader_Vnum, FindElementNodeMatchesByVnumAttr) {
	WriteGuilds();
	parser_wrapper::DataNode doc(kSrc);
	guilds::GuildsLoader loader;

	auto hit = loader.FindElementNode(doc, "6");
	ASSERT_TRUE(hit.IsNotEmpty());
	EXPECT_STREQ(hit.GetValue("text_id"), "kOldMan");

	auto hit2 = loader.FindElementNode(doc, "42");
	ASSERT_TRUE(hit2.IsNotEmpty());
	EXPECT_STREQ(hit2.GetValue("text_id"), "kSmith");

	EXPECT_TRUE(loader.FindElementNode(doc, "7").IsEmpty());        // no such vnum
	EXPECT_TRUE(loader.FindElementNode(doc, "kOldMan").IsEmpty());  // text_id is not the node key
	std::remove(kSrc);
}

// CanonicalElementId (the create-path key gate) accepts only non-negative integers.
TEST(GuildsLoader_Vnum, CanonicalElementIdAcceptsIntegersOnly) {
	guilds::GuildsLoader loader;
	EXPECT_EQ(loader.CanonicalElementId("6"), "6");
	EXPECT_EQ(loader.CanonicalElementId("42"), "42");
	EXPECT_EQ(loader.CanonicalElementId("0"), "0");
	EXPECT_EQ(loader.CanonicalElementId(""), "");
	EXPECT_EQ(loader.CanonicalElementId("kOldMan"), "");  // text_id resolves via ListElements, not here
	EXPECT_EQ(loader.CanonicalElementId("-3"), "");
	EXPECT_EQ(loader.CanonicalElementId("6a"), "");
}

// CreateElementNode stamps the vnum so the fresh guild is immediately findable by it.
TEST(GuildsLoader_Vnum, CreateElementNodeSetsVnumAndIsFindable) {
	const char *src = "guilds_create_src.xml";
	{
		std::ofstream f(src);
		f << R"(<guilds></guilds>)";
	}
	parser_wrapper::DataNode doc(src);
	guilds::GuildsLoader loader;

	auto node = loader.CreateElementNode(doc, "99");
	ASSERT_TRUE(node.IsNotEmpty());
	EXPECT_STREQ(node.GetValue("vnum"), "99");
	EXPECT_TRUE(loader.FindElementNode(doc, "99").IsNotEmpty());
	std::remove(src);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
