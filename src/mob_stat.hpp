// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MOB_STAT_HPP_INCLUDED
#define MOB_STAT_HPP_INCLUDED

#include "conf.h"
#include <unordered_map>
#include <array>
#include <list>
#include <string>

#include "sysdep.h"
#include "structs.h"

/// Статистика по убийствам мобов/мобами с делением на размер группы и месяцы
namespace mob_stat
{

/// макс. кол-во участников в группе учитываемое в статистике
const int MAX_GROUP_SIZE = 12;
/// период сохранения mob_stat.xml (минуты)
const int SAVE_PERIOD = 27;
/// 0 - убиства мобом игроков, 1..MAX_GROUP_SIZE - убиства моба игроками
typedef std::array<int, MAX_GROUP_SIZE + 1> KillStatType;



struct mob_node
{
	mob_node() : month(0), year(0)
	{
		kills.fill(0);
	};
	// месяц (1..12)
	int month;
	// год (хххх)
	int year;
	// стата по убийствам за данный месяц
	KillStatType kills;
};

struct MobNode
{
	static const time_t DEFAULT_DATE;

	MobNode(): date(0) {}
	MobNode(const time_t d): date(d) {}

	time_t date;
	std::list<mob_node> stats;
};

using mob_list_t = std::unordered_map<int, MobNode>;

extern mob_list_t mob_list;

/// лоад mob_stat.xml
void load();
/// сейв mob_stat.xml
void save();
/// вывод инфы имму по show stats
void show_stats(CHAR_DATA *ch);
/// добавление статы по мобу
/// \param members если = 0 - см. KillStatType
void add_mob(CHAR_DATA *mob, int members);
// когда последний раз убили моба
void last_kill_mob(CHAR_DATA *mob, std::string& result);
int last_time_killed_mob(int vnum);
/// печать статистики имму по конкретной зоне (show mobstat zone_vnum)
void show_zone(CHAR_DATA *ch, int zone_vnum, int months);
/// очистка статы по всем мобам из зоны zone_vnum
void clear_zone(int zone_vnum);
/// выборка моб-статистики за последние months месяцев (0 = все)
mob_node sum_stat(const std::list<mob_node> &mob_stat, int months);

} // namespace mob_stat

namespace char_stat
{

/// мобов убито - для 'статистика'
extern int mkilled;
/// игроков убито - для 'статистика'
extern int pkilled;
/// добавление экспы по профам - для 'статистика'
void add_class_exp(unsigned class_num, int exp);
/// распечатка экспы по профам - для 'статистика'
std::string print_class_exp(CHAR_DATA *ch);
/// распечатка перед ребутом набранной статистики экспы по профам
void log_class_exp();

} // namespace char_stat

#endif // MOB_STAT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
