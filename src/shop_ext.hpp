// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SHOP_EXT_HPP_INCLUDED
#define SHOP_EXT_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"
#include "dictionary.hpp"


namespace ShopExt
{

void do_shops_list(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void load(bool reload);
int get_spent_today();
void update_timers();

} // namespace ShopExt

int shop_ext(CHAR_DATA *ch, void *me, int cmd, char* argument);
void town_shop_keepers();
void fill_shop_dictionary(DictionaryType &dic);

#endif // SHOP_EXT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
