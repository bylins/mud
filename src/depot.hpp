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
extern const int SHARE_CHEST_VNUM;
extern int PERS_CHEST_RNUM;
extern int SHARE_CHEST_RNUM;

void init_depot();
void save_online_objs();
void save_timedata();
void update_timers();
void load_share_depots();

void enter_char(CHAR_DATA *ch);
void exit_char(CHAR_DATA *ch);

int is_depot(CHAR_DATA *ch, OBJ_DATA *obj);
bool put_depot(CHAR_DATA *ch, OBJ_DATA *obj, int type);
void show_depot(CHAR_DATA * ch, OBJ_DATA * obj, int type);
void take_depot(CHAR_DATA *ch, char *arg, int howmany, int type);

SPECIAL(Special);
int get_cost_per_day(CHAR_DATA *ch);
void rename_char(long uid);
void reload_char(long uid);
void show_stats(CHAR_DATA *ch);

} // namespace Depot

#endif // DEPOT_HPP_INCLUDED
