// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef EXT_MONEY_HPP_INCLUDED
#define EXT_MONEY_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include <string>

class CharData;

namespace ExtMoney {

void torc_exch_menu(CharData *ch);
void torc_exch_parse(CharData *ch, const char *arg);

void drop_torc(CharData *mob);
std::string draw_daily_limit(CharData *ch, bool imm_stat = false);

void player_drop_log(CharData *ch, unsigned type, int num);
std::string name_currency_plural(std::string name);

} // namespace ExtMoney

namespace Remort {

extern std::string WHERE_TO_REMORT_STR;

bool can_remort_now(CharData *ch);
void init();
void show_config(CharData *ch);
bool need_torc(CharData *ch);

} // namespace Remort

#endif // EXT_MONEY_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
