#include "id_converter.h"

#include "engine/db/world_objects.h"
#include "engine/entities/zone.h"

ZoneRnum get_zone_rnum_by_vnumum(RoomVnum vnum) {
	for (ZoneRnum counter = 0; counter < static_cast<ZoneRnum>(zone_table.size()); counter++) {
		if ((vnum >= (zone_table[counter].vnum * 100)) && (vnum <= (zone_table[counter].top))) {
			return counter;
		}
	}
	return -1;
}

// logic is the same as for RoomVnum. keep this function for consistency
ZoneRnum get_zone_rnum_by_obj_vnum(ObjVnum vnum) {
	return get_zone_rnum_by_vnumum(vnum);
}

// logic is the same as for RoomVnum. keep this function for consistency
ZoneRnum get_zone_rnum_by_mob_vnum(MobVnum vnum) {
	return get_zone_rnum_by_vnumum(vnum);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
