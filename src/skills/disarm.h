#ifndef BYLINS_DISARM_H
#define BYLINS_DISARM_H

class CharacterData;

void go_disarm(CharacterData *ch, CharacterData *vict);
void do_disarm(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_DISARM_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
