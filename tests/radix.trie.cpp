#include <radix.trie.hpp>

#include <gtest/gtest.h>

TEST(RadixTrie, EmptyTrie)
{
	RadixTrie trie;

	EXPECT_FALSE(trie.has_string(""));
	EXPECT_FALSE(trie.has_string("empty"));
}

TEST(RadixTrie, SingleString)
{
	RadixTrie trie;

	const auto single_string = "single string";
	const auto single = "single";
	const auto string = "single";

	trie.add_string(single_string);

	EXPECT_TRUE(trie.has_string(single_string));
	EXPECT_FALSE(trie.has_string(single));
	EXPECT_FALSE(trie.has_string(string));
}

TEST(RadixTrie, EmptyString)
{
	RadixTrie trie;

	const auto empty_string = "";
	const auto other_string = "any other string";

	trie.add_string(empty_string);

	EXPECT_TRUE(trie.has_string(empty_string));
	EXPECT_FALSE(trie.has_string(other_string));
}

TEST(RadixTrie, CoupleStringsWithSamePrefix_NoPrefixInTrie)
{
	RadixTrie trie;

	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	trie.add_string(string1);
	trie.add_string(string2);

	EXPECT_TRUE(trie.has_string(string1));
	EXPECT_TRUE(trie.has_string(string2));
	EXPECT_FALSE(trie.has_string(prefix));
}

TEST(RadixTrie, CoupleStringsWithPrefix_PrefixInTrie)
{
	RadixTrie trie;

	const std::string prefix = "value for the ";
	const auto string1 = prefix + "first string";
	const auto string2 = prefix + "second string";

	trie.add_string(string1);
	trie.add_string(string2);
	trie.add_string(prefix);

	EXPECT_TRUE(trie.has_string(string1));
	EXPECT_TRUE(trie.has_string(string2));
	EXPECT_TRUE(trie.has_string(prefix));
	EXPECT_FALSE(trie.has_string(prefix.substr(0, prefix.size() >> 1)));
	EXPECT_FALSE(trie.has_string(string1.substr(0, prefix.size() + ((string1.size() - prefix.size()) >> 1))));
}

TEST(RadixTrie, AllChars)
{
	RadixTrie trie;

	for (auto i = 0; i < 1 << (sizeof(char)*8); ++i)
	{
		trie.add_string(std::string(2, char(i)));
	}

	for (auto i = 0; i < 1 << (sizeof(char)*8); ++i)
	{
		ASSERT_FALSE(trie.has_string(std::string(1, char(i))));
		ASSERT_TRUE(trie.has_string(std::string(2, char(i))));
		ASSERT_FALSE(trie.has_string(std::string(3, char(i))));
	}
}