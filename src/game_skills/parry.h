#ifndef BYLINS_PARRY_H
#define BYLINS_PARRY_H

class CharData;

void do_multyparry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_parry(CharData *ch, char *argument, int cmd, int subcmd);
void parry_override(CharData *ch);

#endif //BYLINS_PARRY_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
