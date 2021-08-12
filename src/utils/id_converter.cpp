#include "id_converter.h"

#include "global_objects.h"

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

// returns the real number of the room with given virtual number
room_rnum real_room(room_vnum vnum) {
	const auto it_room = std::lower_bound(world.begin(), world.end(), vnum, [](const ROOM_DATA *room, room_vnum value) {
		return room->number < value;
	});

	return (it_room == world.end() || (*it_room)->number != vnum) ?
		NOWHERE : std::distance(world.begin(), it_room);
}

// returns the real number of the monster with given virtual number
mob_rnum real_mobile(mob_vnum vnum) {
	mob_rnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	// perform binary search on mob-table
	for (;;) {
		mid = (bot + top) / 2;

		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
