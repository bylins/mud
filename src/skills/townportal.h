#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "chars/character.h"

void spell_townportal(CHAR_DATA *ch, char *arg);

inline void decay_portal(const int room_num)
{
    act("Пентаграмма медленно растаяла.", FALSE, world[room_num]->first_character(), 0, 0, TO_ROOM);
    act("Пентаграмма медленно растаяла.", FALSE, world[room_num]->first_character(), 0, 0, TO_CHAR);
    world[room_num]->portal_time = 0;
    world[room_num]->portal_room = 0;
}

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/
namespace OneWayPortal
{

    void add(ROOM_DATA* to_room, ROOM_DATA* from_room);
    void remove(ROOM_DATA* to_room);
    ROOM_DATA* get_from_room(ROOM_DATA* to_room);

} // namespace OneWayPortal


#endif //BYLINS_TOWNPORTAL_H
