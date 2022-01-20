#ifndef BYLINS_PROTECT_H
#define BYLINS_PROTECT_H

class CharacterData;

void go_protect(CharacterData *ch, CharacterData *vict);
void do_protect(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
CharacterData *try_protect(CharacterData *victim, CharacterData *ch);

#endif //BYLINS_PROTECT_H
