/**
\file doors.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Механика дверей и крышек контейнеров.
\detail Тут размещается код, который управляет оккрыванием/закрыванием/отпиранием/запиранием.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_DOORS_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_DOORS_H_

class CharData;

enum EDoorScmd : int {
  kScmdOpen,
  kScmdClose,
  kScmdUnlock,
  kScmdLock,
  kScmdPick
};

extern const char *a_cmd_door[];

void do_doorcmd(CharData *ch, ObjData *obj, int door, EDoorScmd scmd);
void go_gen_door(CharData *ch, char *type, char *dir, int where_bits, int subcmd);
bool IsPickLockSucessdul(CharData *ch, ObjVnum /*keynum*/, ObjData *obj, EDirection door, EDoorScmd scmd);
int HasKey(CharData *ch, ObjVnum key);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_DOORS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
