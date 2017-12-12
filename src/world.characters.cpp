#include "world.characters.hpp"

#include "dg_db_scripts.hpp"
#include "mobact.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "utils.h"
#include "dg_scripts.h"
#include "debug.utils.hpp"

#include <iostream>

Characters character_list;	// global container of chars

void Characters::push_front(const CHAR_DATA::shared_ptr& character)
{
	std::stringstream ss;
	{
		StreamFlagsHolder holder(ss);
		ss << "Adding character at address 0x" << std::hex << character << ".";
	}
	if (IS_NPC(character))
	{
		ss << " VNUM: " << GET_MOB_VNUM(character) << "; Name: '" << character->get_name() << "'";
	}
	else
	{
		ss << " Player: " << character->get_name();
	}
	debug::log_queue("characters").push(ss.str());

	m_list.push_front(character);
	m_object_raw_ptr_to_object_ptr[character.get()] = m_list.begin();
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

	std::stringstream ss;
	{
		StreamFlagsHolder flags_holder(ss);
		ss << "Removing character at address 0x" << std::hex << character << ".";
	}
	if (IS_NPC(character))
	{
		ss << " VNUM: " << GET_MOB_VNUM(character) << "; Name: '" << character->get_name() <<"'";
	}
	else
	{
		ss << " Player: " << character->get_name();
	}
	debug::log_queue("characters").push(ss.str());

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

bool Characters::has(const CHAR_DATA* character) const
{
	return m_object_raw_ptr_to_object_ptr.find(character) != m_object_raw_ptr_to_object_ptr.end()
		|| m_purge_set.find(character) != m_purge_set.end();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
