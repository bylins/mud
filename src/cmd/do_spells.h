#ifndef BYLINS_SRC_CMD_DO_SPELLS_H_
#define BYLINS_SRC_CMD_DO_SPELLS_H_

class CharData;

void DoSpells(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DisplaySpells(CharData *ch, CharData *vict, bool all);

#endif //BYLINS_SRC_CMD_DO_SPELLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
