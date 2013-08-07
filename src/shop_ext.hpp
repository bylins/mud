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

ACMD(do_shops_list);
void load(bool reload);
int get_spent_today();
void renumber_obj_rnum(int rnum);
void update_timers();

} // namespace ShopExt

SPECIAL(shop_ext);
void town_shop_keepers();
void fill_shop_dictionary(DictionaryType &dic);

#endif // SHOP_EXT_HPP_INCLUDED
