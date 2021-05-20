#ifndef __FIGHT_STUFF_HPP__
#define __FIGHT_STUFF_HPP__

class CHAR_DATA;	// to avoid inclusion "char.hpp" into header file.

bool check_tester_death(CHAR_DATA *ch, CHAR_DATA *killer);

#endif // __FIGHT_STUFF_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
