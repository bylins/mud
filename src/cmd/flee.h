#ifndef BYLINS_FLEE_H
#define BYLINS_FLEE_H

class CharData;

void go_flee(CharData *ch);
void do_flee(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_FLEE_H


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
