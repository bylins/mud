#include "radix.trie.hpp"

void compare(const std::string& string1, size_t& pos1, const std::string& string2, size_t& pos2)
{
	while (pos1 < string1.size()
		&& pos2 < string2.size()
		&& string1[pos1] == string2[pos2])
	{
		++pos1;
		++pos2;
	}
}

bool RadixTrie::add_string(const std::string& string)
{
	if (!m_root)
	{
		m_root = std::make_shared<Node>(string);
	}
	else
	{
		size_t from = 0;
		m_root->add_suffix(string, from);
	}

	return true;
}

bool RadixTrie::has_string(const std::string& string) const
{
	bool result = false;
	
	if (m_root)
	{
		result = m_root->has_suffix(string, 0);
	}

	return result;
}

void RadixTrie::Node::add_suffix(const std::string& string, size_t from)
{
	if (from >= string.size())
	{
		return;
	}

	size_t pos = 0;
	compare(string, from, m_piece, pos);

	if (pos == m_piece.size())
	{
		if (from == string.size())
		{
			m_is_terminal = true;	// node is the suffix we are appending
		}
		else // from < string.size()
		{
			// need to find existing or create new child
			const unsigned char branch = string[from];

			if (m_children[branch])
			{
				m_children[branch]->add_suffix(string, from);
			}
			else
			{
				const auto tail = string.substr(from);
				m_children[branch] = std::make_shared<Node>(tail);
			}
		}
	}
	else // pos < m_piece.size()
	{
		// need to split piece
		const auto old_tail = m_piece.substr(pos);
		m_piece.resize(pos);
		const auto new_child = std::make_shared<Node>(old_tail);
		const auto size = m_children.size();
		new_child->m_children = std::move(m_children);
		m_children.resize(size);
		const unsigned char old_branch = old_tail[0];
		m_children[old_branch] = new_child;
		m_is_terminal = from == string.size();

		if (!m_is_terminal)
		{
			const unsigned char new_branch = string[from];
			const auto new_tail = string.substr(from);
			m_children[new_branch] = std::make_shared<Node>(new_tail);
		}
	}
}

bool RadixTrie::Node::has_suffix(const std::string& string, size_t from) const
{
	size_t pos = 0;
	compare(m_piece, pos, string, from);

	if (m_piece.size() > pos)
	{
		return false;
	}
	else if (from == string.size())
	{
		// found suffix
		return m_is_terminal;
	}
	else // from < string.size()
	{
		const unsigned char branch = string[from];
		if (!m_children[branch])
		{
			return false;
		}

		return m_children[branch]->has_suffix(string, from);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
