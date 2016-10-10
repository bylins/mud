#include <char.hpp>
#include <handler.h>

#include <gtest/gtest.h>

#include <memory>

auto create_character()
{
	return std::shared_ptr<CHAR_DATA>(new CHAR_DATA());
}

auto create_character_with_one_removable_affect()
{
	auto character = create_character();
	AFFECT_DATA<EApplyLocation> poison;
	poison.type = SPELL_POISON;
	poison.modifier = 0;
	poison.location = APPLY_STR;
	poison.duration = pc_duration(character.get(), 10*2, 0, 0, 0, 0);
	poison.bitvector = to_underlying(EAffectFlag::AFF_POISON);
	poison.battleflag = AF_SAME_TIME;
	affect_join(character.get(), poison, false, false, false, false);
	return character;
}

auto create_character_with_two_removable_affects()
{
	auto character = create_character_with_one_removable_affect();
	AFFECT_DATA<EApplyLocation> sleep;
	sleep.type = SPELL_SLEEP;
	sleep.modifier = 0;
	sleep.location = APPLY_AC;
	sleep.duration = pc_duration(character.get(), 10*2, 0, 0, 0, 0);
	sleep.bitvector = to_underlying(EAffectFlag::AFF_SLEEP);
	sleep.battleflag = AF_SAME_TIME;
	affect_join(character.get(), sleep, false, false, false, false);
	return character;
}

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
