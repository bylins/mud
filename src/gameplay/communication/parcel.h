// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARCEL_HPP_INCLUDED
#define PARCEL_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

namespace Parcel {

int delete_obj(int vnum);
void send(CharData *ch, CharData *mailman, long vict_uid, char *arg);
void print_sending_stuff(CharData *ch);
int print_spell_locate_object(CharData *ch, int count, std::string name);
bool has_parcel(CharData *ch);
void receive(CharData *ch, CharData *mailman);
void update_timers();
void show_stats(CharData *ch);
void load();
void save();
void bring_back(CharData *ch, CharData *mailman);

int print_imm_where_obj(CharData *ch, char *arg, int num);
ObjData *locate_object(const char *str);

void olc_update_from_proto(int robj_num, ObjData *olc_proto);

} // namespace Parcel

#endif // PARCEL_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
