// issue.thing-names: GuildsLoader's Vedun overrides for vnum-keyed guilds. A <guild> has no `id`
// attribute -- its identity is the integer `vnum` -- so the loader overrides FindElementNode (match
// by `vnum`), CanonicalElementId (accept only integer keys, for create-by-vnum) and CreateElementNode
// (stamp the new vnum). Exercised through inline XML, no world boot (mirrors vedun.element_find.cpp).

#include <gtest/gtest.h>

#include "gameplay/mechanics/guilds.h"
#include "gameplay/mechanics/guild_messages.h"
#include "engine/structs/msg_container.h"
#include "engine/structs/info_container.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>

namespace {

const char *kGuildsSrc = "guilds_vnum_src.xml";

void WriteGuilds() {
	std::ofstream f(kGuildsSrc);
	f << R"(<guilds>)"
	     R"(<guild text_id="kOldMan" vnum="6" name="x"><trainers vnums="4008"/></guild>)"
	     R"(<guild text_id="kSmith" vnum="42" name="y"/>)"
	     R"(</guilds>)";
}

} // namespace

// FindElementNode locates a guild by its `vnum` attribute (not an `id`, which guilds lack).
TEST(GuildsLoader_Vnum, FindElementNodeMatchesByVnumAttr) {
	WriteGuilds();
	parser_wrapper::DataNode doc(kGuildsSrc);
	guilds::GuildsLoader loader;

	auto hit = loader.FindElementNode(doc, "6");
	ASSERT_TRUE(hit.IsNotEmpty());
	EXPECT_STREQ(hit.GetValue("text_id"), "kOldMan");

	auto hit2 = loader.FindElementNode(doc, "42");
	ASSERT_TRUE(hit2.IsNotEmpty());
	EXPECT_STREQ(hit2.GetValue("text_id"), "kSmith");

	EXPECT_TRUE(loader.FindElementNode(doc, "7").IsEmpty());        // no such vnum
	EXPECT_TRUE(loader.FindElementNode(doc, "kOldMan").IsEmpty());  // text_id is not the node key
	std::remove(kGuildsSrc);
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

// The node FindElementNode hands back to the editor must expose the guild's children (trainers,
// talents) -- the editor renders ChildrenOf(found). Regression for "no talents section".
TEST(GuildsLoader_Vnum, FoundGuildExposesItsChildren) {
	const char *src = "guilds_children_src.xml";
	{
		std::ofstream f(src);
		f << R"(<guilds><guild text_id="kElder" vnum="0" name="x">)"
		     R"(<trainers vnums="1"/>)"
		     R"(<talents><skill id="kPunch" fail="0"><upgrade practices="10" min="0" max="10"/></skill></talents>)"
		     R"(</guild></guilds>)";
	}
	parser_wrapper::DataNode doc(src);
	guilds::GuildsLoader loader;

	auto g = loader.FindElementNode(doc, "0");
	ASSERT_TRUE(g.IsNotEmpty());
	bool has_trainers = false, has_talents = false;
	for (auto &c : g.Children()) {
		const std::string n = c.GetName();
		if (n == "trainers") { has_trainers = true; }
		if (n == "talents") { has_talents = true; }
	}
	EXPECT_TRUE(has_trainers);
	EXPECT_TRUE(has_talents);
	std::remove(src);
}

// The int specialization of MsgContainer (used by guild_msg.xml): "kDefault" is the shared sheaf
// (info_container::kUndefinedVnum); a per-vnum sheaf overrides it for that guild and falls back to
// the default for any message type it does not define; an unknown vnum falls back to the default.
TEST(GuildMessages_IntContainer, DefaultSheafVnumOverrideAndFallback) {
	const char *src = "guild_msgc_src.xml";
	{
		std::ofstream f(src);
		f << R"(<msg_container>)"
		     R"(<msg_sheaf id="kDefault"><message type="kGreeting" val="hi"/><message type="kError" val="err"/></msg_sheaf>)"
		     R"(<msg_sheaf id="6"><message type="kGreeting" val="hi6"/></msg_sheaf>)"
		     R"(</msg_container>)";
	}
	parser_wrapper::DataNode doc(src);
	msg_container::MsgContainer<int, guilds::EMsg> c;
	c.Init(doc.Children());

	EXPECT_EQ(c.GetMessage(info_container::kUndefinedVnum, guilds::EMsg::kGreeting), "hi");  // default sheaf
	EXPECT_EQ(c.GetMessage(6, guilds::EMsg::kGreeting), "hi6");                              // per-vnum override
	EXPECT_EQ(c.GetMessage(6, guilds::EMsg::kError), "err");     // vnum 6 lacks kError -> default sheaf
	EXPECT_EQ(c.GetMessage(999, guilds::EMsg::kGreeting), "hi"); // unknown vnum -> default sheaf
	std::remove(src);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
