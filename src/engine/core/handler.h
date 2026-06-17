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

const int kSecsPerPlayerTimed = 1;
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
using target_resolver::get_obj_vis_for_locate;
using target_resolver::try_locate_obj;
using target_resolver::generic_find;
using target_resolver::find_all_dots;
using target_resolver::FindRoomRnum;

int get_room_sky(int rnum);
// issue.handler-cleaning: defined in equipment.cpp (Bucket 1). Declarations kept here
// transitionally to avoid touching ~25 callers; a follow-up moves CharEquipFlag + these
// to equipment.h and drops them from handler.h.
void EquipObj(CharData *ch, ObjData *obj, int pos, const CharEquipFlags& equip_flags);
ObjData *UnequipChar(CharData *ch, int pos, const CharEquipFlags& equip_flags);
bool IsAwakeOthers(CharData *ch);


// handling the affected-structures //

// utility //


// ******** objects *********** //


ObjData *get_obj_in_list(char *name, ObjData *list);
ObjData *get_obj_in_list(const char *name, const ObjData::obj_list_t &list);
ObjData *GetObjByVnumInContent(int vnum, const ObjData::obj_list_t &list);

//ObjData *get_obj(const char *name, int vnum = 0);



// ******* characters ********* //

//CharData *get_char(char *name);


// find if character can see //



// find all dots //


const int kFindIndiv = 0;
const int kFindAll = 1;
const int kFindAlldot = 2;


// Generic Find //

enum EFind : Bitvector {
	kCharInRoom = 1 << 0,
	kCharInWorld = 1 << 1,
	kCharDisconnected = 1 << 6,
	kObjInventory = 1 << 2,
	kObjRoom = 1 << 3,
	kObjWorld = 1 << 4,
	kObjEquip = 1 << 5,
	kObjExtraDesc = 1 << 7
};


// prototypes from crash save system //
int Crash_delete_crashfile(CharData *ch);
void Crash_listrent(CharData *ch, char *name);
int Crash_load(CharData *ch);
void Crash_crashsave(CharData *ch);
void Crash_idlesave(CharData *ch);





#endif // HANDLER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
