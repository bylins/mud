#include "zone.h"

#include "engine/db/global_objects.h"

struct ZoneCategory *zone_types = nullptr;

ZoneData::ZoneData() : traffic(0),
					   level(0),
					   type(0),
					   lifespan(0),
					   age(0),
					   time_awake(0),
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
					   entrance(0),
					   RnumTrigsLocation(-1, -1),
					   RnumRoomsLocation(-1, -1),
					   RnumMobsLocation(-1, -1) {
}

ZoneData::ZoneData(ZoneData&& other) noexcept
	: name(std::move(other.name)),
	  comment(std::move(other.comment)),
	  author(std::move(other.author)),
	  traffic(other.traffic),
	  level(other.level),
	  type(other.type),
	  first_enter(std::move(other.first_enter)),
	  lifespan(other.lifespan),
	  age(other.age),
	  time_awake(other.time_awake),
	  top(other.top),
	  reset_mode(other.reset_mode),
	  vnum(other.vnum),
	  copy_from_zone(other.copy_from_zone),
	  location(std::move(other.location)),
	  description(std::move(other.description)),
	  cmd(other.cmd),
	  typeA_count(other.typeA_count),
	  typeA_list(other.typeA_list),
	  typeB_count(other.typeB_count),
	  typeB_list(other.typeB_list),
	  typeB_flag(other.typeB_flag),
	  under_construction(other.under_construction),
	  locked(other.locked),
	  reset_idle(other.reset_idle),
	  used(other.used),
	  activity(other.activity),
	  group(other.group),
	  mob_level(other.mob_level),
	  is_town(other.is_town),
	  count_reset(other.count_reset),
	  entrance(other.entrance),
	  RnumTrigsLocation(other.RnumTrigsLocation),
	  RnumRoomsLocation(other.RnumRoomsLocation),
	  RnumMobsLocation(other.RnumMobsLocation)
{
	other.cmd = nullptr;
	other.typeA_list = nullptr;
	other.typeB_list = nullptr;
	other.typeB_flag = nullptr;
}

ZoneData& ZoneData::operator=(ZoneData&& other) noexcept
{
	if (this != &other)
	{
		if (cmd) free(cmd);
		if (typeA_list) free(typeA_list);
		if (typeB_list) free(typeB_list);
		if (typeB_flag) free(typeB_flag);

		name = std::move(other.name);
		comment = std::move(other.comment);
		author = std::move(other.author);
		traffic = other.traffic;
		level = other.level;
		type = other.type;
		first_enter = std::move(other.first_enter);
		lifespan = other.lifespan;
		age = other.age;
		time_awake = other.time_awake;
		top = other.top;
		reset_mode = other.reset_mode;
		vnum = other.vnum;
		copy_from_zone = other.copy_from_zone;
		location = std::move(other.location);
		description = std::move(other.description);
		cmd = other.cmd;
		typeA_count = other.typeA_count;
		typeA_list = other.typeA_list;
		typeB_count = other.typeB_count;
		typeB_list = other.typeB_list;
		typeB_flag = other.typeB_flag;
		under_construction = other.under_construction;
		locked = other.locked;
		reset_idle = other.reset_idle;
		used = other.used;
		activity = other.activity;
		group = other.group;
		mob_level = other.mob_level;
		is_town = other.is_town;
		count_reset = other.count_reset;
		entrance = other.entrance;
		RnumTrigsLocation = other.RnumTrigsLocation;
		RnumRoomsLocation = other.RnumRoomsLocation;
		RnumMobsLocation = other.RnumMobsLocation;

		other.cmd = nullptr;
		other.typeA_list = nullptr;
		other.typeB_list = nullptr;
		other.typeB_flag = nullptr;
	}
	return *this;
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

ZoneVnum GetZoneVnumByCharPlace(CharData *ch) {
	return zone_table[world[ch->in_room]->zone_rn].vnum;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
