// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEPOT_HPP_INCLUDED
#define DEPOT_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace Depot
{

extern const int PERS_CHEST_VNUM;
extern int PERS_CHEST_RNUM;

void init_depot();
void load_chests();
void save_timedata();
void save_all_online_objs();
void update_timers();

bool is_depot(OBJ_DATA *obj);
void show_depot(CHAR_DATA *ch);
bool put_depot(CHAR_DATA *ch, OBJ_DATA *obj);
void take_depot(CHAR_DATA *ch, char *arg, int howmany);

int get_total_cost_per_day(CHAR_DATA *ch);
void show_stats(CHAR_DATA *ch);

void enter_char(CHAR_DATA *ch);
void exit_char(CHAR_DATA *ch);
void reload_char(long uid, CHAR_DATA *ch);

int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name);
void show_purged_message(CHAR_DATA *ch);
int print_imm_where_obj(CHAR_DATA *ch, char *arg, int num);

void renumber_obj_rnum(int rnum);
void olc_update_from_proto(int robj_num, OBJ_DATA *olc_proto);

} // namespace Depot

#endif // DEPOT_HPP_INCLUDED
