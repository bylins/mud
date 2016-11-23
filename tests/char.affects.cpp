#include "char.utilities.hpp"

#include <gtest/gtest.h>

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_RemoveZero)
{
	auto character = create_character();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_RemoveOne)
{
	auto character = create_character();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_Remove100500)
{
	auto character = create_character();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_RemoveZero)
{
	auto character = create_character_with_one_removable_affect();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(1u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_RemoveOne)
{
	auto character = create_character_with_one_removable_affect();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_Remove100500)
{
	auto character = create_character_with_one_removable_affect();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_RemoveZero)
{
	auto character = create_character_with_two_removable_affects();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_RemoveOne)
{
	auto character = create_character_with_two_removable_affects();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_Remove100500)
{
	auto character = create_character_with_two_removable_affects();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveZero)
{
	auto character = create_character_with_two_removable_and_two_not_removable_affects();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(4u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveOne)
{
	auto character = create_character_with_two_removable_and_two_not_removable_affects();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(3u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveTwo)
{
	auto character = create_character_with_two_removable_and_two_not_removable_affects();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(2));
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(2));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_Remove100500)
{
	auto character = create_character_with_two_removable_and_two_not_removable_affects();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(100500));
	EXPECT_EQ(2u, character->affected.size());
}
