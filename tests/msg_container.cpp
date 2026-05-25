// Part of Bylins http://www.mud.ru
// Unit tests for msg_container: MsgSheaf / MsgSheafBuilder / MsgContainer (issue #3294).
//
// The container stores message strings opaquely, so ASCII placeholder values are
// used here. Real game data stores Russian player messages (KOI8-R).
//
// Names are deliberately unique (EMsgTest*, MsgTest*) so they do not collide
// with other test sources under a unity build.

#include "engine/structs/msg_container.h"

#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>

// Throwaway element-id enum (must provide kUndefined for the default slot).
enum class EMsgTestElem {
	kUndefined,
	kFireball,
	kHeal
};

// Throwaway per-element message-type enum.
enum class EMsgTestKind {
	kStart,
	kSuccess,
	kFail,
	kTick
};

// --- enum <-> name specializations required by the meta_enum mechanism ---
// The XML "id" of the default sheaf ("kDefault") maps to EMsgTestElem::kUndefined.

template<>
EMsgTestElem ITEM_BY_NAME<EMsgTestElem>(const std::string &name) {
	static const std::map<std::string, EMsgTestElem> kMap{
		{"kDefault", EMsgTestElem::kUndefined},
		{"kFireball", EMsgTestElem::kFireball},
		{"kHeal", EMsgTestElem::kHeal},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

template<>
const std::string &NAME_BY_ITEM<EMsgTestElem>(const EMsgTestElem item) {
	static const std::map<EMsgTestElem, std::string> kMap{
		{EMsgTestElem::kUndefined, "kDefault"},
		{EMsgTestElem::kFireball, "kFireball"},
		{EMsgTestElem::kHeal, "kHeal"},
	};
	return kMap.at(item);
}

template<>
EMsgTestKind ITEM_BY_NAME<EMsgTestKind>(const std::string &name) {
	static const std::map<std::string, EMsgTestKind> kMap{
		{"kStart", EMsgTestKind::kStart},
		{"kSuccess", EMsgTestKind::kSuccess},
		{"kFail", EMsgTestKind::kFail},
		{"kTick", EMsgTestKind::kTick},
	};
	return kMap.at(name);
}

template<>
const std::string &NAME_BY_ITEM<EMsgTestKind>(const EMsgTestKind item) {
	static const std::map<EMsgTestKind, std::string> kMap{
		{EMsgTestKind::kStart, "kStart"},
		{EMsgTestKind::kSuccess, "kSuccess"},
		{EMsgTestKind::kFail, "kFail"},
		{EMsgTestKind::kTick, "kTick"},
	};
	return kMap.at(item);
}

using MsgTestSheaf = msg_container::MsgSheaf<EMsgTestElem, EMsgTestKind>;
using MsgTestContainer = msg_container::MsgContainer<EMsgTestElem, EMsgTestKind>;

static constexpr const char *kMsgTestXmlPath = "data/msg_container/test_messages.xml";

// --- MsgSheaf ---

TEST(MsgSheaf, AddAndGetMessage) {
	MsgTestSheaf sheaf(EMsgTestElem::kFireball, EItemMode::kEnabled);
	sheaf.AddMessage(EMsgTestKind::kStart, "boom");
	EXPECT_EQ("boom", sheaf.GetMessage(EMsgTestKind::kStart));
	EXPECT_TRUE(sheaf.HasMessage(EMsgTestKind::kStart));
	EXPECT_EQ(EMsgTestElem::kFireball, sheaf.GetId());
}

TEST(MsgSheaf, MissingTypeReturnsEmpty) {
	MsgTestSheaf sheaf(EMsgTestElem::kFireball, EItemMode::kEnabled);
	EXPECT_TRUE(sheaf.GetMessage(EMsgTestKind::kFail).empty());
	EXPECT_FALSE(sheaf.HasMessage(EMsgTestKind::kFail));
}

TEST(MsgSheaf, GetMessagePicksAmongAllVariants) {
	MsgTestSheaf sheaf(EMsgTestElem::kFireball, EItemMode::kEnabled);
	sheaf.AddMessage(EMsgTestKind::kSuccess, "a");
	sheaf.AddMessage(EMsgTestKind::kSuccess, "b");

	std::set<std::string> seen;
	for (int i = 0; i < 1000; ++i) {
		const auto &message = sheaf.GetMessage(EMsgTestKind::kSuccess);
		ASSERT_TRUE(message == "a" || message == "b");
		seen.insert(message);
	}
	EXPECT_EQ(2u, seen.size()); // both variants appear over 1000 random draws
}

// --- MsgContainer (full XML parse via parser_wrapper) ---

TEST(MsgContainer, LoadsAndReturnsMessages) {
	parser_wrapper::DataNode data(kMsgTestXmlPath);
	ASSERT_TRUE(static_cast<bool>(data));

	MsgTestContainer container;
	container.Init(data.Children());

	EXPECT_EQ("fireball-start", container.GetMessage(EMsgTestElem::kFireball, EMsgTestKind::kStart));
	EXPECT_EQ("fireball-fail", container.GetMessage(EMsgTestElem::kFireball, EMsgTestKind::kFail));

	const auto &hit = container.GetMessage(EMsgTestElem::kFireball, EMsgTestKind::kSuccess);
	EXPECT_TRUE(hit == "fireball-hit-a" || hit == "fireball-hit-b");
}

TEST(MsgContainer, FallsBackToDefaultSheaf) {
	parser_wrapper::DataNode data(kMsgTestXmlPath);
	MsgTestContainer container;
	container.Init(data.Children());

	// kFireball has no kTick message -> default sheaf supplies it.
	EXPECT_EQ("default-tick", container.GetMessage(EMsgTestElem::kFireball, EMsgTestKind::kTick));
	// Unknown element kHeal -> default sheaf supplies kFail.
	EXPECT_EQ("default-fail", container.GetMessage(EMsgTestElem::kHeal, EMsgTestKind::kFail));
}

TEST(MsgContainer, ReturnsLiteralFallbackWhenNothingMatches) {
	parser_wrapper::DataNode data(kMsgTestXmlPath);
	MsgTestContainer container;
	container.Init(data.Children());

	// Unknown element + a type the default sheaf lacks (kStart) -> literal error string.
	EXPECT_EQ("ERROR: message not found",
			  container.GetMessage(EMsgTestElem::kHeal, EMsgTestKind::kStart));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
