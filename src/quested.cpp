// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include "quested.hpp"
#include "utils.h"
#include "char.hpp"

void Quested::add(CHAR_DATA *ch, int vnum)
{
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch))
		quested_.insert(vnum);
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
		return true;
	return false;
}

std::string Quested::print() const
{
	std::stringstream text;
	for (QuestedType::const_iterator it = quested_.begin(); it != quested_.end(); ++it)
		text << " " << *it;
	return text.str();
}

void Quested::save(FILE *saved) const
{
	for (QuestedType::const_iterator it = quested_.begin(); it != quested_.end(); ++it)
		fprintf(saved, "Qst : %d\n", *it);
}

void Quested::clear()
{
	quested_.clear();
}
