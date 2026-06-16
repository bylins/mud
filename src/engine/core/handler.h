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

const int kLightNo = 0;
const int kLightYes = 1;
const int kLightUndef = 2;
const int kSecsPerPlayerTimed = 1;
#include "engine/core/char_equip_flags.h"   // CharEquipFlag(s) (issue.handler-cleaning)

#include "engine/core/target_resolver.h"   // issue.handler-cleaning: target search moved here
using target_resolver::get_obj_vis_for_locate;
using target_resolver::try_locate_obj;
using target_resolver::generic_find;
using target_resolver::find_all_dots;
using target_resolver::FindRoomRnum;

int get_room_sky(int rnum);
int IsEquipInMetall(CharData *ch);
bool IsWearingLight(CharData *ch);   // issue.handler-cleaning: was file-local
// issue.handler-cleaning: defined in equipment.cpp (Bucket 1). Declarations kept here
// transitionally to avoid touching ~25 callers; a follow-up moves CharEquipFlag + these
// to equipment.h and drops them from handler.h.
void EquipObj(CharData *ch, ObjData *obj, int pos, const CharEquipFlags& equip_flags);
ObjData *UnequipChar(CharData *ch, int pos, const CharEquipFlags& equip_flags);
bool IsAwakeOthers(CharData *ch);

void CheckLight(CharData *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef);

// handling the affected-structures //
void ImposeTimedFeat(CharData *ch, TimedFeat *timed);
int IsTimedByFeat(CharData *ch, EFeat feat);
void ImposeTimedSkill(CharData *ch, TimedSkill *timed);
void ExpireTimedSkill(CharData *ch, ESkill skill);
int IsTimedBySkill(CharData *ch, ESkill id);

// utility //
char *fname(const char *namelist);
int get_number(char **name);
int get_number(std::string &name);

RoomVnum get_room_where_obj(ObjData *obj, bool deep = false);

// ******** objects *********** //
bool IsObjsStackable(ObjData *obj_one, ObjData *obj_two);
void PlaceObjToInventory(ObjData *object, CharData *ch);
void RemoveObjFromChar(ObjData *object);

bool HaveIncompatibleAlign(CharData *ch, ObjData *obj);

ObjData *get_obj_in_list(char *name, ObjData *list);
ObjData *get_obj_in_list(const char *name, const ObjData::obj_list_t &list);
ObjData *GetObjByRnumInContent(int obj_rnum, ObjData *list);
ObjData *GetObjByRnumInContent(int obj_rnum, const ObjData::obj_list_t &list);
ObjData *GetObjByVnumInContent(int vnum, ObjData *list);
ObjData *GetObjByVnumInContent(int vnum, const ObjData::obj_list_t &list);

//ObjData *get_obj(const char *name, int vnum = 0);

bool CheckObjDecay(ObjData *object, bool need_extract = true);
bool PlaceObjToRoom(ObjData *object, RoomRnum room);
void RemoveObjFromRoom(ObjData *object);
void PlaceObjIntoObj(ObjData *obj, ObjData *obj_to);
void RemoveObjFromObj(ObjData *obj);
void object_list_new_owner(ObjData *list, CharData *ch);

void ExtractObjFromWorld(ObjData *obj, bool showlog = false);

// ******* characters ********* //

//CharData *get_char(char *name);

void RemoveCharFromRoom(CharData *ch);
inline void char_from_room(const CharData::shared_ptr &ch) { RemoveCharFromRoom(ch.get()); }
void PlaceCharToRoom(CharData *ch, RoomRnum room);
inline void char_to_room(const CharData::shared_ptr &ch, RoomRnum room) { PlaceCharToRoom(ch.get(), room); }
void ExtractCharFromWorld(CharData *ch, int clear_objs, bool zone_reset = false);
void DropEquipment(CharData *ch, bool zone_reset);
void DropInventory(CharData *ch, bool zone_reset);

// find if character can see //
ObjData *get_obj_in_list_vis(CharData *ch, const char *name, const ObjData::obj_list_t &list, bool locate_item = false);
ObjData *get_obj_in_list_vis(CharData *ch, const char *name, ObjData *list, bool locate_item = false);
inline ObjData *get_obj_in_list_vis(CharData *ch,
									const std::string &name,
									const ObjData::obj_list_t &list) { return get_obj_in_list_vis(ch, name.c_str(), list); }
inline ObjData *get_obj_in_list_vis(CharData *ch,
									const std::string &name,
									ObjData *list) { return get_obj_in_list_vis(ch, name.c_str(), list); }



ObjData *get_object_in_equip_vis(CharData *ch, const char *arg, ObjData *equipment[], int *j);
inline ObjData *get_object_in_equip_vis(CharData *ch,
										const std::string &arg,
										ObjData *equipment[],
										int *j) { return get_object_in_equip_vis(ch, arg.c_str(), equipment, j); }
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



int get_object_low_rent(ObjData *obj);
void InitUid(ObjData *object);

void can_carry_obj(CharData *ch, ObjData *obj);
int num_pc_in_room(RoomData *room);
int check_moves(CharData *ch, int how_moves);
int real_sector(int room);

#endif // HANDLER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
