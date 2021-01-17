#ifndef BYLINS_FOLLOW_H
#define BYLINS_FOLLOW_H

#include "chars/char.hpp"

void do_follow(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
bool stop_follower(CHAR_DATA * ch, int mode);
bool die_follower(CHAR_DATA * ch);
bool circle_follow(CHAR_DATA * ch, CHAR_DATA * victim);

#endif //BYLINS_FOLLOW_H
