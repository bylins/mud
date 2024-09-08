#ifndef ACT_MOVEMENT_HPP_
#define ACT_MOVEMENT_HPP_

#include "structs/structs.h"

class CharData;
class ObjData;

enum EDoorScmd : int {
  kScmdOpen,
  kScmdClose,
  kScmdUnlock,
  kScmdLock,
  kScmdPick
};

enum EFindDoorResult : int {
  FD_WRONG_DIR = -1,  // -1 НЕВЕРНОЕ НАПРАВЛЕНИЕ
  FD_WRONG_DOOR_NAME = -2,  // -2 НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
  FD_NO_DOOR_GIVEN_DIR = -3,  // -3 В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРИ
  FD_DOORNAME_EMPTY = -4, // -4 НЕ УКАЗАНО АРГУМЕНТОВ
  FD_DOORNAME_WRONG = -5   // -5 НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ
};

void do_doorcmd(CharData *ch, ObjData *obj, int door, EDoorScmd scmd);
void do_gen_door(CharData *ch, char *argument, int cmd, int subcmd);
void do_stand(CharData *ch, char *argument, int cmd, int subcmd);
void do_sit(CharData *ch, char *argument, int cmd, int subcmd);
void do_rest(CharData *ch, char *argument, int cmd, int subcmd);

int HasKey(CharData *ch, ObjVnum key);
bool HasBoat(CharData *ch);
bool ok_pick(CharData *ch, ObjVnum keynum, ObjData *obj, int door, int scmd);
bool IsCorrectDirection(CharData *ch, int dir, bool check_specials, bool show_msg);

int skip_hiding(CharData *ch, CharData *vict);
int skip_sneaking(CharData *ch, CharData *vict);
int skip_camouflage(CharData *ch, CharData *vict);

#endif // ACT_MOVEMENT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
