#include <compact.trie.hpp>

#include <gtest/gtest.h>

class CompactTrieBasics: public ::testing::Test
{
protected:
	BasicCompactTrie trie;
};

TEST_F(CompactTrieBasics, EmptyTrie)
{
	EXPECT_FALSE(trie.has_string(""));
	EXPECT_FALSE(trie.has_string("empty"));

	EXPECT_EQ(0, trie.size());
}

TEST_F(CompactTrieBasics, SingleString)
{
	const auto single_string = "single string";
	const auto single = "single";
	const auto string = "string";

	EXPECT_TRUE(trie.add_string(single_string));

	EXPECT_TRUE(trie.has_string(single_string));
	EXPECT_FALSE(trie.has_string(single));
	EXPECT_FALSE(trie.has_string(string));

	EXPECT_EQ(1, trie.size());
}

TEST_F(CompactTrieBasics, EmptyString)
{
	const auto empty_string = "";
	const auto other_string = "any other string";

	EXPECT_TRUE(trie.add_string(empty_string));

	EXPECT_TRUE(trie.has_string(empty_string));
	EXPECT_FALSE(trie.has_string(other_string));

	EXPECT_EQ(0, trie.size());
}

TEST_F(CompactTrieBasics, CoupleStringsWithSamePrefix_NoPrefixInTrie)
{
	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	EXPECT_TRUE(trie.add_string(string1));
	EXPECT_TRUE(trie.add_string(string2));

	EXPECT_TRUE(trie.has_string(string1));
	EXPECT_TRUE(trie.has_string(string2));
	EXPECT_FALSE(trie.has_string(prefix));

	EXPECT_EQ(2, trie.size());
}

TEST_F(CompactTrieBasics, CoupleStringsWithPrefix_PrefixInTrie)
{
	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	EXPECT_TRUE(trie.add_string(prefix));
	EXPECT_TRUE(trie.add_string(string1));
	EXPECT_TRUE(trie.add_string(string2));

	EXPECT_TRUE(trie.has_string(string1));
	EXPECT_TRUE(trie.has_string(string2));
	EXPECT_TRUE(trie.has_string(prefix));
	EXPECT_FALSE(trie.has_string(prefix.substr(0, prefix.size() >> 1)));
	EXPECT_FALSE(trie.has_string(string1.substr(0, prefix.size() + ((string1.size() - prefix.size()) >> 1))));

	EXPECT_EQ(3, trie.size());
}

TEST_F(CompactTrieBasics, Order)
{
	const std::string str111 = "111";
	const std::string str12 = "12";
	const std::string str112 = "112";
	const std::string str123 = "123";

	EXPECT_TRUE(trie.add_string(str111));
	EXPECT_TRUE(trie.add_string(str12));
	EXPECT_TRUE(trie.add_string(str112));
	EXPECT_FALSE(trie.add_string(str123));

	EXPECT_TRUE(trie.has_string(str111));
	EXPECT_TRUE(trie.has_string(str12));
	EXPECT_TRUE(trie.has_string(str112));
	EXPECT_FALSE(trie.has_string(str123));

	EXPECT_EQ(3, trie.size());
}

TEST_F(CompactTrieBasics, AllChars)
{
	constexpr size_t CHARS_NUMBER = 1 << (sizeof(char) * 8);

	for (auto i = 0; i < CHARS_NUMBER; ++i)
	{
		ASSERT_TRUE(trie.add_string(std::string(2, char(i))));
	}

	for (auto i = 0; i < CHARS_NUMBER; ++i)
	{
		ASSERT_FALSE(trie.has_string(std::string(1, char(i))));
		ASSERT_TRUE(trie.has_string(std::string(2, char(i))));
		ASSERT_FALSE(trie.has_string(std::string(3, char(i))));
	}

	EXPECT_EQ(CHARS_NUMBER, trie.size());
}

class CompactTrieF : public ::testing::Test
{
protected:
	CompactTrie trie;
};

TEST_F(CompactTrieF, Order)
{
	const std::string str111 = "111";
	const std::string str12 = "12";
	const std::string str112 = "112";
	const std::string str123 = "123";

	EXPECT_TRUE(trie.add_string(str111));
	EXPECT_TRUE(trie.add_string(str12));
	EXPECT_TRUE(trie.add_string(str112));
	EXPECT_TRUE(trie.add_string(str123));

	EXPECT_TRUE(trie.has_string(str111));
	EXPECT_TRUE(trie.has_string(str12));
	EXPECT_TRUE(trie.has_string(str112));
	EXPECT_TRUE(trie.has_string(str123));

	EXPECT_EQ(4, trie.size());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
