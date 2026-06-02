#include <gtest/gtest.h>

#include "simulator/character_builder.h"
#include "gameplay/core/constants.h"

TEST(CharacterBuilder, MakeBasicPlayerSetsClassLevelAndStats) {
	simulator::CharacterBuilder b;
	b.make_basic_player(static_cast<short>(ECharClass::kSorcerer), 30);
	const auto ch = b.get();
	ASSERT_NE(ch, nullptr);
	EXPECT_EQ(ch->GetClass(), ECharClass::kSorcerer);
	EXPECT_EQ(ch->GetLevel(), 30);
	EXPECT_EQ(ch->get_str(), 25);
	EXPECT_EQ(ch->get_dex(), 25);
	EXPECT_EQ(ch->get_con(), 25);
	EXPECT_EQ(ch->get_int(), 25);
	EXPECT_EQ(ch->get_wis(), 25);
	EXPECT_EQ(ch->get_cha(), 25);
}

TEST(CharacterBuilder, IndividualStatSettersOverrideDefaults) {
	simulator::CharacterBuilder b;
	b.make_basic_player(static_cast<short>(ECharClass::kSorcerer), 30);
	b.set_str(50);
	b.set_wis(70);
	const auto ch = b.get();
	EXPECT_EQ(ch->get_str(), 50);
	EXPECT_EQ(ch->get_wis(), 70);
	EXPECT_EQ(ch->get_dex(), 25);  // unchanged
}

TEST(CharacterBuilder, HitSettersUpdateBothFields) {
	simulator::CharacterBuilder b;
	b.make_basic_player(static_cast<short>(ECharClass::kSorcerer), 30);
	b.set_max_hit(1000);
	b.set_hit(500);
	const auto ch = b.get();
	EXPECT_EQ(ch->get_max_hit(), 1000);
	EXPECT_EQ(ch->get_hit(), 500);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
