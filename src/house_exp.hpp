// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HOUSE_EXP_HPP_INCLUDED
#define HOUSE_EXP_HPP_INCLUDED

#include <list>
#include <map>
#include <string>
#include <vector>
#include "conf.h"
#include "sysdep.h"
#include "chars/character.h"

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
	void load(const std::string &abbrev);
	void save(const std::string &abbrev) const;
	void update_total_exp();
	void fulldelete();
private:
	int buffer_exp_;
	long long total_exp_;
	typedef std::list<long long> ExpListType;
	ExpListType list_;
};

// * Список последних пк с участием чаров из данного клана.
class ClanPkLog
{
public:
	ClanPkLog() : need_save(false) {};

	void load(const std::string &abbrev);
	void save(const std::string &abbrev);
	void print(CHAR_DATA *ch) const;
	static void check(CHAR_DATA *ch, CHAR_DATA *victim);

private:
	void add(const std::string &text);

	bool need_save;
	std::list<std::string> pk_log;
};

// * Помесячная история набранной кланом экспы без учета минусов от смертей и прочего.
class ClanExpHistory
{
public:
	void add_exp(long exp);
	void load(const std::string &abbrev);
	void save(const std::string &abbrev) const;
	long long get(int month) const;
	bool need_destroy() const;
	void show(CHAR_DATA *ch) const;
	void fulldelete();

private:
	typedef std::map<std::string /* месяц.год */, long long /* экспа*/> HistoryExpListType;
	HistoryExpListType list_;
	long long calc_exp_history() const;
};

////////////////////////////////////////////////////////////////////////////////
class ClanChestLog
{
	std::list<std::string> chest_log_;
	bool need_save_;

public:
	ClanChestLog() : need_save_(false) {};

	void add(const std::string &text);
	void print(CHAR_DATA *ch, std::string &text) const;
	void save(const std::string &abbrev);
	void load(const std::string &abbrev);
};
////////////////////////////////////////////////////////////////////////////////

#endif // HOUSE_EXP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
