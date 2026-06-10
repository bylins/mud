#ifndef __FIGHT_STUFF_HPP__
#define __FIGHT_STUFF_HPP__

class CharData;    // to avoid inclusion "char.hpp" into header file.

bool check_tester_death(CharData *ch, CharData *killer);
bool stone_rebirth(CharData *ch, CharData *killer);

// issue.chardata-cleaning: when ch leaves combat/the room, fix up everyone targeting ch (protect,
// touch, extra-attack, cast, enemy) -- reassign to another foe or (need_stop) stop their fight.
void change_fighting(CharData *ch, int need_stop);

#endif // __FIGHT_STUFF_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
