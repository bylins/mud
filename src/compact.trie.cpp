#include "compact.trie.hpp"

bool CompactTrie::add_string(const std::string& string)
{
	const auto original_size = m_contents.size();

	if (0 == string.size())
	{
		return false;
	}

	size_t offset = get_or_create_sibling(string[0], 0);

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

		offset = get_or_create_sibling(string[i], offset);
	}

	m_contents[offset].is_terminal = true;

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

size_t CompactTrie::get_or_create_sibling(const char c, size_t offset)
{
	if (0 == m_contents.size())
	{
		offset = 0;
		m_contents.push_back(Node(c));
	}

	while (c != m_contents[offset].character)
	{
		if (NO_INDEX == m_contents[offset].sibling)
		{
			m_contents[offset].sibling = m_contents.size();
			offset = m_contents.size();
			m_contents.push_back(Node(c));
		}
		else
		{
			offset = m_contents[offset].sibling;
		}
	}

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
		offset = m_contents[offset].sibling;
	}

	return offset;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
