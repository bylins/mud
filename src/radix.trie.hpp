#ifndef __TRIE_HPP__
#define __TRIE_HPP__

#include <memory>
#include <array>
#include <string>
#include <vector>

class RadixTrie
{
public:
	using shared_ptr = std::shared_ptr<RadixTrie>;

	class DFS_Iterator
	{
	public:
		DFS_Iterator(const RadixTrie::shared_ptr& trie) : m_trie(trie) {}

	private:
		RadixTrie::shared_ptr m_trie;
	};

	bool add_string(const std::string& string);
	bool has_string(const std::string& string) const;

private:
	class Node
	{
	public:
		using shared_ptr = std::shared_ptr<Node>;
		using children_t = std::vector<Node::shared_ptr>;

		Node(const std::string& string) : m_piece(string), m_children(1 << (sizeof(char)*8)), m_is_terminal(true) {}

		void add_suffix(const std::string& string, size_t from);
		bool has_suffix(const std::string& string, size_t from) const;

	private:
		std::string m_piece;
		children_t m_children;
		bool m_is_terminal;
	};

	Node::shared_ptr m_root;
};

#endif // __TRIE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
