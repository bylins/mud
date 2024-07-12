#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "db.h"
#include "comm.h"
#include "entities/room_data.h"

class CharData;

void spell_townportal(CharData *ch, char *arg);

inline void DecayPortalMessage(const RoomRnum room_num) {
	act("Пентаграмма медленно растаяла.", false, world[room_num]->first_character(), 0, 0, kToRoom);
	act("Пентаграмма медленно растаяла.", false, world[room_num]->first_character(), 0, 0, kToChar);
}

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/

namespace OneWayPortal {
void ReplacePortalTimer(CharData *ch, RoomRnum from_room, RoomRnum to_room, int time);

} // namespace OneWayPortal


#endif //BYLINS_TOWNPORTAL_H
