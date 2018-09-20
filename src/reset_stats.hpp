// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef RESET_STATS_HPP_INCLUDED
#define RESET_STATS_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

///
/// Платный сброс/перераспределение характеристик персонажа через главное меню.
///
namespace ResetStats
{

enum Type : int
{
	MAIN_STATS,
	RACE,
	FEATS,
	RELIGION,
	TOTAL_NUM
};

void init();
void print_menu(DESCRIPTOR_DATA *d);
void parse_menu(DESCRIPTOR_DATA *d, const char *arg);

} // namespace ResetStats

#endif // RESET_STATS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
