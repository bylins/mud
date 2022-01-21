#ifndef MOBACT_HPP_
#define MOBACT_HPP_

#include "structs/structs.h"

#include <unordered_map>
#include <vector>

class CharacterData;    // to avoid inclusion of "char.hpp"

// Настройки, кхм, ИИ мобов.
const short kStupidMod = 10;
const short kMiddleAI = 30;
const short kHighAI = 40;
const short kCharacterHPForMobPriorityAttack = 100;

void mobile_activity(int activity_level, int missed_pulses);
int perform_mob_switch(CharacterData *ch);
void do_aggressive_room(CharacterData *ch, int check_sneak);
bool find_master_charmice(CharacterData *charmice);

void mobRemember(CharacterData *ch, CharacterData *victim);
void mobForget(CharacterData *ch, CharacterData *victim);
void clearMemory(CharacterData *ch);

struct mob_guardian {
	int max_wars_allow{};
	bool agro_killers{};
	bool agro_all_agressors{};
	std::vector<ZoneVnum> agro_argressors_in_zones{};
};

typedef std::unordered_map<int, mob_guardian> guardian_type;

#endif // MOBACT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
