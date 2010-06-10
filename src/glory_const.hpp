// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef GLORY_CONST_HPP_INCLUDED
#define GLORY_CONST_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"

namespace GloryConst
{

int get_glory(long uid);
void add_glory(long uid, int amount);

ACMD(do_glory);
ACMD(do_spend_glory);
void spend_glory_menu(CHAR_DATA *ch);
bool parse_spend_glory_menu(CHAR_DATA *ch, char *arg);

void save();
void load();

void set_stats(CHAR_DATA *ch);
int main_stats_count(CHAR_DATA *ch);

void show_stats(CHAR_DATA *ch);

} // namespace GloryConst

#endif // GLORY_CONST_HPP_INCLUDED
