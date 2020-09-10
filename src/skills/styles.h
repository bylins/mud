#ifndef BYLINS_STYLES_H
#define BYLINS_STYLES_H

#include "chars/char.hpp"

void go_touch(CHAR_DATA * ch, CHAR_DATA * vict);
void do_touch(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

void go_deviate(CHAR_DATA * ch);
void do_deviate(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);

void do_style(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_STYLES_H
