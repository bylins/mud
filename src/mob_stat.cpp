// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <sstream>
#include <iomanip>
#include <string>
#include "pugixml.hpp"

#include "mob_stat.hpp"
#include "utils.h"
#include "db.h"
#include "parse.hpp"
#include "char.hpp"

namespace mob_stat
{

const char *MOB_STAT_FILE = LIB_PLRSTUFF"mob_stat.xml";
const char *MOB_STAT_FILE_NEW = LIB_PLRSTUFF"mob_stat_new.xml";
/// за сколько месяцев хранится статистика (+ текущий месяц)
const int HISTORY_SIZE = 6;
/// выборка кол-ва мобов для show stats при старте мада <months, mob-count>
std::map<int, int> count_stats;
/// список мобов по внуму и месяцам
std::unordered_map<int, std::list<mob_node>> mob_list;

/// month, year
std::pair<int, int> get_date()
{
	time_t curr_time = time(0);
	struct tm *tmp_tm = localtime(&curr_time);
	return std::make_pair(tmp_tm->tm_mon + 1, tmp_tm->tm_year + 1900);
}

void convert()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOB_STAT_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("mob_list");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<mob_list> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	for (pugi::xml_node xml_mob = node_list.child("mob"); xml_mob; xml_mob = xml_mob.next_sibling("mob"))
	{
		int mob_vnum = Parse::attr_int(xml_mob, "vnum");
		if (real_mobile(mob_vnum) < 0)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...bad mob attributes (vnum=%d)", mob_vnum);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		struct mob_node tmp_mob;
		tmp_mob.month = 11;
		tmp_mob.year = 2013;

		for (int k = 1; k <= MAX_GROUP_SIZE; ++k)
		{
			snprintf(buf, sizeof(buf), "n%d", k);
			pugi::xml_attribute attr = xml_mob.attribute(buf);
			if (attr && attr.as_int() > 0)
			{
				tmp_mob.kills.at(k) = attr.as_int();
			}
		}

		std::list<mob_node> tmp_time;
		tmp_time.push_back(tmp_mob);
		mob_list.insert(std::make_pair(mob_vnum, tmp_time));
	}
	rename(MOB_STAT_FILE, LIB_PLRSTUFF"mob_stat.old");
	save();
}

void load()
{
	convert();
	mob_list.clear();

	char buf_[MAX_INPUT_LENGTH];

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOB_STAT_FILE_NEW);
	if (!result)
	{
		snprintf(buf_, sizeof(buf_), "...%s", result.description());
		mudlog(buf_, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("mob_list");
    if (!node_list)
    {
		snprintf(buf_, sizeof(buf_), "...<mob_list> read fail");
		mudlog(buf_, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	for (pugi::xml_node xml_mob = node_list.child("mob"); xml_mob;
		xml_mob = xml_mob.next_sibling("mob"))
	{
		const int mob_vnum = Parse::attr_int(xml_mob, "vnum");
		if (real_mobile(mob_vnum) < 0)
		{
			snprintf(buf_, sizeof(buf_),
				"...bad mob attributes (vnum=%d)", mob_vnum);
			mudlog(buf_, CMP, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}
		// инит статы конкретного моба по месяцам
		std::list<mob_node> tmp_time;
		for (pugi::xml_node xml_time = xml_mob.child("t"); xml_time;
			xml_time = xml_time.next_sibling("t"))
		{
			struct mob_node tmp_mob;
			tmp_mob.month = Parse::attr_int(xml_time, "m");
			tmp_mob.year = Parse::attr_int(xml_time, "y");
			if (tmp_mob.month <= 0 || tmp_mob.month > 12 || tmp_mob.year <= 0)
			{
				snprintf(buf_, sizeof(buf_),
					"...bad mob attributes (month=%d, year=%d)",
					tmp_mob.month, tmp_mob.year);
				mudlog(buf_, CMP, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}
			auto date = get_date();
			if ((date.first + date.second * 12)
				- (tmp_mob.month + tmp_mob.year * 12) > HISTORY_SIZE + 1)
			{
				continue;
			}
			count_stats[tmp_mob.month] += 1;
			for (int k = 0; k <= MAX_GROUP_SIZE; ++k)
			{
				snprintf(buf_, sizeof(buf_), "n%d", k);
				pugi::xml_attribute attr = xml_time.attribute(buf_);
				if (attr && attr.as_int() > 0)
				{
					tmp_mob.kills.at(k) = attr.as_int();
				}
			}
			tmp_time.push_back(tmp_mob);
		}
		mob_list.insert(std::make_pair(mob_vnum, tmp_time));
	}
}

void save()
{
	pugi::xml_document doc;
	doc.append_child().set_name("mob_list");
	pugi::xml_node xml_mob_list = doc.child("mob_list");
	char buf_[MAX_INPUT_LENGTH];

	for (auto i = mob_list.cbegin(), iend = mob_list.cend(); i != iend; ++i)
	{
		pugi::xml_node mob_node = xml_mob_list.append_child();
		mob_node.set_name("mob");
		mob_node.append_attribute("vnum") = i->first;
		// стата по месяцам
		for (auto k = i->second.cbegin(), kend = i->second.cend(); k != kend; ++k)
		{
			pugi::xml_node time_node = mob_node.append_child();
			time_node.set_name("t");
			time_node.append_attribute("m") = k->month;
			time_node.append_attribute("y") = k->year;
			for (int g = 0; g <= MAX_GROUP_SIZE; ++g)
			{
				if (k->kills.at(g) > 0)
				{
					snprintf(buf_, sizeof(buf_), "n%d", g);
					time_node.append_attribute(buf_) = k->kills.at(g);
				}
			}
		}
	}
	doc.save_file(MOB_STAT_FILE_NEW);
}

void clear_zone(int zone_vnum)
{
	for (auto i = mob_list.begin(), iend = mob_list.end(); i != iend; /**/)
	{
		if (i->first/100 == zone_vnum)
		{
			mob_list.erase(i++);
		}
		else
		{
			++i;
		}
	}
	save();
}

void show_stats(CHAR_DATA *ch)
{
	std::stringstream out;
	out << "  Всего мобов в статистике убийств: " << mob_list.size() << "\r\n"
		<< "  По месяцам:";
	for (auto i = count_stats.begin(); i != count_stats.end(); ++i)
	{
		out << " " << std::setw(2) << std::setfill('0') << i->first << ":" << i->second;
	}
	out << "\r\n";
	send_to_char(out.str(), ch);
}

/// !node_list.empty()
void update_mob_node(std::list<mob_node> &node_list, int members)
{
	auto date = get_date();
	auto k = node_list.rbegin();
	const int months = k->month + k->year * 12;
	// сравнение номера месяца с переключением на следующий
	if (months == date.first + date.second * 12)
	{
		k->kills.at(members) += 1;
	}
	else
	{
		struct mob_node node;
		node.month = date.first;
		node.year = date.second;
		node.kills.at(members) += 1;
		node_list.push_back(node);
		// проверка на переполнение кол-ва месяцев
		if (node_list.size() > HISTORY_SIZE + 1)
		{
			node_list.erase(node_list.begin());
		}
	}
}

void add_mob(CHAR_DATA *mob, int members)
{
	if (members < 0 || members > MAX_GROUP_SIZE)
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_),
			"SYSERROR: mob_vnum=%d, members=%d (%s:%d)",
			GET_MOB_VNUM(mob), members, __FILE__, __LINE__);
		mudlog(buf_, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	auto i = mob_list.find(GET_MOB_VNUM(mob));
	if (i != mob_list.end() && !i->second.empty())
	{
		update_mob_node(i->second, members);
	}
	else
	{
		struct mob_node node;
		auto date = get_date();
		node.month = date.first;
		node.year = date.second;
		node.kills.at(members) += 1;

		std::list<mob_node> list_node;
		list_node.push_back(node);

		mob_list.insert(std::make_pair(GET_MOB_VNUM(mob), list_node));
	}
}

std::string print_mob_name(int mob_vnum)
{
	std::string name = "null";
	const int rnum = real_mobile(mob_vnum);
	if (rnum > 0 && rnum <= top_of_mobt)
	{
		name = mob_proto[rnum].get_name();
	}
	if (name.size() > 20)
	{
		name = name.substr(0, 20);
	}
	return name;
}

mob_node sum_stat(const std::list<mob_node> &mob_stat, int months)
{
	auto date = get_date();
	const int min_month = (date.first + date.second * 12) - months;
	struct mob_node tmp_stat;

	for (auto i = mob_stat.rbegin(), iend = mob_stat.rend(); i != iend; ++i)
	{
		if (months == 0 || min_month < (i->month + i->year * 12))
		{
			for (int k = 0; k <= MAX_GROUP_SIZE; ++k)
			{
				tmp_stat.kills.at(k) += i->kills.at(k);
			}
		}
	}
	return tmp_stat;
}

void show_zone(CHAR_DATA *ch, int zone_vnum, int months)
{
	std::map<int, mob_node> sort_list;
	for (auto i = mob_list.begin(), iend = mob_list.end(); i != iend; ++i)
	{
		if (i->first/100 == zone_vnum)
		{
			mob_node sum = sum_stat(i->second, months);
			sort_list.insert(std::make_pair(i->first, sum));
		}
	}

	std::stringstream out;
	out << "Статистика убийств мобов в зоне номер " << zone_vnum
		<< ", месяцев: " << months << "\r\n"
		"   vnum : имя : pk : группа = убийств (n3=100 моба убили 100 раз втроем)\r\n\r\n";

	for (auto i = sort_list.begin(); i != sort_list.end(); ++i)
	{
		out << i->first << " : " << std::setw(20)
			<< print_mob_name(i->first) << " : "
			<< i->second.kills.at(0) << " :";
		for (int g = 1; g <= MAX_GROUP_SIZE; ++g)
		{
			if (i->second.kills.at(g) > 0)
			{
				out << " n" << g << "=" << i->second.kills.at(g);
			}
		}
		out << "\r\n";
	}

	send_to_char(out.str().c_str(), ch);
}

} // namespace mob_stat
