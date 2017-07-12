#include "world.characters.hpp"

Characters character_list;	// global container of chars

void Characters::foreach_on_copy(const foreach_f function) const
{
	const list_t list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

bool Characters::erase_if(const predicate_f predicate)
{
	bool result = false;

	auto i = m_list.begin();
	while (i != m_list.end())
	{
		if (predicate(*i))
		{
			i = m_list.erase(i);
			result = true;
		}
		else
		{
			++i;
		}
	}

	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
