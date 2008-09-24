// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef QUESTED_HPP_INCLUDED
#define QUESTED_HPP_INCLUDED

#include <set>
#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

class Quested
{
public:
	void add(CHAR_DATA *ch, int vnum);
	bool remove(int vnum);
	bool get(int vnum) const;
	std::string print() const;
	void save(FILE *saved) const;
	void clear();

private:
	// выполненные квесты
	std::set<int/* внум */> quested_;
};

#endif // QUESTED_HPP_INCLUDED
