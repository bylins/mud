#ifndef BYLINS_BACKSTAB_H
#define BYLINS_BACKSTAB_H

#include "game_fight/fight.h"
class CharData;

void go_backstab(CharData *ch, CharData *vict);
void do_backstab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
Damage CreateBackstabDamage(CharData *ch, CharData *vict);

#endif //BYLINS_BACKSTAB_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

