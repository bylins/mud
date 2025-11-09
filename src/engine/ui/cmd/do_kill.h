#ifndef BYLINS_START_FIGHT_H
#define BYLINS_START_FIGHT_H

#include "engine/entities/char_data.h"

void DoHit(CharData *ch, char *argument, int/* cmd*/, int subcmd);
void DoKill(CharData *ch, char *argument, int cmd, int subcmd);

#endif //BYLINS_START_FIGHT_H
