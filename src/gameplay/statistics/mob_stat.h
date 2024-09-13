// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MOB_STAT_HPP_INCLUDED
#define MOB_STAT_HPP_INCLUDED

#include "engine/core/conf.h"
#include <unordered_map>
#include <array>
#include <list>
#include <string>
#include "gameplay/classes/classes_constants.h"

#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

/// Статистика по убийствам мобов/мобами с делением на размер группы и месяцы
namespace mob_stat {

/// макс. кол-во участников в группе учитываемое в статистике
const int kMaxGroupSize = 20;
/// период сохранения mob_stat.xml (минуты)
const int kSavePeriod = 27;
/// 0 - убиства мобом игроков, 1..MAX_GROUP_SIZE - убиства моба игроками
using KillStat = std::array<int, kMaxGroupSize + 1>;

struct MobMonthKillStat {
	MobMonthKillStat() : month(0), year(0) {
		kills.fill(0);
	};
	int month;			// месяц (1..12)
	int year;			// год (хххх)
	KillStat kills{};	// стата по убийствам за данный месяц
};

struct MobKillStat {
	static const time_t default_date;

	MobKillStat() : date(0) {}
	explicit MobKillStat(const time_t d) : date(d) {}

	time_t date;
	std::list<MobMonthKillStat> stats;
};

using MobStatRegister = std::unordered_map<MobVnum, MobKillStat>;

extern MobStatRegister mob_stat_register;

/// лоад mob_stat.xml
void Load();
/// сейв mob_stat.xml
void Save();
/// вывод инфы имму по show stats
void ShowStats(CharData *ch);
/// добавление статы по мобу
/// \param members если = 0 - см. KillStatType
void AddMob(CharData *mob, int members);
// когда последний раз убили моба
void GetLastMobKill(CharData *mob, std::string &result);
time_t GetMobKilllastTime(MobVnum vnum);
/// печать статистики имму по конкретной зоне (show mobstat ZoneVnum)
void ShowZoneMobKillsStat(CharData *ch, ZoneVnum zone_vnum, int months);
/// очистка статы по всем мобам из зоны ZoneVnum
void ClearZoneStat(ZoneVnum zone_vnum);
/// выборка моб-статистики за последние months месяцев (0 = все)
MobMonthKillStat SumStat(const std::list<MobMonthKillStat> &mob_stat, int months);
// имя моба по его vnum
std::string PrintMobName(int mob_vnum, unsigned int len);

} // namespace mob_stat

namespace char_stat {

/// мобов убито - для 'статистика'
extern int mobs_killed;
/// игроков убито - для 'статистика'
extern int players_killed;
/// добавление экспы по профам - для 'статистика'
void AddClassExp(ECharClass class_id, int exp);
/// распечатка экспы по профам - для 'статистика'
void PrintClassesExpStat(std::ostringstream &out);
/// распечатка перед ребутом набранной статистики экспы по профам
void LogClassesExpStat();

} // namespace char_stat

#endif // MOB_STAT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
