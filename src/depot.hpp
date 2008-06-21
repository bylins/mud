// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEPOT_HPP_INCLUDED
#define DEPOT_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace Depot {

extern const int PERS_CHEST_VNUM;
extern int PERS_CHEST_RNUM;

void init_depot();
void load_chests();
void save_timedata();
void save_all_online_objs();
void update_timers();

bool is_depot(OBJ_DATA *obj);
void show_depot(CHAR_DATA * ch, OBJ_DATA * obj);
bool put_depot(CHAR_DATA *ch, OBJ_DATA *obj);
void take_depot(CHAR_DATA *ch, char *arg, int howmany);
void renumber_obj_rnum(int rnum);

int get_total_cost_per_day(CHAR_DATA *ch);
void show_stats(CHAR_DATA *ch);

void enter_char(CHAR_DATA *ch);
void exit_char(CHAR_DATA *ch);
void reload_char(long uid, CHAR_DATA *ch);

} // namespace Depot

#endif // DEPOT_HPP_INCLUDED
