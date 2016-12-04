#include "char.utilities.hpp"

#include <gtest/gtest.h>

TEST(CHAR_Leaders, Initialization)
{
	test_utils::CharacterBuilder builder;

	builder.create_new();
	auto character = builder.get();

	EXPECT_EQ(false, character->has_master());
	EXPECT_EQ(nullptr, character->get_master());
}

TEST(CHAR_Leaders, Pair)
{
	test_utils::CharacterBuilder builder;

	builder.create_new();
	auto leader = builder.get();

	builder.create_new();
	auto follower = builder.get();

	follower->set_master(leader.get());
	EXPECT_EQ(true, follower->has_master());
	EXPECT_EQ(false, leader->has_master());
	EXPECT_EQ(leader.get(), follower->get_master());
	EXPECT_EQ(nullptr, leader->get_master());
}

TEST(CHAR_Leaders, TripleChain)
{
	test_utils::CharacterBuilder builder;

	builder.create_new();
	auto character1 = builder.get();
	builder.create_new();
	auto character2 = builder.get();
	builder.create_new();
	auto character3 = builder.get();

	character1->set_master(character2.get());
	character2->set_master(character3.get());

	EXPECT_EQ(true, character1->has_master());
	EXPECT_EQ(true, character2->has_master());
	EXPECT_EQ(false, character3->has_master());

	EXPECT_EQ(character2.get(), character1->get_master());
	EXPECT_EQ(character3.get(), character2->get_master());
	EXPECT_EQ(nullptr, character3->get_master());
}

TEST(CHAR_Leaders, LoopCheck)
{
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
