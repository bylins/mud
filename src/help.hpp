// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

#include <string>
#include <vector>
#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"

ACMD(do_help);

namespace HelpSystem
{

extern bool need_update;
enum Flags { STATIC, DYNAMIC, TOTAL_NUM };

// добавление в справку топика в нужный массив через флаг
void add(const std::string key_str, const std::string entry_str, int min_level, HelpSystem::Flags add_flag);
// добавление сетов, идет в DYNAMIC массив с включенным sets_drop_page
void add_sets(const std::string key_str, const std::string entry_str);
// лоад/релоад конкретного массива справки
void reload(HelpSystem::Flags sort_flag);
// лоад/релоад всей справки
void reload_all();
// проверка раз в минуту нужно ли обновить динамическую справку (клан-сайты, групп-зоны)
void check_update_dynamic();

} // namespace HelpSystem

#endif // HELP_HPP_INCLUDED
