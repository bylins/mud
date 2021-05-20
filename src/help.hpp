// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"
#include "chars/char_player.hpp"

#include <string>
#include <map>
#include <vector>
#include <array>

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
	CObjectPrototype::skills_t skills;
};

std::string print_skill(const CObjectPrototype::skills_t::value_type &skill, bool activ);
std::string print_skills(const CObjectPrototype::skills_t &skills, bool activ, bool header = true);
void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype::skills_t::value_type &add);
void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype::skills_t &add);

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
			[&](const auto& obj_aff)
		{
			return obj_aff.location == ri->location;
		});

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
void add_pair(T &target, const typename T::value_type &add)
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
void add_map(T &target, const T &add)
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

void do_help(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

#endif // HELP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
