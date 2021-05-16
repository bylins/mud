#ifndef _ACT_MOVEMENT_HPP_
#define _ACT_MOVEMENT_HPP_

#include "chars/character.h"

enum DOOR_SCMD : int
{
	SCMD_OPEN   = 0,    // открыть
	SCMD_CLOSE  = 1,   // закрыть
	SCMD_UNLOCK = 2,  // отпереть
	SCMD_LOCK   = 3,    // запереть
	SCMD_PICK   = 4  // взломать
};

enum FD_RESULT : int
{
	FD_WRONG_DIR         = -1,  // -1 НЕВЕРНОЕ НАПРАВЛЕНИЕ
	FD_WRONG_DOOR_NAME   = -2,  // -2 НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
	FD_NO_DOOR_GIVEN_DIR = -3,  // -3 В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРИ
	FD_DOORNAME_EMPTY    = -4 , // -4 НЕ УКАЗАНО АРГУМЕНТОВ
	FD_DOORNAME_WRONG    = -5   // -5 НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ
};

void do_doorcmd(CHAR_DATA * ch, OBJ_DATA * obj, int door, DOOR_SCMD scmd);
void do_gen_door(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_enter(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_stand(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_rest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sleep(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_wake(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

int has_boat(CHAR_DATA * ch);
int has_key(CHAR_DATA * ch, obj_vnum key);
int ok_pick(CHAR_DATA * ch, obj_vnum keynum, OBJ_DATA* obj, int door, int scmd);
int legal_dir(CHAR_DATA * ch, int dir, int need_specials_check, int show_msg);

int skip_hiding(CHAR_DATA * ch, CHAR_DATA * vict);
int skip_sneaking(CHAR_DATA * ch, CHAR_DATA * vict);
int skip_camouflage(CHAR_DATA * ch, CHAR_DATA * vict);

#endif // _ACT_MOVEMENT_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
