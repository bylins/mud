#ifndef MOBACT_HPP_
#define MOBACT_HPP_

//#include "engine/structs/structs.h"

#include <unordered_map>
#include <vector>

class CharData;    // to avoid inclusion of "char.hpp"

namespace mob_ai {

// Настройки, кхм, ИИ мобов.
extern const short kStupidMob;
extern const short kMiddleAi;
extern const short kHighAi;
extern const short kCharacterHpForMobPriorityAttack;

void mobile_activity(int activity_level, int missed_pulses);
int perform_mob_switch(CharData *ch);
void do_aggressive_room(CharData *ch, int check_sneak);
bool find_master_charmice(CharData *charmice);
int attack_best(CharData *ch, CharData *victim, bool do_mode = false);
void extract_charmice(CharData *ch, bool on_ground);

} //namespace mob_ai

#endif // MOBACT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
