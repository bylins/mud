#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "structs/structs.h"

class CharData;
void DecayPortalMessage(RoomRnum room_num);
void LoadTownportals();
bool ViewTownportal(CharData *ch, int where_bits);
void ShowPortalRunestone(CharData *ch);
void AddTownportalToChar(CharData *ch, RoomVnum vnum);
void CleanupSurplusPortals(CharData *ch);

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/

namespace one_way_portal {
void ReplacePortalTimer(CharData *ch, RoomRnum from_room, RoomRnum to_room, int time);

} // namespace OneWayPortal


#endif //BYLINS_TOWNPORTAL_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
