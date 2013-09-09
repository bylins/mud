// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef NOOB_HPP_INCLUDED
#define NOOB_HPP_INCLUDED

#include "conf.h"
#include <string>
#include <vector>
#include "sysdep.h"
#include "structs.h"
#include "char.hpp"

namespace Noob
{

void init();
SPECIAL(outfit);

bool is_noob(const CHAR_DATA *ch);
std::string print_start_outfit(CHAR_DATA *ch);
std::vector<int> get_start_outfit(CHAR_DATA *ch);
void check_help_message(CHAR_DATA *ch);
void equip_start_outfit(CHAR_DATA *ch, OBJ_DATA *obj);

} // namespace Noob

#endif // NOOB_HPP_INCLUDED
