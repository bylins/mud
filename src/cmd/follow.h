#ifndef BYLINS_FOLLOW_H
#define BYLINS_FOLLOW_H

class CharacterData;

void do_follow(CharacterData *ch, char *argument, int cmd, int subcmd);
bool stop_follower(CharacterData *ch, int mode);
bool die_follower(CharacterData *ch);
bool circle_follow(CharacterData *ch, CharacterData *victim);

#endif //BYLINS_FOLLOW_H
