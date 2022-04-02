// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef GLORY_HPP_INCLUDED
#define GLORY_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "interpreter.h"

namespace Glory {

int get_glory(long uid);
void add_glory(long uid, int amount);
int remove_glory(long uid, int amount);

void do_spend_glory(CharData *ch, char *argument, int cmd, int subcmd);
bool parse_spend_glory_menu(CharData *ch, const char *arg);
void spend_glory_menu(CharData *ch);

void load_glory();
void save_glory();
void timers_update();
void set_stats(CharData *ch);
int get_spend_glory(CharData *ch);

bool remove_stats(CharData *ch, CharData *god, int amount);
void transfer_stats(CharData *ch, CharData *god, const std::string& name, char *reason);
void show_glory(CharData *ch, CharData *god);
void show_stats(CharData *ch);

void print_glory_top(CharData *ch);
void hide_char(CharData *vict, CharData *god, char const *const mode);

void set_freeze(long uid);
void remove_freeze(long uid);
void check_freeze(CharData *ch);

} // namespace Glory

#endif // GLORY_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
