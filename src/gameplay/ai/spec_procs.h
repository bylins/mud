/**
\file spec_procs.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_AI_SPEC_PROCS_H_
#define BYLINS_SRC_GAMEPLAY_AI_SPEC_PROCS_H_

#include "engine/structs/structs.h"

class CharData;
int exchange(CharData *ch, void *me, int cmd, char *argument);
int horse_keeper(CharData *ch, void *me, int cmd, char *argument);
int torc(CharData *ch, void *me, int cmd, char *argument);
int mercenary(CharData *ch, void * /*me*/, int cmd, char *argument);
int shop_ext(CharData *ch, void *me, int cmd, char *argument);
bool is_post(RoomRnum room);
bool is_rent(RoomRnum room);

int receptionist(CharData *, void *, int, char *);
int postmaster(CharData *, void *, int, char *);
int bank(CharData *, void *, int, char *);
int shop_ext(CharData *, void *, int, char *);
int mercenary(CharData *, void *, int, char *);

#endif //BYLINS_SRC_GAMEPLAY_AI_SPEC_PROCS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
