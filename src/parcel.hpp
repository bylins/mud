// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARCEL_HPP_INCLUDED
#define PARCEL_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace Parcel
{


int delete_obj(int vnum);
void send(CHAR_DATA *ch, CHAR_DATA *mailman, long vict_uid, char *arg);
void print_sending_stuff(CHAR_DATA *ch);
int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name);
bool has_parcel(CHAR_DATA *ch);
void receive(CHAR_DATA *ch, CHAR_DATA *mailman);
void update_timers();
void show_stats(CHAR_DATA *ch);
void load();
void save();
void bring_back(CHAR_DATA *ch, CHAR_DATA *mailman);

int print_imm_where_obj(CHAR_DATA *ch, char *arg, int num);
OBJ_DATA * locate_object(const char *str);

void olc_update_from_proto(int robj_num, OBJ_DATA *olc_proto);

} // namespace Parcel

#endif // PARCEL_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
