#ifndef BYLINS_CHOPOFF_H
#define BYLINS_CHOPOFF_H

#include "entities/char.h"

void go_chopoff(CharacterData *ch, CharacterData *vict);
void do_chopoff(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_CHOPOFF_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
