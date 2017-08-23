#ifndef __WORLD_CHARACTERS_HPP__
#define __WORLD_CHARACTERS_HPP__

#include "char.hpp"

#include <list>
#include <functional>
#include <unordered_map>

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

	bool erase_if(const predicate_f predicate);
	void remove(const CHAR_DATA* character);

private:
	using object_raw_ptr_to_object_ptr_t = std::unordered_map<const void*, list_t::iterator>;

	list_t m_list;
	object_raw_ptr_to_object_ptr_t m_object_raw_ptr_to_object_ptr;
};

extern Characters character_list;

#endif // __WORLD_CHARACTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
