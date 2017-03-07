#include <compact.trie.hpp>

#include <gtest/gtest.h>

TEST(CompactTrie, EmptyTrie)
{
	CompactTrie trie;

	EXPECT_FALSE(trie.has_string(""));
	EXPECT_FALSE(trie.has_string("empty"));
}

TEST(CompactTrie, SingleString)
{
	CompactTrie trie;

	const auto single_string = "single string";
	const auto single = "single";
	const auto string = "string";

	EXPECT_TRUE(trie.add_string(single_string));

	EXPECT_TRUE(trie.has_string(single_string));
	EXPECT_FALSE(trie.has_string(single));
	EXPECT_FALSE(trie.has_string(string));
}

TEST(CompactTrie, EmptyString)
{
	CompactTrie trie;

	const auto empty_string = "";
	const auto other_string = "any other string";

	EXPECT_FALSE(trie.add_string(empty_string));

	EXPECT_FALSE(trie.has_string(empty_string));
	EXPECT_FALSE(trie.has_string(other_string));
}

TEST(CompactTrie, CoupleStringsWithSamePrefix_NoPrefixInTrie)
{
	CompactTrie trie;

	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	EXPECT_TRUE(trie.add_string(string1));
	EXPECT_TRUE(trie.add_string(string2));

	EXPECT_TRUE(trie.has_string(string1));
	EXPECT_TRUE(trie.has_string(string2));
	EXPECT_FALSE(trie.has_string(prefix));
}

TEST(CompactTrie, CoupleStringsWithPrefix_PrefixInTrie)
{
	CompactTrie trie;

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
}

TEST(CompactTrie, Order)
{
	CompactTrie trie;

	const std::string str111 = "111";
	const std::string str12 = "12";
	const std::string str112 = "112";
	const std::string str123 = "123";

	EXPECT_TRUE(trie.add_string(str111));
	EXPECT_TRUE(trie.add_string(str12));
	EXPECT_TRUE(trie.add_string(str112));
	EXPECT_FALSE(trie.add_string(str123));
}

TEST(CompactTrie, AllChars)
{
	CompactTrie trie;

	for (auto i = 0; i < 1 << (sizeof(char)*8); ++i)
	{
		ASSERT_TRUE(trie.add_string(std::string(2, char(i))));
	}

	for (auto i = 0; i < 1 << (sizeof(char)*8); ++i)
	{
		ASSERT_FALSE(trie.has_string(std::string(1, char(i))));
		ASSERT_TRUE(trie.has_string(std::string(2, char(i))));
		ASSERT_FALSE(trie.has_string(std::string(3, char(i))));
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
