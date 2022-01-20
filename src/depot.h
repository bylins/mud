// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEPOT_HPP_INCLUDED
#define DEPOT_HPP_INCLUDED

#include "entities/obj.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "conf.h"

#include <string>
#include <set>

namespace Depot {

void init_depot();
void load_chests();
void save_timedata();
void save_all_online_objs();
void update_timers();
void save_char_by_uid(int uid);

bool is_depot(ObjectData *obj);
void show_depot(CharacterData *ch);
bool put_depot(CharacterData *ch, const ObjectData::shared_ptr &obj);
void take_depot(CharacterData *ch, char *arg, int howmany);
int delete_obj(int vnum);

int get_total_cost_per_day(CharacterData *ch);
void show_stats(CharacterData *ch);

void enter_char(CharacterData *ch);
void exit_char(CharacterData *ch);
void reload_char(long uid, CharacterData *ch);

int print_spell_locate_object(CharacterData *ch, int count, std::string name);
bool show_purged_message(CharacterData *ch);
int print_imm_where_obj(CharacterData *ch, char *arg, int num);
ObjectData *locate_object(const char *str);

void olc_update_from_proto(int robj_num, ObjectData *olc_proto);
void rename_char(CharacterData *ch);
void add_offline_money(long uid, int money);

bool find_set_item(CharacterData *ch, const std::set<int> &vnum_list);
int report_unrentables(CharacterData *ch, CharacterData *recep);

void check_rented(int uid);
void delete_set_item(int uid, int vnum);

} // namespace Depot

#endif // DEPOT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
