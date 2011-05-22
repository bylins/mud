// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SHOP_EXT_HPP_INCLUDED
#define SHOP_EXT_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"

namespace ShopExt
{

SPECIAL(shop_ext);
void load();
int get_spent_today();
void renumber_obj_rnum(int rnum);

} // namespace ShopExt

void town_shop_keepers();

#endif // SHOP_EXT_HPP_INCLUDED
