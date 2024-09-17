/**
\file money_drop.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_STATISTICS_MONEY_DROP_H_
#define BYLINS_SRC_GAMEPLAY_STATISTICS_MONEY_DROP_H_

class CharData;
namespace MoneyDropStat {

void add(int zone_vnum, long money);
void print(CharData *ch);
void print_log();

} // MoneyDropStat

#endif //BYLINS_SRC_GAMEPLAY_STATISTICS_MONEY_DROP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
