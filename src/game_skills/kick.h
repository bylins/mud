#ifndef BYLINS_KICK_H
#define BYLINS_KICK_H

class CharData;

void go_kick(CharData *ch, CharData *vict);
void do_kick(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_KICK_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
