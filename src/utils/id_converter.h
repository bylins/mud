#pragma once

#include "structs/structs.h"

ZoneRnum get_zone_rnum_by_room_vnum(RoomVnum vnum);
ZoneRnum get_zone_rnum_by_obj_vnum(ObjVnum vnum);
ZoneRnum get_zone_rnum_by_mob_vnum(MobVnum vnum);

ZoneRnum get_zone_rnum_by_zone_vnum(ZoneVnum vnum);

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
