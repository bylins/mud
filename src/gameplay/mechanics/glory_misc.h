// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef GLORY_MISC_HPP_INCLUDED
#define GLORY_MISC_HPP_INCLUDED

#include <string>
#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

namespace GloryMisc {

bool check_stats(CharData *ch);
void recalculate_stats(CharData *ch);

void load_log();
void save_log();
void add_log(int type, int num, std::string punish, std::string reason, CharData *vict);
void show_log(CharData *ch, char const *const value);

enum { ADD_GLORY = 1, REMOVE_GLORY, REMOVE_STAT, TRANSFER_GLORY, HIDE_GLORY };

} // namespace GloryMisc

#endif // GLORY_MISC_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
