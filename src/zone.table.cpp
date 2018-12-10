#include "zone.table.hpp"

#include "global.objects.hpp"

ZoneData::ZoneData() : name(nullptr),
	comment(nullptr),
	author(nullptr),
	traffic(0),
	level(0),
	type(0),
	lifespan(0),
	age(0),
	top(0),
	reset_mode(0),
	number(0),
	location(nullptr),
	description(nullptr),
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
	count_reset(0)
{
}

ZoneData::~ZoneData()
{
	log("~ZoneData zone %d", number);
	if (name)
		free(name);
	if (comment)
		free(comment);
	if (author)
		free(author);
	if (location)
		free(location);
	if (description)
		free(description);
	
	if (cmd)
		free(cmd);

	if (typeA_list)
		free(typeA_list);

	if (typeB_list)
		free(typeB_list);
	if (typeB_flag)
		free(typeB_flag);
}

zone_table_t& zone_table = GlobalObjects::zone_table();	// zone table

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
