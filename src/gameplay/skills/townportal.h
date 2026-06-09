#ifndef BYLINS_TOWNPORTAL_H
#define BYLINS_TOWNPORTAL_H

#include "engine/structs/structs.h"
// issue.runestones: the runestone classes (Runestone / RunestoneRoster / CharacterRunestoneRoster)
// moved to gameplay/mechanics/rune_stones.h. Re-included here so existing includers of townportal.h
// keep compiling unchanged (the townportal skill below uses runestones).
#include "gameplay/mechanics/rune_stones.h"

class CharData;

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/

namespace one_way_portal {
void ReplacePortalTimer(CharData *ch, RoomRnum from_room, RoomRnum to_room, int time);

} // namespace OneWayPortal


#endif //BYLINS_TOWNPORTAL_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
