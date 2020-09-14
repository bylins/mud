#ifndef __MOBACT_HPP__
#define __MOBACT_HPP__

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

void mobile_activity(int activity_level, int missed_pulses);
int perform_mob_switch(CHAR_DATA * ch);
void do_aggressive_room(CHAR_DATA *ch, int check_sneak);
bool find_master_charmice(CHAR_DATA *charmice);

void mobRemember(CHAR_DATA * ch, CHAR_DATA * victim);
void mobForget(CHAR_DATA * ch, CHAR_DATA * victim);
void clearMemory(CHAR_DATA* ch);



#endif // __MOBACT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
