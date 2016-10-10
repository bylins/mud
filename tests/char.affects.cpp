#include <char.hpp>
#include <handler.h>

#include <gtest/gtest.h>

#include <memory>

auto create_character()
{
	return std::shared_ptr<CHAR_DATA>(new CHAR_DATA());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects)
{
	auto character = create_character();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}