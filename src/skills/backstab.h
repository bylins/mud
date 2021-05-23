#ifndef BYLINS_BACKSTAB_H
#define BYLINS_BACKSTAB_H

class CHAR_DATA;

void go_backstab(CHAR_DATA *ch, CHAR_DATA *vict);
void do_backstab(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_BACKSTAB_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

