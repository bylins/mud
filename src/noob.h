// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef NOOB_HPP_INCLUDED
#define NOOB_HPP_INCLUDED

#include "structs/structs.h"

#include <string>
#include <vector>

class CharacterData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

namespace Noob {

void init();
int outfit(CharacterData *ch, void *me, int cmd, char *argument);

bool is_noob(const CharacterData *ch);
std::string print_start_outfit(CharacterData *ch);
std::vector<int> get_start_outfit(CharacterData *ch);
void check_help_message(CharacterData *ch);
void equip_start_outfit(CharacterData *ch, ObjectData *obj);

} // namespace Noob

#endif // NOOB_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
