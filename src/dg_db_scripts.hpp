#ifndef __DG_DB_SCRIPT_HPP__
#define __DG_DB_SCRIPT_HPP__

#include <unordered_set>
#include <unordered_map>

class SCRIPT_DATA;	// to avoid inclusion of "dg_scripts.h"

using triggers_set_t = std::unordered_set<int>;
using owner_to_triggers_map_t = std::unordered_map<int, triggers_set_t>;
using trigger_to_owners_map_t = std::unordered_map<int, owner_to_triggers_map_t>;

void add_trig_to_owner(int vnum_owner, int vnum_trig, int vnum);

extern trigger_to_owners_map_t owner_trig;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :