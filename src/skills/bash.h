#ifndef BYLINS_BASH_H
#define BYLINS_BASH_H

class CharacterData;

void do_bash(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_bash(CharacterData *ch, CharacterData *vict);

#endif //BYLINS_BASH_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
