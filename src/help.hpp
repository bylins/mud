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

#include "sysdep.h"
#include "interpreter.h"

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

std::string print_skills(const std::map<int, int> &skills, bool activ);
void sum_affected(std::vector<obj_affected_type> &l,
	const std::array<obj_affected_type, MAX_OBJ_AFFECT> &r);
void sum_skills(std::map<int, int> &target, const std::map<int, int> &add);

} // namespace PrintActivators

ACMD(do_help);

#endif // HELP_HPP_INCLUDED
