#ifndef BYLINS_STRANGLE_H
#define BYLINS_STRANGLE_H

class CharData;

void go_strangle(CharData *ch, CharData *vict);
void do_strangle(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_STRANGLE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
