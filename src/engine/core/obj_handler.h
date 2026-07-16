/**
\file obj_handler.h - a part of the Bylins engine.
\brief Placing/removing/extracting OBJECTS in rooms, containers and the world
       (issue.handler-cleaning split of handler).
*/
#ifndef BYLINS_SRC_ENGINE_CORE_OBJ_HANDLER_H_
#define BYLINS_SRC_ENGINE_CORE_OBJ_HANDLER_H_

#include "engine/structs/structs.h"   // RoomRnum / RoomVnum

#include <string>

class CharData;
class ObjData;

void InitUid(ObjData *object);
bool PlaceObjToRoom(ObjData *object, RoomRnum room);
bool CheckObjDecay(ObjData *object, bool need_extract = true);
void RemoveObjFromRoom(ObjData *object);
void PlaceObjIntoObj(ObjData *obj, ObjData *obj_to);
void RemoveObjFromObj(ObjData *obj);
void object_list_new_owner(ObjData *list, CharData *ch);
RoomVnum get_room_where_obj(ObjData *obj, bool deep = false);
void ExtractObjFromWorld(ObjData *obj, bool showlog = false);
// issue #3563: пишет в syslog о пропаже вещи у игрока; звать до отвязки от владельца.
void LogPlayerObjLoss(ObjData *obj, const char *reason);
// issue #3563: описание держателя для лога -- "игрока X" / "чармиса 'Y' игрока X" / "" (моб).
std::string ObjHolderLogDesc(CharData *holder);
void UpdateCharObjects(CharData *ch);
void DropObjOnZoneReset(CharData *ch, ObjData *obj, bool inv, bool zone_reset);
int get_object_low_rent(ObjData *obj);

#endif // BYLINS_SRC_ENGINE_CORE_OBJ_HANDLER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
