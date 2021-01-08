#include "char.utilities.hpp"
#include "global.objects.hpp"

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

TEST(CHAR_Leaders, SimpleLoop)
{
	test_utils::CharacterBuilder builder;

	builder.create_new();
	auto character = builder.get();

	character->set_master(character.get());
	EXPECT_EQ(false, character->has_master());
	EXPECT_EQ(nullptr, character->get_master());
}

TEST(CHAR_Leaders, Group) {
    std::string pName;
    test_utils::CharacterBuilder builder;

    builder.create_new();
    builder.add_skill(ESkill::SKILL_LEADERSHIP, 200);
    auto leader = builder.get();
    test_utils::GroupBuilder g;
    auto grp = g._roster->addGroup(leader.get());

    builder.create_new("F7");
    builder.add_skill(ESkill::SKILL_LEADERSHIP, 10);
    auto f7 = builder.get();
    leader->add_follower(f7.get());
    grp->addFollowers(leader.get());


    for (int i = 0; i <12; i++) {
        pName = "Player" + std::to_string(i);
        builder.create_new(pName);
        auto follower = builder.get();
        leader->add_follower(follower.get());
    }
    grp->addFollowers(leader.get());
    EXPECT_EQ(0, leader->personGroup->get_size(100));
    EXPECT_EQ(11, leader->personGroup->size());
    grp->promote("F7");
    EXPECT_EQ(7, leader->personGroup->size());

}

TEST(CHAR_Leaders, GroupMigration) {
    test_utils::GroupBuilder g;

    auto grp = g.makeFullGroup(200);

    EXPECT_EQ(11, grp->get_size(0));
    grp->promote("Player-2");
    EXPECT_EQ(7, grp->get_size(0));

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
