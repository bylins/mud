#include "id_converter.h"

#include "world_objects.h"
#include "zone.table.h"

zone_rnum get_zone_rnum_by_room_vnum(room_vnum vnum) {
	for (zone_rnum counter = 0; counter < static_cast<zone_rnum>(zone_table.size()); counter++)
		if ((vnum >= (zone_table[counter].number * 100)) && (vnum <= (zone_table[counter].top)))
			return counter;

	return -1;
}

// logic is the same as for room_vnum. keep this function for consistency
zone_rnum get_zone_rnum_by_obj_vnum(obj_vnum vnum) {
	return get_zone_rnum_by_room_vnum(vnum);
}

// logic is the same as for room_vnum. keep this function for consistency
zone_rnum get_zone_rnum_by_mob_vnum(mob_vnum vnum) {
	return get_zone_rnum_by_room_vnum(vnum);
}

zone_rnum get_zone_rnum_by_zone_vnum(zone_vnum zone) {
	return get_zone_rnum_by_room_vnum(zone * 100);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
