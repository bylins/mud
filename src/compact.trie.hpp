#ifndef __COMPACT_TRIE_HPP__
#define __COMPACT_TRIE_HPP__

#include <vector>

class CompactTrie
{
public:
	static constexpr size_t CHARS_NUMBER = 1 << (sizeof(char) * 8);
	static constexpr size_t NO_INDEX = ~0u;

	CompactTrie() {}

	/// Each next string must be added into trie in lexical order
	bool add_string(const std::string& string);
	bool has_string(const std::string& string) const;

private:
	struct Node
	{
		Node(const char character) : character(character), has_child(false), is_terminal(false), sibling(NO_INDEX) {}

		char character;
		bool has_child;
		bool is_terminal;
		size_t sibling;
	};

	size_t get_or_create_sibling(const char c, size_t offset);
	size_t find_sibling(const char c, size_t offset) const;

	std::vector<Node> m_contents;
};

constexpr size_t CompactTrie::CHARS_NUMBER;
constexpr size_t CompactTrie::NO_INDEX;

#endif // __COMPACT_TRIE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
