#ifndef BYLINS_MIGHTHIT_H
#define BYLINS_MIGHTHIT_H

#include "gameplay/fight/fight_hit.h"

class CharData;
bool IsArmedWithMighthitWeapon(CharData *ch);
void GoMighthit(CharData *ch, CharData *victim);
void DoMighthit(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DoMighthit(CharData *ch, CharData *victim);
void ProcessMighthit(CharData *ch, CharData *victim, HitData &hit_data);

#endif //BYLINS_MIGHTHIT_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
