#ifndef BYLINS_HIRE_H
#define BYLINS_HIRE_H

#include "chars/char.hpp"

#define MAXPRICE 9999999

void do_findhelpee(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_freehelpee(CHAR_DATA* ch, char* /* argument*/, int/* cmd*/, int/* subcmd*/);
int get_reformed_charmice_hp(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum);

#endif //BYLINS_HIRE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

