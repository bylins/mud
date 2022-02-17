// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MAP_HPP_INCLUDED
#define MAP_HPP_INCLUDED

#include "conf.h"
#include <string>
#include <bitset>

#include "sysdep.h"
#include "structs/structs.h"
#include "entities/char_data.h"
#include "interpreter.h"

void do_map(CharData *ch, char *argument, int cmd, int subcmd);

namespace MapSystem {

enum {
	MAP_MODE_MOBS,
	MAP_MODE_PLAYERS,
	MAP_MODE_MOBS_CORPSES,
	MAP_MODE_PLAYER_CORPSES,
	MAP_MODE_INGREDIENTS,
	MAP_MODE_OTHER_OBJECTS,
	MAP_MODE_1_DEPTH,
	MAP_MODE_2_DEPTH,
	MAP_MODE_DEPTH_FIXED,
	MAP_MODE_MOB_SPEC_SHOP,
	MAP_MODE_MOB_SPEC_RENT,
	MAP_MODE_MOB_SPEC_MAIL,
	MAP_MODE_MOB_SPEC_BANK,
	MAP_MODE_MOB_SPEC_EXCH,
	MAP_MODE_MOB_SPEC_HORSE,
	MAP_MODE_MOB_SPEC_TEACH,
	MAP_MODE_MOB_SPEC_TORC,
	MAP_MODE_MOB_SPEC_ALL,
	MAP_MODE_MOBS_CURR_ROOM,
	MAP_MODE_OBJS_CURR_ROOM,
	MAP_MODE_BIG,
	TOTAL_MAP_OPTIONS
};

void print_map(CharData *ch, CharData *imm = 0);
void do_command(CharData *ch, const std::string &arg);

struct Options {
	void olc_menu(CharData *ch);
	void parse_menu(CharData *ch, const char *arg);
	void text_olc(CharData *ch, const char *arg);

	std::bitset<TOTAL_MAP_OPTIONS> bit_list_;
};

} // namespace MapSystem

#endif // MAP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
