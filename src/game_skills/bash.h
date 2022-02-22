#ifndef BYLINS_BASH_H
#define BYLINS_BASH_H

class CharData;

void do_bash(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_bash(CharData *ch, CharData *vict);

#endif //BYLINS_BASH_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
