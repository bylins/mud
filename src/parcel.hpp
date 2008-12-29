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

void send(CHAR_DATA *ch, CHAR_DATA *mailman, long vict_uid, char *arg);
void print_sending_stuff(CHAR_DATA *ch);
int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name);
bool has_parcel(CHAR_DATA *ch);
void receive(CHAR_DATA *ch, CHAR_DATA *mailman);
void update_timers();
void show_stats(CHAR_DATA *ch);
void load();
void enter_game(CHAR_DATA *ch);

} // namespace Parcel

#endif // PARCEL_HPP_INCLUDED
