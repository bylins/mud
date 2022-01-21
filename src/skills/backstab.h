#ifndef BYLINS_BACKSTAB_H
#define BYLINS_BACKSTAB_H

class CharacterData;

void go_backstab(CharacterData *ch, CharacterData *vict);
void do_backstab(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_BACKSTAB_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

