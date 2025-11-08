#ifndef BYLINS_BLOCK_H
#define BYLINS_BLOCK_H

#include "gameplay/fight/fight_hit.h"

class CharData;
void do_block(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void ProcessShieldBlock(CharData *ch, CharData *victim, HitData &hit_data);
bool CanPerformAutoblock(CharData *ch);

#endif //BYLINS_BLOCK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
