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

	class CL_RNumChangeObserver : public CharacterRNum_ChangeObserver
	{
	public:
		CL_RNumChangeObserver(Characters& cl);

		virtual void notify(ProtectedCharacterData& character, const mob_rnum old_rnum) override;

	private:
		Characters& m_parent;
	};

	Characters();
	~Characters();
	Characters(const Characters&) = delete;
	Characters& operator=(const Characters&) = delete;

	void push_front(const CHAR_DATA::shared_ptr& character);
	void push_front(CHAR_DATA* character) { push_front(CHAR_DATA::shared_ptr(character)); }

	const auto& get_list() const { return m_list; }
	const auto get_character_by_address(const CHAR_DATA* character) const;
	void get_mobs_by_rnum(const mob_rnum rnum, list_t& result);

	const auto begin() const { return m_list.begin(); }
	const auto end() const { return m_list.end(); }

	void foreach_on_copy(const foreach_f function) const;
	void foreach_on_filtered_copy(const foreach_f function, const predicate_f predicate) const;

	void remove(CHAR_DATA* character);
	void remove(const CHAR_DATA::shared_ptr& character) { remove(character.get()); }

	void purge();

	bool has(const CHAR_DATA* character) const;

private:
	using character_raw_ptr_to_character_ptr_t = std::unordered_map<const void*, list_t::iterator>;
	using set_t = std::unordered_set<const CHAR_DATA*>;
	using rnum_to_characters_set_t = std::unordered_map<mob_rnum, set_t>;

	list_t m_list;
	character_raw_ptr_to_character_ptr_t m_character_raw_ptr_to_character_ptr;
	rnum_to_characters_set_t m_rnum_to_characters_set;
	CharacterRNum_ChangeObserver::shared_ptr m_rnum_change_observer;
	list_t m_purge_list;
	set_t m_purge_set;
};

inline const auto Characters::get_character_by_address(const CHAR_DATA* character) const
{
	const auto i = m_character_raw_ptr_to_character_ptr.find(character);
	return i != m_character_raw_ptr_to_character_ptr.end() ? *i->second : list_t::value_type();
}

extern Characters& character_list;

#endif // __WORLD_CHARACTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
