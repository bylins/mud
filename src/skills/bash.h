#ifndef BYLINS_BASH_H
#define BYLINS_BASH_H

class CHAR_DATA;

void do_bash(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_bash(CHAR_DATA * ch, CHAR_DATA * vict);

#endif //BYLINS_BASH_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
