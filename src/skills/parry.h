#ifndef BYLINS_PARRY_H
#define BYLINS_PARRY_H

class CHAR_DATA;

void do_multyparry(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_parry(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void parry_override(CHAR_DATA * ch);

#endif //BYLINS_PARRY_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
