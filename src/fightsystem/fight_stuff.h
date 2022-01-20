#ifndef __FIGHT_STUFF_HPP__
#define __FIGHT_STUFF_HPP__

class CharacterData;    // to avoid inclusion "char.hpp" into header file.

bool check_tester_death(CharacterData *ch, CharacterData *killer);

#endif // __FIGHT_STUFF_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
