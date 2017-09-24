#ifndef __WORLD_CHARACTERS_HPP__
#define __WORLD_CHARACTERS_HPP__

#include "char.hpp"

#include <list>
#include <functional>
#include <unordered_map>
#include <unordered_set>

class Characters
{
public:
	using foreach_f = std::function<void(const CHAR_DATA::shared_ptr&)>;
	using predicate_f = std::function<bool(const CHAR_DATA::shared_ptr&)>;
	using list_t = std::list<CHAR_DATA::shared_ptr>;

	using const_iterator = list_t::const_iterator;

	void push_front(const CHAR_DATA::shared_ptr& character);
	void push_front(CHAR_DATA* character) { push_front(CHAR_DATA::shared_ptr(character)); }

	const auto& get_list() const { return m_list; }

	const auto begin() const { return m_list.begin(); }
	const auto end() const { return m_list.end(); }

	void foreach_on_copy(const foreach_f function) const;

	void remove(CHAR_DATA* character);

	void purge();

	bool has(const CHAR_DATA* character) const
	{
		return m_object_raw_ptr_to_object_ptr.find(character) != m_object_raw_ptr_to_object_ptr.end()
			|| m_purge_set.find(character) != m_purge_set.end();
	}

private:
	using object_raw_ptr_to_object_ptr_t = std::unordered_map<const void*, list_t::iterator>;
	using set_t = std::unordered_set<const CHAR_DATA*>;

	list_t m_list;
	object_raw_ptr_to_object_ptr_t m_object_raw_ptr_to_object_ptr;
	list_t m_purge_list;
	set_t m_purge_set;
};

extern Characters character_list;

#endif // __WORLD_CHARACTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
