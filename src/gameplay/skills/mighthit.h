#ifndef BYLINS_MIGHTHIT_H
#define BYLINS_MIGHTHIT_H

class CharData;

void go_mighthit(CharData *ch, CharData *victim);
void do_mighthit(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_mighthit(CharData *ch, CharData *victim);

#endif //BYLINS_MIGHTHIT_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
