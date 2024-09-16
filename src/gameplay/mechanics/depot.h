// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEPOT_HPP_INCLUDED
#define DEPOT_HPP_INCLUDED

#include "engine/entities/obj_data.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"

#include <string>
#include <set>

namespace Depot {

void init_depot();
void load_chests();
void save_timedata();
void save_all_online_objs();
void update_timers();
void save_char_by_uid(int uid);

bool is_depot(ObjData *obj);
void show_depot(CharData *ch);
bool put_depot(CharData *ch, const ObjData::shared_ptr &obj);
void take_depot(CharData *ch, char *arg, int howmany);
int delete_obj(int vnum);

int get_total_cost_per_day(CharData *ch);
void show_stats(CharData *ch);

void enter_char(CharData *ch);
void exit_char(CharData *ch);
void reload_char(long uid, CharData *ch);

int print_spell_locate_object(CharData *ch, int count, std::string name);
bool show_purged_message(CharData *ch);
int print_imm_where_obj(CharData *ch, char *arg, int num);
char *look_obj_depot(ObjData *obj);
ObjData *find_obj_from_depot_and_dec_number(char *arg, int &number);
ObjData *locate_object(const char *str);

void olc_update_from_proto(int robj_num, ObjData *olc_proto);
void rename_char(CharData *ch);
void add_offline_money(long uid, int money);

bool find_set_item(CharData *ch, const std::set<int> &vnum_list);
int report_unrentables(CharData *ch, CharData *recep);

void check_rented(int uid);
void delete_set_item(int uid, int vnum);

} // namespace Depot

#endif // DEPOT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
