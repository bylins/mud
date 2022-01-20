// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARCEL_HPP_INCLUDED
#define PARCEL_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"

namespace Parcel {

int delete_obj(int vnum);
void send(CharacterData *ch, CharacterData *mailman, long vict_uid, char *arg);
void print_sending_stuff(CharacterData *ch);
int print_spell_locate_object(CharacterData *ch, int count, std::string name);
bool has_parcel(CharacterData *ch);
void receive(CharacterData *ch, CharacterData *mailman);
void update_timers();
void show_stats(CharacterData *ch);
void load();
void save();
void bring_back(CharacterData *ch, CharacterData *mailman);

int print_imm_where_obj(CharacterData *ch, char *arg, int num);
ObjectData *locate_object(const char *str);

void olc_update_from_proto(int robj_num, ObjectData *olc_proto);

} // namespace Parcel

#endif // PARCEL_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
