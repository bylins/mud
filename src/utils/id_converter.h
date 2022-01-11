#pragma once

#include "structs/structs.h"

zone_rnum get_zone_rnum_by_room_vnum(room_vnum vnum);
zone_rnum get_zone_rnum_by_obj_vnum(obj_vnum vnum);
zone_rnum get_zone_rnum_by_mob_vnum(mob_vnum vnum);

zone_rnum get_zone_rnum_by_zone_vnum(zone_vnum vnum);

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
