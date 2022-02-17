// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef NAME_LIST_HPP_INCLUDED
#define NAME_LIST_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"

// оба списка воткнуты в попытке ускорить работу триггеров на этапе подстановки переменных

namespace CharacterAlias {

void add(CharData *ch);
void remove(CharData *ch);
CharData *get_by_name(const char *str);

} // namespace CharacterAlias

namespace ObjectAlias {

void add(ObjData *obj);
void remove(ObjData *obj);
ObjData *get_by_name(const char *str);
ObjData *locate_object(const char *str);

} // namespace ObjectAlias

#endif // NAME_LIST_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
