// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef QUESTED_HPP_INCLUDED
#define QUESTED_HPP_INCLUDED

#include <map>
#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

class Quested
{
public:
	void add(CHAR_DATA *ch, int vnum, char *text);
	bool remove(int vnum);
	void clear();

	bool get(int vnum) const;
	std::string get_text(int vnum) const;
	std::string print() const;
	void save(FILE *saved) const;

private:
	// выполненные квесты
	typedef std::map<int /* внум */, std::string /* текст данных конкретного квеста */> QuestedType;

	QuestedType quested_;
};

#endif // QUESTED_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
