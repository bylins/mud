#ifndef BYLINS_SRC_CMD_GET_H_
#define BYLINS_SRC_CMD_GET_H_

class CharData;
class ObjData;

void split_or_clan_tax(CharData *ch, long amount);
void get_check_money(CharData *ch, ObjData *obj, ObjData *cont);
bool perform_get_from_container(CharData *ch, ObjData *obj, ObjData *cont, int mode);
void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot);
int perform_get_from_room(CharData *ch, ObjData *obj);
void do_get(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_GET_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
