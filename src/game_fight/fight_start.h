#ifndef BYLINS_START_FIGHT_H
#define BYLINS_START_FIGHT_H

#include "entities/char_data.h"

void do_hit(CharData *ch, char *argument, int/* cmd*/, int subcmd);
void do_kill(CharData *ch, char *argument, int cmd, int subcmd);
int set_hit(CharData *ch, CharData *victim);

#endif //BYLINS_START_FIGHT_H
