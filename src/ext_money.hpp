// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef EXT_MONEY_HPP_INCLUDED
#define EXT_MONEY_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
//#include "chars/char.hpp"
#include <string>

class CHAR_DATA;

namespace ExtMoney {
    void torc_exch_menu(CHAR_DATA *ch);
    void torc_exch_parse(CHAR_DATA *ch, const char *arg);

    void drop_torc(CHAR_DATA *mob);
    std::string draw_daily_limit(CHAR_DATA *ch, bool imm_stat = false);

    void player_drop_log(CHAR_DATA *ch, unsigned type, int num);
    std::string name_currency_plural(std::string name);
    void show_config(CHAR_DATA *ch);

} // namespace ExtMoney

struct TorcReq
{
    TorcReq(int rmrt);
    // тип гривн
    unsigned type;
    // кол-во
    int amount;
};


int torc(CHAR_DATA *ch, void *me, int cmd, char* argument);

#endif // EXT_MONEY_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
