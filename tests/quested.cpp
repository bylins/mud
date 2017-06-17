#include "char.utilities.hpp"

#include <quested.hpp>

#include <gtest/gtest.h>

#include <list>

constexpr size_t NUMBER_OF_SIMPLE_TEXTS = 5;
const char* TEXTS[] = {
	"Text 1", "Text 2", "Text 3", "Text 4", "Text 5",
	"@Text 1", "@Text 2", "@Text 3", "@Text 4", "@Text 5"
};

class Quested_F : public ::testing::TestWithParam<int>
{
public:
	Quested_F();

protected:
	void populate_container(const char** texts, const size_t texts_size);
	void populate_container_simple() { populate_container(TEXTS, NUMBER_OF_SIMPLE_TEXTS); }
	void populate_container_ats() { populate_container(TEXTS + NUMBER_OF_SIMPLE_TEXTS, sizeof(TEXTS) / sizeof(*TEXTS) - NUMBER_OF_SIMPLE_TEXTS); }
	void populate_container_mixed() { populate_container(TEXTS, sizeof(TEXTS) / sizeof(*TEXTS)); }

	void test_container();

	Quested quested;
	std::list<std::shared_ptr<char>> strings;
	std::unordered_set<int> vnums;
};

constexpr int DEFAULT_RANDOM_SEED = 100500;
constexpr size_t DEFAULT_CONTAINER_SIZE = 100;
constexpr size_t DEFAULT_ITERATIONS_COUNT = 10;

Quested_F::Quested_F()
{
	std::srand(DEFAULT_RANDOM_SEED);
}

void Quested_F::populate_container(const char** texts, const size_t texts_size)
{
	test_utils::CharacterBuilder builder;
	builder.create_new();
	const auto character = builder.get();

	for (size_t i = 0; i < DEFAULT_CONTAINER_SIZE; ++i)
	{
		int vnum = rand();
		while (vnums.find(vnum) != vnums.end())
		{
			++vnum;
		}
		vnums.insert(vnum);

		const size_t index = i % texts_size;
		
		const size_t length = 1 + strlen(texts[index]);
		char* text = new char[length];
		strncpy(text, texts[index], length);
		strings.push_back(std::shared_ptr<char>(text, std::default_delete<char[]>()));

		EXPECT_NO_THROW(quested.add(character.get(), vnum, text));
	}
}

void Quested_F::test_container()
{
	for (const auto vnum : vnums)
	{
		EXPECT_TRUE(quested.get(vnum));
	}

	for (size_t i = 0; i < DEFAULT_ITERATIONS_COUNT; ++i)
	{
		int vnum = rand();
		while (vnums.find(vnum) != vnums.end())
		{
			++vnum;
		}
		EXPECT_FALSE(quested.get(vnum));
	}

	for (const auto vnum : vnums)
	{
		EXPECT_TRUE(quested.remove(vnum));
	}
}

TEST_F(Quested_F, EmptyContainer_Get)
{
	for (size_t i = 0; i < DEFAULT_ITERATIONS_COUNT; ++i)
	{
		const int vnum = rand();
		EXPECT_FALSE(quested.get(vnum));
	}
}

TEST_F(Quested_F, EmptyContainer_Clear)
{
	EXPECT_NO_THROW(quested.clear());
}

TEST_F(Quested_F, SimpleText_Add_Get_Remove)
{
	populate_container_simple();

	test_container();
}

TEST_F(Quested_F, TextWithAt_Add_Get_Remove)
{
	populate_container_ats();

	test_container();
}

TEST_F(Quested_F, MixedText_Add_Get_Remove)
{
	populate_container_mixed();

	test_container();
}

TEST_F(Quested_F, SimpleText_Clear)
{
	populate_container_simple();

	EXPECT_NO_THROW(quested.clear());
}

TEST_F(Quested_F, TextWithAt_Clear)
{
	populate_container_ats();

	EXPECT_NO_THROW(quested.clear());
}

TEST_F(Quested_F, MixedText_Clear)
{
	populate_container_mixed();

	EXPECT_NO_THROW(quested.clear());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
