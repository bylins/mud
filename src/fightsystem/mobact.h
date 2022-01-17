#ifndef MOBACT_HPP_
#define MOBACT_HPP_

#include "structs/structs.h"

#include <unordered_map>
#include <vector>

class CHAR_DATA;    // to avoid inclusion of "char.hpp"

void mobile_activity(int activity_level, int missed_pulses);
int perform_mob_switch(CHAR_DATA *ch);
void do_aggressive_room(CHAR_DATA *ch, int check_sneak);
bool find_master_charmice(CHAR_DATA *charmice);

void mobRemember(CHAR_DATA *ch, CHAR_DATA *victim);
void mobForget(CHAR_DATA *ch, CHAR_DATA *victim);
void clearMemory(CHAR_DATA *ch);

struct mob_guardian {
	int max_wars_allow{};
	bool agro_killers{};
	bool agro_all_agressors{};
	std::vector<zone_vnum> agro_argressors_in_zones{};
};

typedef std::unordered_map<int, mob_guardian> guardian_type;

#endif // MOBACT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
