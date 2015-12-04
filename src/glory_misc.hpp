// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef GLORY_MISC_HPP_INCLUDED
#define GLORY_MISC_HPP_INCLUDED

#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace GloryMisc
{

bool check_stats(CHAR_DATA *ch);
void recalculate_stats(CHAR_DATA *ch);

void load_log();
void save_log();
void add_log(int type, int num, std::string punish, std::string reason, CHAR_DATA *vict);
void show_log(CHAR_DATA *ch , char const * const value);

enum { ADD_GLORY = 1, REMOVE_GLORY, REMOVE_STAT, TRANSFER_GLORY, HIDE_GLORY };

} // namespace GloryMisc

#endif // GLORY_MISC_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
