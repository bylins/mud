#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "db.h"
#include "comm.h"
#include "entities/room_data.h"

class CharData;

void spell_townportal(CharData *ch, char *arg);
void AddPortalTimer(CharData *ch, RoomData *from_room, RoomRnum to_room, int time);
RoomRnum IsRoomWithPortal(RoomRnum room);

inline void DecayPortalMessage(const RoomRnum room_num) {
	act("Пентаграмма медленно растаяла.", false, world[room_num]->first_character(), 0, 0, kToRoom);
	act("Пентаграмма медленно растаяла.", false, world[room_num]->first_character(), 0, 0, kToChar);
}

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/
namespace OneWayPortal {

void add(RoomData *to_room, RoomData *from_room);
void remove(RoomData *to_room);
RoomData *get_from_room(RoomData *to_room);
RoomData *get_to_room(RoomData *from_room);

} // namespace OneWayPortal


#endif //BYLINS_TOWNPORTAL_H
