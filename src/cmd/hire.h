#ifndef BYLINS_HIRE_H
#define BYLINS_HIRE_H

class CharacterData;

#define MAXPRICE 9999999

void do_findhelpee(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_freehelpee(CharacterData *ch, char * /* argument*/, int/* cmd*/, int/* subcmd*/);
int get_reformed_charmice_hp(CharacterData *ch, CharacterData *victim, int spellnum);

#endif //BYLINS_HIRE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

