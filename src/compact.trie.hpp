#ifndef __COMPACT_TRIE_HPP__
#define __COMPACT_TRIE_HPP__

#include <vector>
#include <list>
#include <memory>
#include <string>

class BasicCompactTrie
{
public:
	class Range
	{
	public:
		class DFS_Iterator
		{
		public:
			bool operator==(const DFS_Iterator& right) const;
			bool operator!=(const DFS_Iterator& right) const { return !(*this == right); }
			const Range& operator*() const;
			const Range* operator->() const { return &operator*(); }
			DFS_Iterator& operator++() { return next(); }

		private:
			friend class Range;

			using path_t = std::list<size_t>;

			DFS_Iterator(const Range* range);
			DFS_Iterator(const Range* range, const size_t current, const bool end = false);

			DFS_Iterator& next();
			void go_to_first_leaf();
			bool go_to_child();
			bool go_to_sibling();
			bool go_to_parent();
			void build_current() const;

			const Range* m_range;
			size_t m_current;
			path_t m_path;
			mutable std::shared_ptr<Range> m_current_element;
		};

		using iterator = DFS_Iterator;

		bool operator==(const Range& right) const { return m_trie == right.m_trie && m_root == right.m_root; }

		iterator begin() const { return std::move(iterator(this)); }
		iterator end() const { return std::move(iterator(this, NO_INDEX, true)); }
		Range find(const std::string& prefix) const;

		bool empty() const { return begin() == end(); }

		const std::string& prefix() const { return m_prefix; }

	private:
		friend class BasicCompactTrie;

		Range(BasicCompactTrie* trie, const size_t root = 0) : m_trie(trie), m_root(root) {}
		Range(BasicCompactTrie* trie, const size_t root, const std::string& prefix) : m_trie(trie), m_root(root), m_prefix(prefix) {}

		bool go_to_child(size_t& node) const;
		bool go_to_sibling(size_t& node) const;

		BasicCompactTrie* m_trie;
		size_t m_root;
		std::string m_prefix;
	};

	using iterator = Range::iterator;

	static constexpr size_t CHARS_NUMBER = 1 << (sizeof(char) * 8);
	static constexpr size_t NO_INDEX = ~0u;

	BasicCompactTrie(): m_contents(1, Node('\0')), m_range(this) {}

	/// Each next string must be added into trie in lexical order
	bool add_string(const std::string& string);
	bool has_string(const std::string& string) const;

	Range find_by_prefix(const std::string& prefix) const { return m_range.find(prefix); }

	iterator begin() const { return std::move(m_range.begin()); }
	iterator end() const { return std::move(m_range.end()); }
	void clear() { m_contents = contents_t(1, Node('\0')); }

	size_t size() const { return m_contents[0].subtree_size; }

private:
	struct Node
	{
		Node(const char character);

		char character;
		bool has_child;
		bool is_terminal;
		size_t next_sibling_index;
		size_t subtree_size;
	};

	using path_t = std::list<size_t>;
	using contents_t = std::vector<Node>;

	size_t get_or_create_sibling(const char c, size_t offset, path_t& path);
	size_t find_sibling(const char c, size_t offset) const;

	contents_t m_contents;
	Range m_range;
};

class CompactTrie : public BasicCompactTrie
{
public:
	bool add_string(const std::string& string);
};

#endif // __COMPACT_TRIE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
