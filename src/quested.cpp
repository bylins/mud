// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "quested.hpp"

#include "chars/character.h"
#include "utils.h"

#include <boost/algorithm/string/predicate.hpp>

#include <sstream>

void smash_tilde(char *str);

// * Добавление выполненного квеста номер/строка данных (128 символов).
void Quested::add(CHAR_DATA *ch, int vnum, char *text)
{
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch))
	{
		smash_tilde(text);
		skip_spaces(&text);
		std::string text_node = *text ? text : "";
		if (text_node.size() > 128)
		{
			text_node = text_node.substr(0, 128);
		}
		quested_[vnum] = text_node;
	}
}

bool Quested::remove(int vnum)
{
	QuestedType::iterator it = quested_.find(vnum);
	if (it != quested_.end())
	{
		quested_.erase(it);
		return true;
	}

	return false;
}

bool Quested::get(int vnum) const
{
	QuestedType::const_iterator it = quested_.find(vnum);
	if (it != quested_.end())
	{
		return true;
	}
	return false;
}

// * Возвращает строку данных для заданного внума квеста или пустую строку.
std::string Quested::get_text(int vnum) const
{
	QuestedType::const_iterator it = quested_.find(vnum);
	if (it != quested_.end())
	{
		return it->second;
	}
	return "";
}

std::string Quested::print() const
{
	std::stringstream text;
	for (QuestedType::const_iterator it = quested_.begin(); it != quested_.end(); ++it)
	{
		text << " " << it->first << " " << it->second << "\r\n";
	}
	return text.str();
}

void Quested::save(FILE *saved) const
{
	for (QuestedType::const_iterator it = quested_.begin(); it != quested_.end(); ++it)
	{
		fprintf(saved, "Qst : %d %s~\n", it->first, it->second.c_str());
	}
}

void Quested::clear()
{
	QuestedType::const_iterator it = quested_.begin();
	while (it != quested_.end())
	{
		if (!boost::starts_with((*it).second, "@"))
		{
			it = quested_.erase(it);
		}
		else
		{
			++it;
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
