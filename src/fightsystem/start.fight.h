#ifndef BYLINS_START_FIGHT_H
#define BYLINS_START_FIGHT_H

#include "chars/character.h"

void do_hit(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd);
void do_kill(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
int set_hit(CHAR_DATA * ch, CHAR_DATA * victim);

#endif //BYLINS_START_FIGHT_H
