#ifndef MOBACT_HPP_
#define MOBACT_HPP_

#include "structs/structs.h"

#include <unordered_map>
#include <vector>

class CharData;    // to avoid inclusion of "char.hpp"

// Настройки, кхм, ИИ мобов.
const short kStupidMod = 10;
const short kMiddleAI = 30;
const short kHighAI = 40;
const short kCharacterHPForMobPriorityAttack = 100;

void mobile_activity(int activity_level, int missed_pulses);
int perform_mob_switch(CharData *ch);
void do_aggressive_room(CharData *ch, int check_sneak);
bool find_master_charmice(CharData *charmice);

void mobRemember(CharData *ch, CharData *victim);
void mobForget(CharData *ch, CharData *victim);
void clearMemory(CharData *ch);

#endif // MOBACT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
