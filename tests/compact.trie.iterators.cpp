#include "engine/structs/compact_trie.h"

#include <gtest/gtest.h>

#include <map>
#include <algorithm>

class CompactTrieIterators: public ::testing::Test
{
protected:
	using strings_t = std::vector<std::string>;

	void check(strings_t strings);

	BasicCompactTrie trie;
};

void CompactTrieIterators::check(strings_t strings)
{
	std::sort(strings.begin(), strings.end());

	for (const auto &s : strings)
	{
		ASSERT_TRUE(trie.add_string(s));
	}

	size_t i = 0;
	for (const auto &subtree : trie)
	{
		const std::string& word = subtree.prefix();
		ASSERT_TRUE(word == strings[i])
			<< "String [" << word << "] in the trie does not match the original string [" << strings[i] << "]";
		++i;
	}
}

TEST_F(CompactTrieIterators, EmptyTrie)
{
	EXPECT_TRUE(trie.begin() == trie.end());
}

TEST_F(CompactTrieIterators, SingleElement)
{
	const strings_t strings = {"sample string"};

	check(strings);
}

TEST_F(CompactTrieIterators, ManyElements_SingleTree_TwoLeaves)
{
	const strings_t strings = {
		"1111",
		"1121"
	};

	check(strings);
}

TEST_F(CompactTrieIterators, ManyElements_SingleTree_TwoLeavesAndTheirPrefixes)
{
	const strings_t strings = {
		"1",
		"11",
		"111",
		"1111",
		"112",
		"1121"
	};

	check(strings);
}

TEST_F(CompactTrieIterators, ManyElements_TwoTrees_TwoLeaves)
{
	const strings_t strings = {
		"1111",
		"1121",
		"2111",
		"2121"
	};

	check(strings);
}

TEST_F(CompactTrieIterators, ManyElements_TwoTrees_TwoLeavesAndTheirPrefixes)
{
	const strings_t strings = {
		"1",
		"11",
		"111",
		"1111",
		"112",
		"1121"
		"2",
		"21",
		"211",
		"2111",
		"212",
		"2121"
	};

	check(strings);
}

TEST_F(CompactTrieIterators, ManyElements_Subranges)
{
	using tree_t = std::map<std::string, strings_t>;
	const tree_t tree = {
		{"1", {
			"11",
			"111",
			"1111",
			"112",
			"1121"
		}},
		{"2", {
			"21",
			"211",
			"2111",
			"212",
			"2121"
		}}
	};

	strings_t strings;

	for (const auto& v : tree)
	{
		EXPECT_TRUE(trie.add_string(v.first));
		for (const auto& s : v.second)
		{
			EXPECT_TRUE(trie.add_string(s));
		}
	}

	for (const auto &subtree : trie)
	{
		const tree_t::const_iterator tree_i = tree.find(subtree.prefix());
		if (tree_i != tree.end())
		{
			// check subtrie
			size_t i = 0;
			for (const auto &r : subtree)
			{
				const std::string& word = r.prefix();
				if (subtree.prefix() == word)
				{
					continue;
				}
				ASSERT_TRUE(word == tree_i->second[i])
					<< "String " << word << " in the trie does not match the original string " << tree_i->second[i];
				++i;
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
