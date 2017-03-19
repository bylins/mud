#include <compact.trie.hpp>

#include <gtest/gtest.h>

class CompactTriePrefixes : public ::testing::Test
{
protected:
	using strings_t = std::vector<std::string>;

	void fill_trie(strings_t strings);

	CompactTrie trie;
};

void CompactTriePrefixes::fill_trie(strings_t strings)
{
	std::sort(strings.begin(), strings.end());

	for (const auto s : strings)
	{
		ASSERT_TRUE(trie.add_string(s));
	}
}

TEST_F(CompactTriePrefixes, EmptyTrie_EmptyPrefix)
{
	const auto range = trie.find_prefix("");

	EXPECT_TRUE(range.empty());
}

TEST_F(CompactTriePrefixes, EmptyTrie_NotEmptyPrefix)
{
	const auto range = trie.find_prefix("prefix");

	EXPECT_TRUE(range.empty());
}

TEST_F(CompactTriePrefixes, OneElementTrie_EmptyPrefix)
{
	const std::string PREFIX_VALUE = "prefix";

	fill_trie({ PREFIX_VALUE });

	const auto range = trie.find_prefix("");

	EXPECT_TRUE(PREFIX_VALUE == range.begin()->prefix());
	EXPECT_TRUE(++range.begin() == range.end());
}

TEST_F(CompactTriePrefixes, OneElementTrie_Prefix)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE = PREFIX_VALUE + " value";

	fill_trie({ STRING_VALUE });

	const auto range = trie.find_prefix(PREFIX_VALUE);

	EXPECT_EQ(PREFIX_VALUE, range.prefix());
	EXPECT_EQ(STRING_VALUE, range.begin()->prefix());
	EXPECT_TRUE(++range.begin() == range.end());
}

TEST_F(CompactTriePrefixes, OneElementTrie_PrefixLeaf)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE = PREFIX_VALUE + " value";

	fill_trie({ STRING_VALUE });

	const auto range = trie.find_prefix(STRING_VALUE);

	EXPECT_EQ(STRING_VALUE, range.prefix());
	EXPECT_EQ(STRING_VALUE, range.begin()->prefix());
	EXPECT_TRUE(++range.begin() == range.end());
}

TEST_F(CompactTriePrefixes, TrieWithNonLeafFork)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "2";

	const strings_t strings = { STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto range = trie.find_prefix(PREFIX_VALUE);

	EXPECT_EQ(PREFIX_VALUE, range.prefix());
	size_t i = 0;
	for (const auto string : range)
	{
		ASSERT_LT(i, strings.size());
		ASSERT_EQ(string.prefix(), strings[i++]);
	}
	ASSERT_EQ(i, strings.size());
}

TEST_F(CompactTriePrefixes, TrieWithLeafFork)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "2";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto range = trie.find_prefix(PREFIX_VALUE);

	EXPECT_EQ(PREFIX_VALUE, range.prefix());
	size_t i = 0;
	for (const auto string : range)
	{
		ASSERT_LT(i, strings.size());
		ASSERT_EQ(string.prefix(), strings[i++]);
	}
	ASSERT_EQ(i, strings.size());
}