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

void add(CharacterData *ch);
void remove(CharacterData *ch);
CharacterData *get_by_name(const char *str);

} // namespace CharacterAlias

namespace ObjectAlias {

void add(ObjectData *obj);
void remove(ObjectData *obj);
ObjectData *get_by_name(const char *str);
ObjectData *locate_object(const char *str);

} // namespace ObjectAlias

#endif // NAME_LIST_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
