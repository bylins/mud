/**
\file boat.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_BOAT_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_BOAT_H_

#include "engine/structs/structs.h"

class CharData;
bool HasBoat(CharData *ch);
bool IsCharNeedBoatThere(CharData *ch, RoomRnum room_rnum);
bool IsCharCanDrownThere(CharData *ch, RoomRnum room_rnum);
bool IsCharNeedBoatToMove(CharData *ch, int dir);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_BOAT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
