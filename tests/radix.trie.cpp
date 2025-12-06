#include "engine/structs/radix_trie.h"

#include <gtest/gtest.h>

TEST(RadixTrie, EmptyTrie)
{
	RadixTrie trie;

	EXPECT_FALSE(trie.HasString(""));
	EXPECT_FALSE(trie.HasString("empty"));
}

TEST(RadixTrie, SingleString)
{
	RadixTrie trie;

	const auto single_string = "single string";
	const auto single = "single";
	const auto string = "string";

	EXPECT_TRUE(trie.AddString(single_string));

	EXPECT_TRUE(trie.HasString(single_string));
	EXPECT_FALSE(trie.HasString(single));
	EXPECT_FALSE(trie.HasString(string));
}

TEST(RadixTrie, EmptyString)
{
	RadixTrie trie;

	const auto empty_string = "";
	const auto other_string = "any other string";

	EXPECT_TRUE(trie.AddString(empty_string));

	EXPECT_TRUE(trie.HasString(empty_string));
	EXPECT_FALSE(trie.HasString(other_string));
}

TEST(RadixTrie, CoupleStringsWithSamePrefix_NoPrefixInTrie)
{
	RadixTrie trie;

	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	EXPECT_TRUE(trie.AddString(string1));
	EXPECT_TRUE(trie.AddString(string2));

	EXPECT_TRUE(trie.HasString(string1));
	EXPECT_TRUE(trie.HasString(string2));
	EXPECT_FALSE(trie.HasString(prefix));
}

TEST(RadixTrie, CoupleStringsWithPrefix_PrefixInTrie)
{
	RadixTrie trie;

	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	EXPECT_TRUE(trie.AddString(prefix));
	EXPECT_TRUE(trie.AddString(string1));
	EXPECT_TRUE(trie.AddString(string2));

	EXPECT_TRUE(trie.HasString(string1));
	EXPECT_TRUE(trie.HasString(string2));
	EXPECT_TRUE(trie.HasString(prefix));
	EXPECT_FALSE(trie.HasString(prefix.substr(0, prefix.size() >> 1)));
	EXPECT_FALSE(trie.HasString(string1.substr(0, prefix.size() + ((string1.size() - prefix.size()) >> 1))));
}

TEST(RadixTrie, Order)
{
	RadixTrie trie;

	const std::string str111 = "111";
	const std::string str12 = "12";
	const std::string str112 = "112";
	const std::string str123 = "123";

	EXPECT_TRUE(trie.AddString(str111));
	EXPECT_TRUE(trie.AddString(str12));
	EXPECT_TRUE(trie.AddString(str112));
	EXPECT_TRUE(trie.AddString(str123));
}

TEST(RadixTrie, AllChars)
{
	RadixTrie trie;

	for (auto i = 0; i < 1 << (sizeof(char) * 8); ++i)
	{
		ASSERT_TRUE(trie.AddString(std::string(2, char(i))));
	}

	for (auto i = 0; i < 1 << (sizeof(char) * 8); ++i)
	{
		ASSERT_FALSE(trie.HasString(std::string(1, char(i))));
		ASSERT_TRUE(trie.HasString(std::string(2, char(i))));
		ASSERT_FALSE(trie.HasString(std::string(3, char(i))));
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
