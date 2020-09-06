#ifndef BYLINS_PROTECT_H
#define BYLINS_PROTECT_H

#include "chars/char.hpp"

void go_protect(CHAR_DATA * ch, CHAR_DATA * vict);
void do_protect(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
CHAR_DATA *try_protect(CHAR_DATA * victim, CHAR_DATA * ch);


#endif //BYLINS_PROTECT_H
