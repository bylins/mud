// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "mobmax.hpp"

#include "char.hpp"
#include "logger.hpp"
#include "utils.h"
#include "db.h"

#include <map>

std::array<int, MAX_MOB_LEVEL / 11 + 1> animals_levels = { { 0 } };
namespace
{

// Минимальное количество мобов одного уровня в списке замакса по левелам
const int MIN_MOB_IN_MOBKILL = 2;
// Максимальное количество мобов одного уровня в списке замакса по левелам
const int MAX_MOB_IN_MOBKILL = 100;
// Во сколько раз надо убить мобов меньше, чем их есть в мире, чтобы начать размаксивать
const int MOBKILL_KOEFF = 3;
// кол-во мобов каждого уровня
std::array<int, MAX_MOB_LEVEL + 1> num_levels = { {0} };

// мап соответствий внумов и левелов (для быстрого чтения плеер-файла)
typedef std::map<int/* внум моба */, int/* левел моба */> VnumToLevelType;
VnumToLevelType vnum_to_level;

} // namespace

int get_max_kills(const int level)
{
	return num_levels[level];
}

void MobMax::get_stats(mobmax_stats_t& result) const
{
	mob_rnum r_num;
	result.clear();
	for (const auto& item : mobmax_)
	{
		if ((r_num = real_mobile(item.vnum)) < 0)
		{
			log("SYSERR: unknown rnum mob in mombax");
			return;
		}		
		const auto level = item.level;
		if (result.find(level) == result.end())
		{
			result[level] = 0;
		}
		result[level] += MAX(0, item.count - mob_proto[r_num].mob_specials.MaxFactor); //Уберем количество мобов c экспой без замакса
	}
}

// * Иним массив кол-ва мобов каждого левела и мап соответствий внумов и левелов.
void MobMax::init()
{std::array<int, MAX_MOB_LEVEL + 1> num_animals_levels = { {0} };
	for (int i = 0; i <= top_of_mobt; ++i)
	{
		int level = GET_LEVEL(mob_proto + i);
		if (level > MAX_MOB_LEVEL)
			log("Warning! Mob >MAXIMUN lev!");
		else if (level < 0)
			log("Warning! Mob <0 lev!");
		else
		{
			++num_levels[level];
			if (GET_RACE(mob_proto + i) == NPC_RACE_ANIMAL)
				++num_animals_levels[level];
			vnum_to_level[mob_index[i].vnum] = level;
		}
	}

	for (int i = 0; i <= MAX_MOB_LEVEL; ++i)
	{
		log("Mob lev %d. Num of mobs %d", i, num_levels[i]);
		log("Mob animals lev %d. Num of animals mobs %d", i, num_animals_levels[i]);
		num_levels[i] = num_levels[i] / MOBKILL_KOEFF;
		if (num_levels[i] < MIN_MOB_IN_MOBKILL)
			num_levels[i] = MIN_MOB_IN_MOBKILL;
		if (num_levels[i] > MAX_MOB_IN_MOBKILL)
			num_levels[i] = MAX_MOB_IN_MOBKILL;
		animals_levels[i/11] += num_animals_levels[i]; //составим список количества животных по уровню для освежевки
		log("Mob lev %d. Max in MobKill file %d", i, num_levels[i]);

	}
}

// * Возвращает левел указанного vnum моба или -1, если такого нет.
int MobMax::get_level_by_vnum(int vnum)
{
	VnumToLevelType::const_iterator it = vnum_to_level.find(vnum);
	if (it != vnum_to_level.end())
		return it->second;
	return -1;
}

/**
* Снятие замакса по мобам уровня level, если мобов данного уровня убито необходимое кол-во.
* Контейнер для замакса нужен с сохранением порядка элементов, т.к. размакс происходит не
* для всех мобов данного уровня, а только начиная с кол-ва, большего необходимого и убирая
* более старые замаксы.
*/
void MobMax::refresh(int level)
{
	int count = 0;
	for (MobMaxType::iterator it = mobmax_.begin(); it != mobmax_.end();/* empty */)
	{
		if (it->level == level)
		{
			if (count > num_levels[level])
				mobmax_.erase(it++);
			else
			{
				++count;
				++it;
			}
		}
		else
			++it;
	}
}

// * Добавление замакса по мобу vnum, левела level. count для случая сета замакса иммом.
void MobMax::add(CHAR_DATA *ch, int vnum, int count, int level)
{
	if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0 || count < 1 || level < 0 || level > MAX_MOB_LEVEL) return;

	MobMaxType::iterator it = std::find_if(mobmax_.begin(), mobmax_.end(),
		[&](const mobmax_data& data)
	{
		return data.vnum == vnum;
	});

	if (it != mobmax_.end())
		it->count += count;
	else
	{
		mobmax_data tmp_data(vnum, count, level);
		mobmax_.push_front(tmp_data);
	}
	refresh(level);
}

// * Версия add без лишних расчетов для инита во время загрузки персонажа.
void MobMax::load(CHAR_DATA *ch, int vnum, int count, int level)
{
	if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0 || count < 1 || level < 0 || level > MAX_MOB_LEVEL) return;

	mobmax_data tmp_data(vnum, count, level);
	mobmax_.push_front(tmp_data);
}

// * Удаление замакса по указанному мобу vnum.
void MobMax::remove(int vnum)
{
	MobMaxType::iterator it = std::find_if(mobmax_.begin(), mobmax_.end(),
		[&](const mobmax_data& data)
	{
		return data.vnum == vnum;
	});

	if (it != mobmax_.end())
		mobmax_.erase(it);
}

// * Возвращает кол-во убитых мобов данного vnum.
int MobMax::get_kill_count(int vnum) const
{
	MobMaxType::const_iterator it = std::find_if(mobmax_.begin(), mobmax_.end(),
		[&](const mobmax_data& data)
	{
		return data.vnum == vnum;
	});

	if (it != mobmax_.end())
		return it->count;
	return 0;
}

// * Сохранение в плеер-файл.
void MobMax::save(FILE *saved) const
{
	fprintf(saved, "Mobs:\n");
	for (MobMaxType::const_reverse_iterator it = mobmax_.rbegin(); it != mobmax_.rend(); ++it)
		fprintf(saved, "%d %d\n", it->vnum, it->count);
	fprintf(saved, "~\n");
}

// * Для очистки всех замаксов при реморте.
void MobMax::clear()
{
	mobmax_.clear();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
