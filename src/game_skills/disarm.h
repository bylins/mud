#ifndef BYLINS_DISARM_H
#define BYLINS_DISARM_H

class CharData;

void do_disarm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_injure(CharData *ch, CharData *vict);
void go_disarm(CharData *ch, CharData *vict);


#endif //BYLINS_DISARM_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
