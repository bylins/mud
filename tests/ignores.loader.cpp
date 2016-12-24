#include <gtest/gtest.h>

#include "char.utilities.hpp"

#include <ignores.loader.hpp>

TEST(Ignores_Loader, OneIgnore)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: [100500]100501");
	ASSERT_EQ(1, character->get_ignores().size());
	EXPECT_EQ(100500, character->get_ignores().front()->mode);
	EXPECT_EQ(0, character->get_ignores().front()->id);
}

TEST(Ignores_Loader, OneIgnore_TrailingSpaces)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: [100500]100501     ");
	ASSERT_EQ(1, character->get_ignores().size());
	EXPECT_EQ(100500, character->get_ignores().front()->mode);
	EXPECT_EQ(0, character->get_ignores().front()->id);
}

TEST(Ignores_Loader, ManyIgnores_Sequencial)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: [100500]100501");
	ASSERT_EQ(1, character->get_ignores().size());
	EXPECT_EQ(100500, character->get_ignores().front()->mode);
	EXPECT_EQ(0, character->get_ignores().front()->id);

	ignores_loader.load_from_string("Ignr: [100502]100503");
	ASSERT_EQ(2, character->get_ignores().size());
	EXPECT_EQ(100502, character->get_ignores().back()->mode);
	EXPECT_EQ(0, character->get_ignores().back()->id);

	ignores_loader.load_from_string("Ignr: [100504]100505");
	ASSERT_EQ(3, character->get_ignores().size());
	EXPECT_EQ(100504, character->get_ignores().back()->mode);
	EXPECT_EQ(0, character->get_ignores().back()->id);
}

TEST(Ignores_Loader, TwoIgnoreInLine_SpaceSeparator)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: [100500]100501 [100502]100503");
	ASSERT_EQ(2, character->get_ignores().size());

	EXPECT_EQ(100500, character->get_ignores().front()->mode);
	EXPECT_EQ(0, character->get_ignores().front()->id);
	EXPECT_EQ(100502, character->get_ignores().back()->mode);
	EXPECT_EQ(0, character->get_ignores().back()->id);
}

TEST(Ignores_Loader, TwoIgnoreInLine_TabSeparator)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr:\t[100500]100501\t[100502]100503");
	ASSERT_EQ(2, character->get_ignores().size());

	EXPECT_EQ(100500, character->get_ignores().front()->mode);
	EXPECT_EQ(0, character->get_ignores().front()->id);
	EXPECT_EQ(100502, character->get_ignores().back()->mode);
	EXPECT_EQ(0, character->get_ignores().back()->id);
}

TEST(Ignores_Loader, TwoIgnoreInLine_MixedSeparators)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: \t \t[100500]100501\t \t [100502]100503");
	ASSERT_EQ(2, character->get_ignores().size());

	EXPECT_EQ(100500, character->get_ignores().front()->mode);
	EXPECT_EQ(0, character->get_ignores().front()->id);
	EXPECT_EQ(100502, character->get_ignores().back()->mode);
	EXPECT_EQ(0, character->get_ignores().back()->id);
}

TEST(Ignores_Loader, OneBadIgnore_WrongId)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: [100500]id");
	EXPECT_EQ(0, character->get_ignores().size());
}

TEST(Ignores_Loader, OneBadIgnore_WrongMode)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Ignr: [mode]100500");
	EXPECT_EQ(0, character->get_ignores().size());
}

TEST(Ignores_Loader, OneBadIgnore_WrongFormat)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();

	const auto character = builder.get();
	ASSERT_EQ(0, character->get_ignores().size());

	IgnoresLoader ignores_loader(character.get());
	ignores_loader.load_from_string("Size: 50");
	EXPECT_EQ(0, character->get_ignores().size());
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
