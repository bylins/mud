// map_options.h - опции миникарты, вынесены из mapsystem.h
// чтобы char_player.h не тянул тяжёлые зависимости

#ifndef MAP_OPTIONS_H_INCLUDED
#define MAP_OPTIONS_H_INCLUDED

#include <bitset>

class CharData;

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
	MAP_MODE_GOD_BIG,
	TOTAL_MAP_OPTIONS
};

struct Options {
	void olc_menu(CharData *ch);
	void parse_menu(CharData *ch, const char *arg);
	void text_olc(CharData *ch, const char *arg);

	std::bitset<TOTAL_MAP_OPTIONS> bit_list_;
};

} // namespace MapSystem

#endif // MAP_OPTIONS_H_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
