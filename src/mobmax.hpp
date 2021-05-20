// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MOBMAX_HPP_INCLUDED
#define MOBMAX_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#include <list>
#include <map>

int get_max_kills(const int level);

class MobMax
{
public:
	using mobmax_stats_t = std::map<int, int>;	///< maps level to count

	void get_stats(mobmax_stats_t& result) const;

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

	struct mobmax_data
	{
		mobmax_data(int in_vnum, int in_count, int in_level)
			: vnum(in_vnum), count(in_count), level(in_level)
		{};
		// внум моба
		int vnum;
		// кол-во мобов
		int count;
		// их уровень
		int level;
	};

	typedef std::list<mobmax_data> MobMaxType;
	MobMaxType mobmax_;
};

#endif // MOBMAX_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
