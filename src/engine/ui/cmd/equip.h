#ifndef BYLINS_SRC_CMD_EQUIP_H_
#define BYLINS_SRC_CMD_EQUIP_H_

class CharData;
class ObjData;

void do_wear(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_wield(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_grab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);

#endif //BYLINS_SRC_CMD_EQUIP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
