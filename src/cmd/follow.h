#ifndef BYLINS_FOLLOW_H
#define BYLINS_FOLLOW_H

class CharData;

void do_follow(CharData *ch, char *argument, int cmd, int subcmd);
bool stop_follower(CharData *ch, int mode);
bool die_follower(CharData *ch);
bool circle_follow(CharData *ch, CharData *victim);

#endif //BYLINS_FOLLOW_H
