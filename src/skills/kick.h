#ifndef BYLINS_KICK_H
#define BYLINS_KICK_H

class CharacterData;

void go_kick(CharacterData *ch, CharacterData *vict);
void do_kick(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_KICK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
