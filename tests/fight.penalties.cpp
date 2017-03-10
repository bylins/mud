#include "fight.penalties.hpp"

#include "char.utilities.hpp"

#include <gtest/gtest.h>

class FightPenalties : public ::testing::Test
{
public:
	const GroupPenalties& penalties() const { return m_grouping; }

protected:
	virtual void SetUp() override;

private:
	GroupPenalties m_grouping;
};

void FightPenalties::SetUp()
{
	EXPECT_EQ(0, m_grouping.init())
		<< "Couldn't initialize group penalties table.";
}

TEST_F(FightPenalties, TheSameLevels)
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

		GroupPenaltyCalculator penalty(killer.get(), leader.get(), level, penalties());

		EXPECT_EQ(penalty.get(), 0);
	}
}

TEST_F(FightPenalties, UndefinedKillerClass_SameLevels)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new_with_class(CLASS_UNDEFINED);

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new();

	leader_builder.make_group(killer_builder);

	auto killer = killer_builder.get();
	auto leader = leader_builder.get();
	const auto max_level = std::max(killer->get_level(), leader->get_level());
	GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, penalties());

	EXPECT_EQ(penalty.get(), 100);
}

TEST_F(FightPenalties, UndefinedLeaderClass_SameLevels)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new();

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new_with_class(CLASS_UNDEFINED);

	leader_builder.make_group(killer_builder);

	auto killer = killer_builder.get();
	auto leader = leader_builder.get();
	const auto max_level = std::max(killer->get_level(), leader->get_level());
	GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, FightPenalties::penalties());

	EXPECT_EQ(penalty.get(), 100);
}

TEST_F(FightPenalties, MobKillerClass_SameLevels)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new_with_class(CLASS_MOB);

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new();

	leader_builder.make_group(killer_builder);

	auto killer = killer_builder.get();
	auto leader = leader_builder.get();
	const auto max_level = std::max(killer->get_level(), leader->get_level());
	GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, FightPenalties::penalties());

	EXPECT_EQ(penalty.get(), 100);
}

TEST_F(FightPenalties, MobLeaderClass_SameLevels)
{
	test_utils::CharacterBuilder killer_builder;
	killer_builder.create_new();

	test_utils::CharacterBuilder leader_builder;
	leader_builder.create_new_with_class(CLASS_MOB);

	leader_builder.make_group(killer_builder);

	auto killer = killer_builder.get();
	auto leader = leader_builder.get();
	const auto max_level = std::max(killer->get_level(), leader->get_level());
	GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, FightPenalties::penalties());

	EXPECT_EQ(penalty.get(), 100);
}

template <typename HeaderHandler, typename NewRowHandler, typename EndOfRowHandler, typename ItemHandler>
void iterate_over_group_penalties_ext(HeaderHandler header_handler, NewRowHandler new_row_handler, EndOfRowHandler end_of_row_handler, ItemHandler item_handler)
{
	for (int killer_class = CLASS_CLERIC; killer_class < NUM_PLAYER_CLASSES; ++killer_class)
	{
		for (int leader_class = CLASS_CLERIC; leader_class < NUM_PLAYER_CLASSES; ++leader_class)
		{
			test_utils::CharacterBuilder killer_builder;
			killer_builder.create_new_with_class(killer_class);

			test_utils::CharacterBuilder leader_builder;
			leader_builder.create_new_with_class(leader_class);

			leader_builder.make_group(killer_builder);

			header_handler(killer_class, leader_class);
			for (int killer_level = 1; killer_level < LVL_IMMORT; ++killer_level)
			{
				killer_builder.set_level(killer_level);
				new_row_handler(killer_level);
				for (int leader_level = 1; leader_level < LVL_IMMORT; ++leader_level)
				{
					leader_builder.set_level(leader_level);

					auto killer = killer_builder.get();
					auto leader = leader_builder.get();

					item_handler(killer, leader);
				}
				end_of_row_handler();
			}
		}
	}
}

template <typename ItemHandler>
void iterate_over_group_penalties(ItemHandler item_handler)
{
	iterate_over_group_penalties_ext(
		[](const auto, const auto) {},
		[](const auto) {},
		[] {},
		item_handler
	);
}

TEST_F(FightPenalties, NoPenaltyWithing5Levels)
{
	iterate_over_group_penalties(
		[&](const auto killer, const auto leader)
		{
			const auto killer_level = killer->get_level();
			const auto leader_level = leader->get_level();

			if (5 >= std::abs(killer_level - leader_level))
			{
				const auto max_level = std::max(killer_level, leader_level);
				GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, this->penalties());
				EXPECT_EQ(penalty.get(), 0);
			}
		}
	);
}

TEST_F(FightPenalties, HasPenaltyWithingMoreThan5Levels)
{
	iterate_over_group_penalties(
		[&](const auto killer, const auto leader)
		{
			const auto killer_level = killer->get_level();
			const auto leader_level = leader->get_level();

			if (5 < std::abs(killer_level - leader_level))
			{
				const auto max_level = std::max(killer_level, leader_level);
				GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, this->penalties());
				EXPECT_GT(penalty.get(), 0);
			}
		}
	);
}

TEST_F(FightPenalties, DISABLED_PrintTable)
{
	// Anton Gorev(2016-11-25): seems like I stepped on a bug in compiler (g++ (Ubuntu 4.9.4-2ubuntu1~14.04.1) 4.9.4):
	// Lambda does not capture constexpr/const constants. To workaround this bug I removed constexpr keyword.
	int PLACEHOLDER_LENGTH = 4;

	const auto header_handler = [&](const auto killer_class, const auto leader_class)
		{
			std::cout << "Combination: " << killer_class << "/" << leader_class << std::endl
				<< "===================" << std::endl << std::setw(1 + PLACEHOLDER_LENGTH) << "|";
			for (int leader_level = 1; leader_level < LVL_IMMORT; ++leader_level)
			{
				std::cout << std::setw(PLACEHOLDER_LENGTH) << leader_level;
			}
			std::cout << std::endl << "----+";
			for (int i = 1; i < LVL_IMMORT; ++i)
			{
				std::cout << "----";
			}
			std::cout << std::endl;
		};
	const auto new_row_handler = [&](const auto killer_level) { std::cout << std::setw(PLACEHOLDER_LENGTH) << killer_level << "| "; };
	const auto end_of_row_handler = [] { std::cout << std::endl; };
	const auto item_handler = [&](const auto killer, const auto leader)
	{
		const auto killer_level = killer->get_level();
		const auto leader_level = leader->get_level();
		const auto max_level = std::max(killer_level, leader_level);
		GroupPenaltyCalculator penalty(killer.get(), leader.get(), max_level, this->penalties());
		std::cout << std::setw(PLACEHOLDER_LENGTH) << penalty.get();
	};

	iterate_over_group_penalties_ext(
		header_handler,
		new_row_handler,
		end_of_row_handler,
		item_handler
	);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
