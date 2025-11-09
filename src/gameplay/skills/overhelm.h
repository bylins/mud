#ifndef BYLINS_STUPOR_H
#define BYLINS_STUPOR_H

#include "gameplay/fight/fight_hit.h"

class CharData;
void GoOverhelm(CharData *ch, CharData *victim);
void DoOverhelm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DoOverhelm(CharData *ch, CharData *victim);
void ProcessOverhelm(CharData *ch, CharData *victim, HitData &hit_data);

#endif //BYLINS_STUPOR_H
