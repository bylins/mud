#ifndef BYLINS_MIGHTHIT_H
#define BYLINS_MIGHTHIT_H

class CharacterData;

void go_mighthit(CharacterData *ch, CharacterData *victim);
void do_mighthit(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_MIGHTHIT_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
