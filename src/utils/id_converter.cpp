#include "id_converter.h"

#include "world_objects.h"
#include "entities/zone.h"

ZoneRnum get_zone_rnum_by_room_vnum(RoomVnum vnum) {
	for (ZoneRnum counter = 0; counter < static_cast<ZoneRnum>(zone_table.size()); counter++)
		if ((vnum >= (zone_table[counter].vnum * 100)) && (vnum <= (zone_table[counter].top)))
			return counter;

	return -1;
}

// logic is the same as for RoomVnum. keep this function for consistency
ZoneRnum get_zone_rnum_by_obj_vnum(ObjVnum vnum) {
	return get_zone_rnum_by_room_vnum(vnum);
}

// logic is the same as for RoomVnum. keep this function for consistency
ZoneRnum get_zone_rnum_by_mob_vnum(MobVnum vnum) {
	return get_zone_rnum_by_room_vnum(vnum);
}
ZoneRnum get_zone_rnum_by_zone_vnum(ZoneVnum zone) {
	return get_zone_rnum_by_room_vnum(zone * 100);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
