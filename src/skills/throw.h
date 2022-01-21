#ifndef BYLINS_THROW_H
#define BYLINS_THROW_H

class CharacterData;

void go_throw(CharacterData *ch, CharacterData *victim);
void do_throw(CharacterData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_THROW_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
