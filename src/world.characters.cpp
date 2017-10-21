#include "world.characters.hpp"

#include "dg_db_scripts.hpp"
#include "mobact.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "utils.h"
#include "dg_scripts.h"

#include <iostream>

Characters character_list;	// global container of chars

void Characters::push_front(const CHAR_DATA::shared_ptr& character)
{
	m_list.push_front(character);
	m_object_raw_ptr_to_object_ptr[character.get()] = m_list.begin();

	if (m_purge_set.end() != m_purge_set.find(character.get()))
	{
		const size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		snprintf(buffer, BUFFER_SIZE, "Restoring purged character %s at address %p. Most likely something will get broken soon.",
			character->get_name().c_str(), character.get());
		mudlog(buffer, LGH, LVL_IMPL, SYSLOG, TRUE);
	}
}

void Characters::foreach_on_copy(const foreach_f function) const
{
	const list_t list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

void Characters::remove(CHAR_DATA* character)
{
	const auto index_i = m_object_raw_ptr_to_object_ptr.find(character);
	if (index_i == m_object_raw_ptr_to_object_ptr.end())
	{
		const size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		snprintf(buffer, BUFFER_SIZE, "Character at address %p requested to remove not found in the world.", character);
		mudlog(buffer, LGH, LVL_IMPL, SYSLOG, TRUE);

		return;
	}

	m_purge_list.push_back(*index_i->second);
	m_purge_set.insert(index_i->second->get());
	m_list.erase(index_i->second);
	m_object_raw_ptr_to_object_ptr.erase(index_i);
	character->set_purged();
}

void Characters::purge()
{
	m_purge_set.clear();
	for (const auto& character : m_purge_list)
	{
		if (GET_MOB_RNUM(character) > -1)
		{
			mob_index[GET_MOB_RNUM(character)].number--;
		}

		if (IS_NPC(character))
		{
			clearMemory(character.get());
		}

		free_script(SCRIPT(character));	// см. выше
		SCRIPT(character) = NULL;

		if (SCRIPT_MEM(character))
		{
			extract_script_mem(SCRIPT_MEM(character));
			SCRIPT_MEM(character) = NULL;
		}

		MOB_FLAGS(character).set(MOB_FREE);
	}

	m_purge_list.clear();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
