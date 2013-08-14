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

extern int BRONZE_MORT_NUM;
extern int SILVER_MORT_NUM;
extern int GOLD_MORT_NUM;

void torc_exch_menu(CHAR_DATA *ch);
void torc_exch_parse(CHAR_DATA *ch, const char *arg);

void gain_torc(CHAR_DATA *ch, CHAR_DATA *victim, int members);

} // namespace ExtMoney

namespace Remort
{

extern std::string WHERE_TO_REMORT_STR;

bool can_remort_now(CHAR_DATA *ch);
void init();

} // namespace Remort

SPECIAL(torc);

#endif // EXT_MONEY_HPP_INCLUDED
