// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef EXT_MONEY_HPP_INCLUDED
#define EXT_MONEY_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include <string>

class CharacterData;

namespace ExtMoney {

void torc_exch_menu(CharacterData *ch);
void torc_exch_parse(CharacterData *ch, const char *arg);

void drop_torc(CharacterData *mob);
std::string draw_daily_limit(CharacterData *ch, bool imm_stat = false);

void player_drop_log(CharacterData *ch, unsigned type, int num);
std::string name_currency_plural(std::string name);

} // namespace ExtMoney

namespace Remort {

extern std::string WHERE_TO_REMORT_STR;

bool can_remort_now(CharacterData *ch);
void init();
void show_config(CharacterData *ch);
bool need_torc(CharacterData *ch);

} // namespace Remort

int torc(CharacterData *ch, void *me, int cmd, char *argument);

#endif // EXT_MONEY_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
