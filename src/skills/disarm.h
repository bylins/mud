#ifndef BYLINS_DISARM_H
#define BYLINS_DISARM_H

class CharData;

void go_disarm(CharData *ch, CharData *vict);
void do_disarm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_DISARM_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
