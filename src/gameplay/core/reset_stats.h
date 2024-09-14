// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef RESET_STATS_HPP_INCLUDED
#define RESET_STATS_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/network/descriptor_data.h"

///
/// Платный сброс/перераспределение характеристик персонажа через главное меню.
///
namespace stats_reset {

enum Type : int {
	MAIN_STATS,
	RACE,
	FEATS,
	RELIGION,
	TOTAL_NUM
};

void init();
void print_menu(DescriptorData *d);
void parse_menu(DescriptorData *d, const char *arg);

} // namespace stats_reset

#endif // RESET_STATS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
