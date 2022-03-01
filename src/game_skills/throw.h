#ifndef BYLINS_THROW_H
#define BYLINS_THROW_H

class CharData;

void go_throw(CharData *ch, CharData *victim);
void do_throw(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_THROW_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
