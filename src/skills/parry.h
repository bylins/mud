#ifndef BYLINS_PARRY_H
#define BYLINS_PARRY_H

class CharacterData;

void do_multyparry(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_parry(CharacterData *ch, char *argument, int cmd, int subcmd);
void parry_override(CharacterData *ch);

#endif //BYLINS_PARRY_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
