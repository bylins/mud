#ifndef BYLINS_START_FIGHT_H
#define BYLINS_START_FIGHT_H

#include "entities/char.h"

void do_hit(CharacterData *ch, char *argument, int/* cmd*/, int subcmd);
void do_kill(CharacterData *ch, char *argument, int cmd, int subcmd);
int set_hit(CharacterData *ch, CharacterData *victim);

#endif //BYLINS_START_FIGHT_H
