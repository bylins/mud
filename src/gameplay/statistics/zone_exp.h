/**
\file zone_exp.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_STATISTICS_ZONE_EXP_H_
#define BYLINS_SRC_GAMEPLAY_STATISTICS_ZONE_EXP_H_

class CharData;
namespace ZoneExpStat {

void add(int zone_vnum, long exp);
void print_gain(CharData *ch);
void print_log();

} // ZoneExpStat

#endif //BYLINS_SRC_GAMEPLAY_STATISTICS_ZONE_EXP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
