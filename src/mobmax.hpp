// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MOBMAX_HPP_INCLUDED
#define MOBMAX_HPP_INCLUDED

#include <map>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

struct mobmax_data
{
	// внум моба
	int vnum;
	// кол-во мобов
	int count;
	// их уровень
	int level;
};

typedef std::list<mobmax_data> MobMaxType;

class MobMax
{
public:
	static void init();
	static int get_level_by_vnum(int vnum);

	int get_kill_count(int vnum) const;
	void add(CHAR_DATA *ch, int vnum, int count, int level);
	void load(CHAR_DATA *ch, int vnum, int count, int level);
	void remove(int vnum);
	void save(FILE *saved) const;
	void clear();

private:
	void refresh(int level);

	MobMaxType mobmax_;
};

#endif // MOBMAX_HPP_INCLUDED
