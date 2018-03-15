#ifndef __MOBACT_HPP__
#define __MOBACT_HPP__

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

void clearMemory(CHAR_DATA* ch);
void mobile_activity(int activity_level, int missed_pulses);

#endif // __MOBACT_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
