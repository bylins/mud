#ifndef __WORLD_CHARACTERS_HPP__
#define __WORLD_CHARACTERS_HPP__

#include "engine/entities/char_data.h"

#include <list>
#include <functional>
#include <unordered_map>
#include <unordered_set>

class Characters {
 public:
	using foreach_f = std::function<void(const CharData::shared_ptr &)>;
	using predicate_f = std::function<bool(const CharData::shared_ptr &)>;
	using list_t = std::list<CharData::shared_ptr>;

	using const_iterator = list_t::const_iterator;

	class CL_RNumChangeObserver : public CharacterRNum_ChangeObserver {
	 public:
		CL_RNumChangeObserver(Characters &cl);

		virtual void notify(ProtectedCharData &character, const MobRnum old_rnum) override;

	 private:
		Characters &m_parent;
	};

	Characters();
	~Characters();
	Characters(const Characters &) = delete;
	Characters &operator=(const Characters &) = delete;

	void push_front(const CharData::shared_ptr &character);
	void push_front(CharData *character) { push_front(CharData::shared_ptr(character)); }

	const auto &get_list() const { return m_list; }
	const auto get_character_by_address(const CharData *character) const;
	void get_mobs_by_vnum(const MobVnum vnum, list_t &result);

	const auto begin() const { return m_list.begin(); }
	const auto end() const { return m_list.end(); }

	void foreach_on_copy(const foreach_f function) const;
	void foreach_on_filtered_copy(const foreach_f function, const predicate_f predicate) const;

	void remove(CharData *character);
	void remove(const CharData::shared_ptr &character) { remove(character.get()); }

	void purge();

	bool has(const CharData *character) const;

 private:
	using character_raw_ptr_to_character_ptr_t = std::unordered_map<const void *, list_t::iterator>;
	using set_t = std::unordered_set<const CharData *>;
	using vnum_to_characters_set_t = std::unordered_map<MobVnum, set_t>;

	list_t m_list;
	character_raw_ptr_to_character_ptr_t m_character_raw_ptr_to_character_ptr;
	vnum_to_characters_set_t m_vnum_to_characters_set;
	CharacterRNum_ChangeObserver::shared_ptr m_rnum_change_observer;
	list_t m_purge_list;
	set_t m_purge_set;
};

inline const auto Characters::get_character_by_address(const CharData *character) const {
	const auto i = m_character_raw_ptr_to_character_ptr.find(character);
	return i != m_character_raw_ptr_to_character_ptr.end() ? *i->second : list_t::value_type();
}

extern Characters &character_list;

#endif // __WORLD_CHARACTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
