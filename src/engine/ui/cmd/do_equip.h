/**
\file equip.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Команда "надать/экипировать".
\detail Отсюда нужно вынести в механику собственно механику надевания, поиск слотов, требования к силе и все такие.
*/

#ifndef BYLINS_SRC_CMD_EQUIP_H_
#define BYLINS_SRC_CMD_EQUIP_H_

class CharData;
class ObjData;

void do_wear(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_wield(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_grab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);
void message_str_need(CharData *ch, ObjData *obj, int type);

bool CanBeTakenInBothHands(CharData *ch, ObjData *obj);
bool CanBeTakenInMajorHand(CharData *ch, ObjData *obj);
bool CanBeTakenInMinorHand(CharData *ch, ObjData *obj);
bool CanBeWearedAsShield(CharData *ch, ObjData *obj);

#endif //BYLINS_SRC_CMD_EQUIP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
