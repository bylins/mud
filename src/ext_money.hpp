// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef EXT_MONEY_HPP_INCLUDED
#define EXT_MONEY_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "char.hpp"

namespace ExtMoney
{

void torc_exch_menu(CHAR_DATA *ch);
void torc_exch_parse(CHAR_DATA *ch, const char *arg);

void drop_torc(CHAR_DATA *mob);
std::string draw_daily_limit(CHAR_DATA *ch, bool imm_stat = false);

void player_drop_log(CHAR_DATA *ch, unsigned type, int num);

} // namespace ExtMoney

namespace Remort
{

extern std::string WHERE_TO_REMORT_STR;

bool can_remort_now(CHAR_DATA *ch);
void init();
void show_config(CHAR_DATA *ch);
bool need_torc(CHAR_DATA *ch);

} // namespace Remort

SPECIAL(torc);

#endif // EXT_MONEY_HPP_INCLUDED
