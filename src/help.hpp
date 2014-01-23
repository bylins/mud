// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

#include "conf.h"
#include <string>
#include <map>
#include <vector>
#include <array>
#include <boost/bind.hpp>

#include "sysdep.h"
#include "interpreter.h"
#include "char_player.hpp"

/// STATIC справка:
/// 	вся обычная справка
/// 	активаторы сетов (старые и новые)
/// 	груп-зоны
/// DYNAMIC справка:
/// 	клан-сайты
/// 	справка по дропу сетов
namespace HelpSystem
{

extern bool need_update;
enum Flags { STATIC, DYNAMIC, TOTAL_NUM };

// добавление статической справки, которая обновляется максимально редко
void add_static(const std::string &key, const std::string &entry,
	int min_level = 0, bool no_immlog = false);
// в динамической справке все с включенным no_immlog и 0 min_level
void add_dynamic(const std::string &key, const std::string &entry);
// добавление сетов, идет в DYNAMIC массив с включенным sets_drop_page
void add_sets_drop(const std::string key_str, const std::string entry_str);
// лоад/релоад конкретного массива справки
void reload(HelpSystem::Flags sort_flag);
// лоад/релоад всей справки
void reload_all();
// проверка раз в минуту нужно ли обновить динамическую справку (клан-сайты, групп-зоны)
void check_update_dynamic();

} // namespace HelpSystem

namespace PrintActivators
{

// суммарные активы для одной профы
struct clss_activ_node
{
	clss_activ_node() { total_affects = clear_flags; };

	// аффекты
	FLAG_DATA total_affects;
	// свойства
	std::vector<obj_affected_type> affected;
	// скилы
	std::map<int, int> skills;
};

std::string print_skill(const std::pair<int, int> &skill, bool activ);
std::string print_skills(const std::map<int, int> &skills, bool activ, bool header = true);
void sum_skills(std::map<int, int> &target, const std::pair<int, int> &add);
void sum_skills(std::map<int, int> &target, const std::map<int, int> &add);

/// l - список <obj_affected_type> куда добавляем,
/// r - список того же, который добавляем в l
template <class T, class N>
void sum_apply(T &l, const N &r)
{
	for (auto ri = r.begin(); ri != r.end(); ++ri)
	{
		if (ri->modifier == 0)
		{
			continue;
		}
		auto li = std::find_if(l.begin(), l.end(),
			boost::bind(&obj_affected_type::location, _1) == ri->location);
		if (li != l.end())
		{
			li->modifier += ri->modifier;
		}
		else
		{
			l.push_back(*ri);
		}
	}
}

template <class T>
void add_pair(std::map<int, T> &target, const std::pair<int, T> &add)
{
	if (add.first > 0)
	{
		auto i = target.find(add.first);
		if (i != target.end())
		{
			i->second += add.second;
		}
		else
		{
			target[add.first] = add.second;
		}
	}
}

template <class T>
void add_map(std::map<int, T> &target, const std::map<int, T> &add)
{
	for (auto i = add.begin(), iend = add.end(); i != iend; ++i)
	{
		if (i->first > 0)
		{
			auto ii = target.find(i->first);
			if (ii != target.end())
			{
				ii->second += i->second;
			}
			else
			{
				target[i->first] = i->second;
			}
		}
	}
}

} // namespace PrintActivators

ACMD(do_help);

#endif // HELP_HPP_INCLUDED
