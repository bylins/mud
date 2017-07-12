#ifndef __WORLD_CHARACTERS_HPP__
#define __WORLD_CHARACTERS_HPP__

#include "char.hpp"

#include <list>
#include <functional>

class Characters
{
public:
	using foreach_f = std::function<void(const CHAR_DATA::shared_ptr&)>;
	using predicate_f = std::function<bool(const CHAR_DATA::shared_ptr&)>;

	void push_front(CHAR_DATA* character) { m_list.emplace_front(character); }
	void push_front(const CHAR_DATA::shared_ptr& character) { m_list.push_front(character); }

	const auto& get_list() const { return m_list; }

	const auto begin() const { return m_list.begin(); }
	const auto end() const { return m_list.end(); }

	void foreach_on_copy(const foreach_f function) const;
	bool erase_if(const predicate_f predicate);

private:
	using list_t = std::list<CHAR_DATA::shared_ptr>;

	list_t m_list;
};

extern Characters character_list;

#endif // __WORLD_CHARACTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
