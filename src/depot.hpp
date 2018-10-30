// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEPOT_HPP_INCLUDED
#define DEPOT_HPP_INCLUDED

#include "obj.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <string>
#include <set>

namespace Depot
{

void init_depot();
void load_chests();
void save_timedata();
void save_all_online_objs();
void update_timers();
void save_char_by_uid(int uid);

bool is_depot(OBJ_DATA *obj);
void show_depot(CHAR_DATA *ch);
bool put_depot(CHAR_DATA *ch, const OBJ_DATA::shared_ptr& obj);
void take_depot(CHAR_DATA *ch, char *arg, int howmany);
int delete_obj(int vnum);


int get_total_cost_per_day(CHAR_DATA *ch);
void show_stats(CHAR_DATA *ch);

void enter_char(CHAR_DATA *ch);
void exit_char(CHAR_DATA *ch);
void reload_char(long uid, CHAR_DATA *ch);

int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name);
bool show_purged_message(CHAR_DATA *ch);
int print_imm_where_obj(CHAR_DATA *ch, char *arg, int num);
OBJ_DATA * locate_object(const char *str);

void olc_update_from_proto(int robj_num, OBJ_DATA *olc_proto);
void rename_char(CHAR_DATA *ch);
void add_offline_money(long uid, int money);

bool find_set_item(CHAR_DATA *ch, const std::set<int> &vnum_list);
int report_unrentables(CHAR_DATA *ch, CHAR_DATA *recep);

void check_rented(int uid);
void delete_set_item(int uid, int vnum);

} // namespace Depot

#endif // DEPOT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
