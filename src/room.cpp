// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

// комментарий на русском в надежде починить кодировки bitbucket

#include "room.hpp"

room_data::room_data()
	: number(0),
	zone(0),
	sector_type(0),
	sector_state(0),
	name(0),
	description_num(0),
	temp_description(0),
	ex_description(0),
	light(0),
	glight(0),
	gdark(0),
	proto_script(0),
	script(0),
	track(0),
	contents(0),
	people(0),
	affected(0),
	fires(0),
	ices(0),
	portal_room(0),
	portal_time(0),
	pkPenterUnique(0),
	holes(0),
	ing_list(0),
	poison(0)
{
	for (int i = 0; i < NUM_OF_DIRS; ++i)
	{
		dir_option[i] = 0;
	}
	memset(&room_flags, 0, sizeof(FLAG_DATA));
	memset(&weather, 0, sizeof(weather_control));
	memset(&affected_by, 0, sizeof(FLAG_DATA));
	memset(&base_property, 0, sizeof(room_property_data));
	memset(&add_property, 0, sizeof(room_property_data));
}
