#ifndef BYLINS_SRC_CMD_REMOVE_H_
#define BYLINS_SRC_CMD_REMOVE_H_

class CharData;

void RemoveEquipment(CharData *ch, int pos);
void do_remove(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_REMOVE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
