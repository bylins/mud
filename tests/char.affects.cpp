#include "char.utilities.hpp"

#include <gtest/gtest.h>

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_RemoveZero)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_RemoveOne)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_WithEmptyAffects_Remove100500)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();
	EXPECT_EQ(0u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_RemoveZero)
{
	test_utils::CharacterBuilder builder;
	builder.create_character_with_one_removable_affect();
	auto character = builder.get();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(1u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_RemoveOne)
{
	test_utils::CharacterBuilder builder;
	builder.create_character_with_one_removable_affect();
	auto character = builder.get();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(1));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_OneRemovableAffect_Remove100500)
{
	test_utils::CharacterBuilder builder;
	builder.create_character_with_one_removable_affect();
	auto character = builder.get();
	EXPECT_EQ(1u, character->affected.size());
	EXPECT_EQ(1u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableAffect_RemoveZero)
{
	test_utils::CharacterBuilder builder;
	builder.create_character_with_two_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(2u, character->affected.size());
}

TEST(CHAR_Affects, DISABLED_RandomlyRemove_TwoRemovableAffect_RemoveOne)
{
	test_utils::CharacterBuilder builder;
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
	test_utils::CharacterBuilder builder;
	builder.create_character_with_two_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(2u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(100500));
	EXPECT_EQ(0u, character->affected.size());
}

TEST(CHAR_Affects, RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveZero)
{
	test_utils::CharacterBuilder builder;
	builder.create_character_with_two_removable_and_two_not_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(0u, character->remove_random_affects(0));
	EXPECT_EQ(4u, character->affected.size());
}

TEST(CHAR_Affects, DISABLED_RandomlyRemove_TwoRemovableTwoNotRemovableAffect_RemoveOne)
{
	test_utils::CharacterBuilder builder;
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
	test_utils::CharacterBuilder builder;
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
	test_utils::CharacterBuilder builder;
	builder.create_character_with_two_removable_and_two_not_removable_affects();
	auto character = builder.get();
	EXPECT_EQ(4u, character->affected.size());
	EXPECT_EQ(2u, character->remove_random_affects(100500));
	EXPECT_EQ(2u, character->affected.size());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
