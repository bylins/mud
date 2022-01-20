#ifndef BYLINS_FLEE_H
#define BYLINS_FLEE_H

class CharacterData;

void go_flee(CharacterData *ch);
void do_flee(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_FLEE_H


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
