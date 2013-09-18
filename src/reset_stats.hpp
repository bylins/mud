// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef RESET_STATS_HPP_INCLUDED
#define RESET_STATS_HPP_INCLUDED

#include "conf.h"
#include <string>
#include "sysdep.h"
#include "structs.h"

///
/// Платный сброс/перераспределение характеристик персонажа через главное меню.
///
namespace ResetStats
{

enum Type
{
	MAIN_STATS,
	RACE,
	TOTAL_NUM
};

void init();
void print_menu(DESCRIPTOR_DATA *d);
void parse_menu(DESCRIPTOR_DATA *d, const char *arg);

} // namespace ResetStats

#endif // RESET_STATS_HPP_INCLUDED
