#ifndef __SPEEDWALKS_HPP__
#define __SPEEDWALKS_HPP__

#include <vector>
#include <string>

struct Route {
	std::string direction;
	int wait;
};

class CharData;    // to avoid inclusion of char.hpp

struct SpeedWalk {
	using mob_vnums_t = std::vector<int>;
	using routes_t = std::vector<Route>;
	using mobs_t = std::vector<CharData *>;

	int default_wait;
	mob_vnums_t vnum_mobs;
	routes_t route;
	int cur_state;
	int wait;
	mobs_t mobs;
};

using Speedwalks = std::vector<SpeedWalk>;

extern Speedwalks &speedwalks;

#endif // __SPEEDWALKS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
