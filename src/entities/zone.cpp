#include "zone.h"

#include "structs/global_objects.h"

struct ZoneCategory *zone_types = nullptr;

ZoneData::ZoneData() : traffic(0),
					   level(0),
					   type(0),
					   lifespan(0),
					   age(0),
					   top(0),
					   reset_mode(0),
					   vnum(0),
					   copy_from_zone(0),
					   cmd(nullptr),
					   typeA_count(0),
					   typeA_list(nullptr),
					   typeB_count(0),
					   typeB_list(nullptr),
					   typeB_flag(nullptr),
					   under_construction(0),
					   locked(false),
					   reset_idle(false),
					   used(false),
					   activity(0),
					   group(0),
					   mob_level(0),
					   is_town(false),
					   count_reset(0),
					   RnumTrigsLocation(-1, -1),
					   RnumRoomsLocation(-1, -1),
					   RnumMobsLocation(-1, -1),
					   RnumObjsLocation(-1, 1) {
}

ZoneData::~ZoneData() {
//	log("~ZoneData zone %d", vnum);
	if (!name.empty())
		name.clear();;
	if (!comment.empty())
		comment.clear();
	if (!author.empty())
		author.clear();
	if (!location.empty())
		location.clear();
	if (!description.empty())
		 description.clear();

	if (cmd)
		free(cmd);

	if (typeA_list)
		free(typeA_list);

	if (typeB_list)
		free(typeB_list);
	if (typeB_flag)
		free(typeB_flag);
}

ZoneTable &zone_table = GlobalObjects::zone_table();    // zone table

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
