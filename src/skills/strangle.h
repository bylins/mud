#ifndef BYLINS_STRANGLE_H
#define BYLINS_STRANGLE_H

class CharacterData;

void go_strangle(CharacterData *ch, CharacterData *vict);
void do_strangle(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_STRANGLE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
