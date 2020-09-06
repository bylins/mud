#ifndef BYLINS_DISARM_H
#define BYLINS_DISARM_H

#include "chars/char.hpp"

void go_disarm(CHAR_DATA * ch, CHAR_DATA * vict);
void do_disarm(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_DISARM_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :