/**
\file sight.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief Модуль оператора над игровыми сущностями - персонажами, предметами и комнатами.
\details Тут должен размещаться код, который перемещает и размещает персонажей, предметы и мобов.
 То есть, в комнату, из комнаты, в контейнер, в инвентарь и обратно и так далее. Либо сообщает их численность,
 составляет списки и тому подобное. Что тут нужно сделать - удалить отсюда и перенести в соответствующие модули
 код, который не относится к таким действиям. Если после этого модуль будет чересчур большим - раздеить его
 на хендлеры объектов, персонажей и комнат/мира.
*/

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef HANDLER_H_
#define HANDLER_H_

#include "engine/entities/char_data.h"
#include "engine/structs/structs.h"    // there was defined type "byte" if it had been missing
#include "engine/structs/flags.hpp"

struct RoomData;

#include "engine/core/char_equip_flags.h"   // CharEquipFlag(s) (issue.handler-cleaning)
#include "utils/parse.h"   // issue.handler-cleaning: get_number
#include "engine/core/char_movement.h"   // issue.handler-cleaning: real_sector/check_moves/num_pc_in_room
#include "gameplay/mechanics/equipment.h"   // issue.handler-cleaning: equip queries
#include "gameplay/mechanics/illumination.h"   // issue.handler-cleaning: room light + IsWearingLight/CheckLight
#include "gameplay/abilities/timed_abilities.h"   // issue.handler-cleaning: timed feat/skill timers
#include "gameplay/mechanics/inventory.h"   // issue.handler-cleaning: inventory API
#include "engine/core/char_handler.h"   // issue.handler-cleaning: char placement/extraction
#include "engine/core/obj_handler.h"     // issue.handler-cleaning: object placement/extraction

#include "engine/core/target_resolver.h"   // issue.handler-cleaning: target search moved here

int get_room_sky(int rnum);
bool IsAwakeOthers(CharData *ch);


// handling the affected-structures //

// utility //


// ******** objects *********** //



//ObjData *get_obj(const char *name, int vnum = 0);



// ******* characters ********* //

//CharData *get_char(char *name);


// find if character can see //



// find all dots //




// Generic Find //



// prototypes from crash save system //
int Crash_delete_crashfile(CharData *ch);
void Crash_listrent(CharData *ch, char *name);
int Crash_load(CharData *ch);
void Crash_crashsave(CharData *ch);
void Crash_idlesave(CharData *ch);





#endif // HANDLER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
