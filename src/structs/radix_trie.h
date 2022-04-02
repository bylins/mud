#ifndef __TRIE_HPP__
#define __TRIE_HPP__

#include <memory>
#include <array>
#include <string>
#include <vector>

class RadixTrie {
 public:
	using shared_ptr = std::shared_ptr<RadixTrie>;

	class DfsIterator {
	 public:
		DfsIterator(const RadixTrie::shared_ptr &trie) : trie_(trie) {}

	 private:
		RadixTrie::shared_ptr trie_;
	};

	bool AddString(const std::string &string);
	bool HasString(const std::string &string) const;

 private:
	class Node {
	 public:
		using shared_ptr = std::shared_ptr<Node>;
		using Children = std::vector<Node::shared_ptr>;

		Node(const std::string &string) : piece_(string), children_(1 << (sizeof(char) * 8)), is_terminal_(true) {}

		void AddSuffix(const std::string &string, size_t from);
		bool HasSuffix(const std::string &string, size_t from) const;

	 private:
		std::string piece_;
		Children children_;
		bool is_terminal_;
	};

	Node::shared_ptr root_;
};

#endif // __TRIE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
