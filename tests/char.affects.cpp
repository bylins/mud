#include "char.utilities.hpp"

#include <gtest/gtest.h>

test_utils::CharacterBuilder builder;

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_RemoveZero)
{
	builder.create_new();
	auto character = builder.get();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_RemoveOne)
{
	builder.create_new();
	auto character = builder.get();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_Remove100500)
{
	builder.create_new();
	auto character = builder.get();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_RemoveZero)
{
	builder.create_character_with_one_removable_affect();
	auto character = builder.get();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(1u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_RemoveOne)
{
	builder.create_character_with_one_removable_affect();
	auto character = builder.get();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_Remove100500)
{
	builder.create_character_with_one_removable_affect();
	auto character = builder.get();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_RemoveZero)
{
	builder.create_character_with_two_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_RemoveOne)
{
	builder.create_character_with_two_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_Remove100500)
{
	builder.create_character_with_two_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveZero)
{
	builder.create_character_with_two_removable_and_two_not_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(4u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveOne)
{
	builder.create_character_with_two_removable_and_two_not_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(3u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveTwo)
{
	builder.create_character_with_two_removable_and_two_not_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(2));
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(2));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_Remove100500)
{
	builder.create_character_with_two_removable_and_two_not_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(100500));
	EXPECT_EQ(2u, character->affected.size());
}
