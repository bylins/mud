#ifndef BYLINS_SRC_CMD_DROP_H_
#define BYLINS_SRC_CMD_DROP_H_

class CharData;
class ObjData;

void PerformDropGold(CharData *ch, int amount);
void PerformDrop(CharData *ch, ObjData *obj);
void DoDrop(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/);

#endif //BYLINS_SRC_CMD_DROP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :