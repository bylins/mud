// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_DUPE_HPP_INCLUDED
#define OBJ_DUPE_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace ObjDupe
{

void load_global_uid();
void save_global_uid();
void add(OBJ_DATA *obj);
void remove(OBJ_DATA *obj);
void check(CHAR_DATA *ch, OBJ_DATA *object);

} // namespace ObjDupe

#endif // OBJ_DUPE_HPP_INCLUDED
