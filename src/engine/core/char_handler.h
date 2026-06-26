/**
\file char_handler.h - a part of the Bylins engine.
\brief Placing/removing/extracting CHARACTERS in the world (issue.handler-cleaning split of handler).
*/
#ifndef BYLINS_SRC_ENGINE_CORE_CHAR_HANDLER_H_
#define BYLINS_SRC_ENGINE_CORE_CHAR_HANDLER_H_

#include "engine/entities/char_data.h"   // CharData(::shared_ptr) for the inlines below
#include "engine/structs/structs.h"      // RoomRnum

void RemoveCharFromRoom(CharData *ch);
inline void char_from_room(const CharData::shared_ptr &ch) { RemoveCharFromRoom(ch.get()); }
void PlaceCharToRoom(CharData *ch, RoomRnum room, bool process_entry_affects = true);
inline void char_to_room(const CharData::shared_ptr &ch, RoomRnum room) { PlaceCharToRoom(ch.get(), room); }
void ExtractCharFromWorld(CharData *ch, int clear_objs, bool zone_reset = false);
void DropEquipment(CharData *ch, bool zone_reset);
void DropInventory(CharData *ch, bool zone_reset);

#endif // BYLINS_SRC_ENGINE_CORE_CHAR_HANDLER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
