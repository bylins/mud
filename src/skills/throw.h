#ifndef BYLINS_THROW_H
#define BYLINS_THROW_H

class CHAR_DATA;

void go_throw(CHAR_DATA * ch, CHAR_DATA * victim);
void do_throw(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_THROW_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
