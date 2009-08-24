// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef NAME_LIST_HPP_INCLUDED
#define NAME_LIST_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

// оба списка воткнуты в попытке ускорить работу триггеров на этапе подстановки переменных

namespace CharacterList
{

void add(CHAR_DATA *ch);
void remove(CHAR_DATA *ch);
CHAR_DATA * get_by_name(const char *str);

} // namespace CharacterList

namespace ObjectList
{

void add(OBJ_DATA *obj);
void remove(OBJ_DATA *obj);
OBJ_DATA * get_by_name(const char *str);

} // namespace ObjectList

#endif // NAME_LIST_HPP_INCLUDED
