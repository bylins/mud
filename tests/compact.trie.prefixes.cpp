#include "engine/structs/compact_trie.h"

#include <gtest/gtest.h>

#include <algorithm>

class CompactTriePrefixes : public ::testing::Test
{
protected:
	using strings_t = std::vector<std::string>;

	void fill_trie(strings_t strings);

	BasicCompactTrie trie;
};

void CompactTriePrefixes::fill_trie(strings_t strings)
{
	std::sort(strings.begin(), strings.end());

	for (const auto &s : strings)
	{
		ASSERT_TRUE(trie.add_string(s));
	}
}

TEST_F(CompactTriePrefixes, EmptyTrie_EmptyPrefix)
{
	const auto range = trie.find_by_prefix("");

	EXPECT_TRUE(range.empty());
}

TEST_F(CompactTriePrefixes, EmptyTrie_NotEmptyPrefix)
{
	const auto range = trie.find_by_prefix("prefix");

	EXPECT_TRUE(range.empty());
}

TEST_F(CompactTriePrefixes, OneElementTrie_EmptyPrefix)
{
	const std::string PREFIX_VALUE = "prefix";

	fill_trie({ PREFIX_VALUE });

	const auto range = trie.find_by_prefix("");

	EXPECT_TRUE(PREFIX_VALUE == range.begin()->prefix());
	EXPECT_TRUE(++range.begin() == range.end());
}

TEST_F(CompactTriePrefixes, OneElementTrie_Prefix)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE = PREFIX_VALUE + " value";

	fill_trie({ STRING_VALUE });

	const auto range = trie.find_by_prefix(PREFIX_VALUE);

	EXPECT_EQ(PREFIX_VALUE, range.prefix());
	EXPECT_EQ(STRING_VALUE, range.begin()->prefix());
	EXPECT_TRUE(++range.begin() == range.end());
}

TEST_F(CompactTriePrefixes, OneElementTrie_PrefixLeaf)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE = PREFIX_VALUE + " value";

	fill_trie({ STRING_VALUE });

	const auto range = trie.find_by_prefix(STRING_VALUE);

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

	const auto range = trie.find_by_prefix(PREFIX_VALUE);

	EXPECT_EQ(PREFIX_VALUE, range.prefix());
	size_t i = 0;
	for (const auto &string : range)
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

	const auto range = trie.find_by_prefix(PREFIX_VALUE);

	EXPECT_EQ(PREFIX_VALUE, range.prefix());
	size_t i = 0;
	for (const auto &string : range)
	{
		ASSERT_LT(i, strings.size());
		ASSERT_EQ(string.prefix(), strings[i++]);
	}
	ASSERT_EQ(i, strings.size());
}

TEST_F(CompactTriePrefixes, SequencialSearch)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string SUBPREFIX_VALUE = "2";
	const std::string PREFIX_VALUE_1 = PREFIX_VALUE + SUBPREFIX_VALUE;
	const std::string STRING_VALUE_22 = PREFIX_VALUE_1 + "2";
	const std::string STRING_VALUE_23 = PREFIX_VALUE_1 + "3";

	const strings_t strings = { STRING_VALUE_1, STRING_VALUE_22, STRING_VALUE_23 };
	fill_trie(strings);

	const auto range1 = trie.find_by_prefix(PREFIX_VALUE);

	ASSERT_EQ(PREFIX_VALUE, range1.prefix());

	const auto range2 = range1.find(SUBPREFIX_VALUE);

	ASSERT_EQ(PREFIX_VALUE_1, range2.prefix());

	size_t i = 1;
	for (const auto &string : range2)
	{
		ASSERT_LT(i, strings.size());
		ASSERT_EQ(string.prefix(), strings[i++]);
	}
	ASSERT_EQ(i, strings.size());
}

TEST_F(CompactTriePrefixes, NoPrefix)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "2";
	const std::string MISSING_PREFIX = "missing prefix";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto range = trie.find_by_prefix(MISSING_PREFIX);
	ASSERT_TRUE(range.empty())
		<< range.begin()->prefix();
}

TEST_F(CompactTriePrefixes, NoPrefix_HalfMatch)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "2";
	const std::string MISSING_PREFIX = "preffix";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto range = trie.find_by_prefix(MISSING_PREFIX);
	ASSERT_TRUE(range.empty())
		<< range.begin()->prefix();
}

TEST_F(CompactTriePrefixes, NoPrefix_ExtraChars)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "2";
	const std::string MISSING_PREFIX = "prefix12";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto range = trie.find_by_prefix(MISSING_PREFIX);
	ASSERT_TRUE(range.empty())
		<< range.begin()->prefix();
}

TEST_F(CompactTriePrefixes, NoPrefix_Subrange)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "1";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "2";
	const std::string MISSING_PREFIX = "missing prefix";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto subrange = trie.find_by_prefix(PREFIX_VALUE);
	const auto range = subrange.find(MISSING_PREFIX);
	ASSERT_TRUE(range.empty())
		<< range.begin()->prefix();
}

TEST_F(CompactTriePrefixes, NoPrefix_Subrange_HalfMatch)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "110";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "220";
	const std::string MISSING_PREFIX = "111";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto subrange = trie.find_by_prefix(PREFIX_VALUE);
	const auto range = subrange.find(MISSING_PREFIX);
	ASSERT_TRUE(range.empty())
		<< range.begin()->prefix();
}

TEST_F(CompactTriePrefixes, NoPrefix_Subrange_ExtraChars)
{
	const std::string PREFIX_VALUE = "prefix";
	const std::string STRING_VALUE_1 = PREFIX_VALUE + "110";
	const std::string STRING_VALUE_2 = PREFIX_VALUE + "220";
	const std::string MISSING_PREFIX = "110+220";

	const strings_t strings = { PREFIX_VALUE, STRING_VALUE_1, STRING_VALUE_2 };

	fill_trie(strings);

	const auto subrange = trie.find_by_prefix(PREFIX_VALUE);
	const auto range = subrange.find(MISSING_PREFIX);
	ASSERT_TRUE(range.empty())
		<< range.begin()->prefix();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
