#include "compact.trie.hpp"

#include <sstream>
#include <algorithm>

bool CompactTrie::add_string(const std::string& string)
{
	const auto original_size = m_contents.size();

	if (0 == string.size())
	{
		return false;
	}

	path_t path;
	size_t offset = get_or_create_sibling(string[0], 0, path);

	for (size_t i = 1; i < string.size(); ++i)
	{
		if (!m_contents[offset].has_child)
		{
			if (1 + offset != m_contents.size())	// not a lexical order
			{
				m_contents.resize(original_size, Node('\0'));
				return false;
			}

			m_contents[offset++].has_child = true;
			m_contents.push_back(Node(string[i]));
		}
		else
		{
			++offset;
		}

		offset = get_or_create_sibling(string[i], offset, path);
	}
	path.push_back(offset);

	m_contents[offset].is_terminal = true;
	
	for (const auto index : path)
	{
		++m_contents[index].subtree_size;
	}
	++m_size;

	return true;
}

bool CompactTrie::has_string(const std::string& string) const
{
	if (0 == string.size())
	{
		return false;
	}

	size_t pos = 0;
	size_t offset = find_sibling(string[pos++], 0);

	while (pos < string.size()
		&& NO_INDEX != offset
		&& m_contents[offset].has_child)
	{
		offset = find_sibling(string[pos++], ++offset);
	}

	if (pos != string.size()	// not all characters have been processed
		|| NO_INDEX == offset	// sibling for the last character has not been found
		|| !m_contents[offset].is_terminal)	// found node is not terminal
	{
		return false;
	}

	return true;
}

CompactTrie::Node::Node(const char character):
	character(character),
	has_child(false),
	is_terminal(false),
	next_sibling_index(NO_INDEX), subtree_size(0)
{
}

size_t CompactTrie::get_or_create_sibling(const char c, size_t offset, path_t& path)
{
	if (0 == m_contents.size())
	{
		offset = 0;
		m_contents.push_back(Node(c));
	}

	while (c != m_contents[offset].character)
	{
		if (NO_INDEX == m_contents[offset].next_sibling_index)
		{
			m_contents[offset].next_sibling_index = m_contents.size();
			offset = m_contents.size();
			m_contents.push_back(Node(c));
		}
		else
		{
			offset = m_contents[offset].next_sibling_index;
		}
	}

	path.push_back(offset);
	return offset;
}

size_t CompactTrie::find_sibling(const char c, size_t offset) const
{
	if (offset >= m_contents.size())
	{
		return NO_INDEX;
	}

	while (NO_INDEX != offset
		&& c != m_contents[offset].character)
	{
		offset = m_contents[offset].next_sibling_index;
	}

	return offset;
}

const CompactTrie::Range& CompactTrie::Range::DFS_Iterator::operator*() const
{
	if (!m_current_element)
	{
		build_current();
	}

	return *m_current_element;
}

CompactTrie::Range::DFS_Iterator::DFS_Iterator(const Range* range) :
	m_range(range),
	m_current(range->m_root)
{
	go_to_first_leaf();
}

CompactTrie::Range::DFS_Iterator::DFS_Iterator(const Range* range, const size_t current):
	m_range(range),
	m_current(current)
{
	go_to_first_leaf();
}

bool CompactTrie::Range::DFS_Iterator::operator==(const DFS_Iterator& right) const
{
	return (NO_INDEX == m_current && NO_INDEX == right.m_current)
		|| (*m_range == *right.m_range && m_current == right.m_current);
}

CompactTrie::Range::DFS_Iterator& CompactTrie::Range::DFS_Iterator::next()
{
	if (NO_INDEX == m_current)	// end of range
	{
		return *this;
	}

	m_current_element.reset();

	const CompactTrie& trie = *m_range->m_trie;
	bool end_of_whole_trie = false;
	do
	{
		if (!go_to_child()
			&& !go_to_sibling())
		{
			bool went_to_sibling = false;
			while (go_to_parent()
				&& m_range->m_root != m_current
				&& !(went_to_sibling = go_to_sibling()));
			if (!went_to_sibling)
			{
				end_of_whole_trie = true;
			}
		}
	} while (m_current < trie.m_contents.size()
		&& !trie.m_contents[m_current].is_terminal
		&& m_range->m_root != m_current
		&& !end_of_whole_trie);

	if (m_range->m_root == m_current
		|| end_of_whole_trie)
	{
		m_current = NO_INDEX;
	}

	return *this;
}

void CompactTrie::Range::DFS_Iterator::go_to_first_leaf()
{
	const CompactTrie& trie = *m_range->m_trie;
	
	if (NO_INDEX == m_current)
	{
		m_current = 0;
	}
	else if (!go_to_child())	// root without subtree
	{
		m_current = NO_INDEX;
	}

	if (m_current >= trie.m_contents.size())	// end of trie
	{
		m_current = NO_INDEX;
		return;
	}

	while (!trie.m_contents[m_current].is_terminal
		&& m_current < trie.m_contents.size())
	{
		if (!trie.m_contents[m_current].has_child)
		{
			throw std::logic_error("Non terminal node does not have children.");
		}
		go_to_child();
	}
}

bool CompactTrie::Range::DFS_Iterator::go_to_child()
{
	const CompactTrie& trie = *m_range->m_trie;

	if (trie.m_contents[m_current].has_child)
	{
		m_path.push_back(m_current);
		++m_current;
		return true;
	}

	return false;
}

bool CompactTrie::Range::DFS_Iterator::go_to_sibling()
{
	const CompactTrie& trie = *m_range->m_trie;

	if (NO_INDEX != trie.m_contents[m_current].next_sibling_index)
	{
		m_current = m_range->m_trie->m_contents[m_current].next_sibling_index;
		return true;
	}

	return false;
}

bool CompactTrie::Range::DFS_Iterator::go_to_parent()
{
	const CompactTrie& trie = *m_range->m_trie;

	if (!m_path.empty())
	{
		m_current = m_path.back();
		m_path.pop_back();
		return true;
	}

	return false;
}

void CompactTrie::Range::DFS_Iterator::build_current() const
{
	std::stringstream ss;
	if (NO_INDEX != m_current)
	{
		ss << m_range->m_prefix;
		std::for_each(m_path.begin(), m_path.end(), [&](const auto e) { ss << m_range->m_trie->m_contents[e].character; });
		ss << m_range->m_trie->m_contents[m_current].character;
	}

	m_current_element.reset(new Range(m_range->m_trie, m_current, ss.str()));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :