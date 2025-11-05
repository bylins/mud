#ifndef BYLINS_RESQUE_H
#define BYLINS_RESQUE_H

class CharData;

void do_rescue(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_rescue(CharData *ch, CharData *vict, CharData *tmp_ch);

#endif //BYLINS_RESQUE_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
