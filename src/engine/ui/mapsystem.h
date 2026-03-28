// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MAP_HPP_INCLUDED
#define MAP_HPP_INCLUDED

#include <string>
#include "engine/ui/map_options.h"

class CharData;

void do_map(CharData *ch, char *argument, int cmd, int subcmd);

namespace MapSystem {

void print_map(CharData *ch, CharData *imm = 0);
void do_command(CharData *ch, const std::string &arg);

} // namespace MapSystem

#endif // MAP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
