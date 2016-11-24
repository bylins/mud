#include "fight.penalties.hpp"

#include "char.utilities.hpp"

#include <gtest/gtest.h>

TEST(FightPenalties, TheSameLevels)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new();

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new();
	leader_builder.make_group(killer_builder);

	for (int level = 1; level < LVL_IMMORT; ++level)
	{
		killer_builder.set_level(level);
		leader_builder.set_level(level);

		auto killer = killer_builder.get();
		auto leader = leader_builder.get();
		const auto max_level = level;

		GroupPenalty penalty(killer.get(), leader.get(), level, grouping);

		EXPECT_EQ(penalty.get(), 0);
	}
}

TEST(FightPenalties, UndefinedKillerClass_SameLevels)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new_with_class(CLASS_UNDEFINED);

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new();

	leader_builder.make_group(killer_builder);

	auto killer = killer_builder.get();
	auto leader = leader_builder.get();
	GroupPenalty penalty(killer.get(), leader.get(), std::max(killer->get_level(), leader->get_level()), grouping);

	EXPECT_EQ(penalty.get(), 100);
}

TEST(FightPenalties, UndefinedLeaderClass)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new();

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new_with_class(CLASS_UNDEFINED);

	leader_builder.make_group(killer_builder);

	auto killer = killer_builder.get();
	auto leader = leader_builder.get();
	GroupPenalty penalty(killer.get(), leader.get(), std::max(killer->get_level(), leader->get_level()), grouping);

	EXPECT_EQ(penalty.get(), 100);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
