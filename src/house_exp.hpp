// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HOUSE_EXP_HPP_INCLUDED
#define HOUSE_EXP_HPP_INCLUDED

#include <list>
#include "conf.h"
#include "sysdep.h"

void update_clan_exp();
void save_clan_exp();
// период обновления и сохранения экспы (в минутах)
const int CLAN_EXP_UPDATE_PERIOD = 60;

class ClanExp
{
public:
	ClanExp() : buffer_exp_(0), total_exp_(0) {};
	long long get_exp() const;
	void add_chunk();
	void add_temp(int exp);
	void load(std::string abbrev);
	void save(std::string abbrev) const;
	void update_total_exp();
private:
	int buffer_exp_;
	long long total_exp_;
	typedef std::list<long long> ExpListType;
	ExpListType list_;
};

#endif // HOUSE_EXP_HPP_INCLUDED
