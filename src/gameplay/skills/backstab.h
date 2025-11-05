#ifndef BYLINS_BACKSTAB_H
#define BYLINS_BACKSTAB_H

#include "gameplay/fight/fight_hit.h"

class CharData;
void GoBackstab(CharData *ch, CharData *vict);
void DoBackstab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_backstab(CharData *ch, CharData *vict);
bool ProcessBackstab(CharData *ch, CharData *victim, HitData &hit_data);

#endif //BYLINS_BACKSTAB_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

