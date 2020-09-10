#include "world.characters.hpp"

#include "mobact.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "utils.h"
#include "global.objects.hpp"

Characters& character_list = GlobalObjects::characters();	// global container of chars

Characters::CL_RNumChangeObserver::CL_RNumChangeObserver(Characters& cl) : m_parent(cl)
{
}

Characters::~Characters()
{
	log("~Characters()");
}

void Characters::CL_RNumChangeObserver::notify(ProtectedCharacterData& character, const mob_rnum old_rnum)
{
	const auto character_ptr = dynamic_cast<CHAR_DATA*>(&character);
	if (nullptr == character_ptr)
	{
		log("LOGIC ERROR: Character object passed to RNUM change observer "
			"is not an instance of CHAR_DATA class. Old RNUM: %d.",
			old_rnum);

		return;
	}

	m_parent.m_rnum_to_characters_set[old_rnum].erase(character_ptr);
	if (m_parent.m_rnum_to_characters_set[old_rnum].empty())
	{
		m_parent.m_rnum_to_characters_set.erase(old_rnum);
	}

	const auto new_rnum = character.get_rnum();
	if (new_rnum != NOBODY)
	{
		m_parent.m_rnum_to_characters_set[new_rnum].insert(character_ptr);
	}
}

Characters::Characters()
{
	m_rnum_change_observer = std::make_shared<CL_RNumChangeObserver>(*this);
}

void Characters::push_front(const CHAR_DATA::shared_ptr& character)
{
	m_list.push_front(character);
	m_character_raw_ptr_to_character_ptr[character.get()] = m_list.begin();

	const auto rnum = character->get_rnum();
	if (NOBODY != rnum)
	{
		m_rnum_to_characters_set[rnum].insert(character.get());
	}
	character->subscribe_for_rnum_changes(m_rnum_change_observer);

	if (character->purged())
	{
		/*
		* Anton Gorev (2017/10/29): It is possible is character quit the game without
		* disconnecting and then reenter the game. In this case #character of his descriptor will be reused but
		* flag #purged still be set. Technically now flag #purged means only "character_list is going to reset
		* shared pointer to this object." but it doesn't mean that object was removed (even partially).
		*
		* Thus we can safely put character with flag #purged and just clear this flag.
		*/
		character->set_purged(false);
	}
}

void Characters::get_mobs_by_rnum(const mob_rnum rnum, list_t& result)
{
	result.clear();

	const auto i = m_rnum_to_characters_set.find(rnum);
	if (i == m_rnum_to_characters_set.end())
	{
		return;
	}

	for (const auto& character : i->second)
	{
		result.push_back(*m_character_raw_ptr_to_character_ptr[character]);
	}
}

void Characters::foreach_on_copy(const foreach_f function) const
{
	const list_t list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

void Characters::foreach_on_filtered_copy(const foreach_f function, const predicate_f predicate) const
{
	list_t list;
	std::copy_if(get_list().begin(), get_list().end(), std::back_inserter(list), predicate);
	std::for_each(list.begin(), list.end(), function);
}

void Characters::remove(CHAR_DATA* character)
{
	const auto index_i = m_character_raw_ptr_to_character_ptr.find(character);
	if (index_i == m_character_raw_ptr_to_character_ptr.end())
	{
		const size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		snprintf(buffer, BUFFER_SIZE, "Character at address %p requested to remove not found in the world.", character);
		mudlog(buffer, LGH, LVL_IMPL, SYSLOG, TRUE);

		return;
	}

	m_purge_list.push_back(*index_i->second);
	m_purge_set.insert(index_i->second->get());

	character->unsubscribe_from_rnum_changes(m_rnum_change_observer);
	const auto rnum = character->get_rnum();
	if (NOBODY != rnum)
	{
		m_rnum_to_characters_set[rnum].erase(character);
		if (m_rnum_to_characters_set[rnum].empty())
		{
			m_rnum_to_characters_set.erase(rnum);
		}
	}

	m_list.erase(index_i->second);
	m_character_raw_ptr_to_character_ptr.erase(index_i);

	character->set_purged();
}

void Characters::purge()
{
	m_purge_set.clear();
	for (const auto& character : m_purge_list)
	{
		if (IS_NPC(character))
		{
			clearMemory(character.get());
		}

		MOB_FLAGS(character).set(MOB_FREE);
	}

	m_purge_list.clear();
}

bool Characters::has(const CHAR_DATA* character) const
{
	return m_character_raw_ptr_to_character_ptr.find(character) != m_character_raw_ptr_to_character_ptr.end()
		|| m_purge_set.find(character) != m_purge_set.end();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
