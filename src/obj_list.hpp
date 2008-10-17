// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_LIST_HPP_INCLUDED
#define OBJ_LIST_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace ObjList
{

void add(OBJ_DATA *obj);
void remove(OBJ_DATA *obj);
size_t size();

OBJ_DATA * obj_by_id(int id);
OBJ_DATA * obj_by_name(char const *name);
OBJ_DATA * obj_by_rnum(obj_rnum nr);
OBJ_DATA * obj_by_nname(CHAR_DATA *ch, char const *name, int number);
int id_by_vnum(int vnum);

void paste();
void check_script();
void check_decay();
void point_update();
void repop_decay(zone_rnum zone);
void recalculate_rnum(int rnum);

void olc_update(obj_rnum nr, DESCRIPTOR_DATA *d);
int count_dupes(OBJ_DATA *obj);

void print_god_where(CHAR_DATA *ch, char const *arg);
int print_spell_locate_object(CHAR_DATA *ch, int count, char const *name);

} // namespace ObjList

#endif // OBJ_LIST_HPP_INCLUDED
