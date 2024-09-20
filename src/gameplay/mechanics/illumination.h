/**
\file illumination.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Освещенность/затемнение комнат.
\detail Все, что связано со степенью освещения и затемнения комнат надо будет пернести сюда.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_ILLUMINATION_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_ILLUMINATION_H_

#include "engine/structs/structs.h"

bool is_dark(RoomRnum room);
bool IsDefaultDark(RoomRnum room_rnum);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_ILLUMINATION_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
