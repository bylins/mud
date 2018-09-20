// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

// комментарий на русском в надежде починить кодировки bitbucket

#include "room.hpp"

#include "dg_scripts.h"

ROOM_DATA::ROOM_DATA()
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
	proto_script(new OBJ_DATA::triggers_list_t()),
	script(new SCRIPT_DATA()),
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
		dir_option[i].reset();
	}
	memset(&weather, 0, sizeof(weather_control));
	memset(&base_property, 0, sizeof(room_property_data));
	memset(&add_property, 0, sizeof(room_property_data));
}

CHAR_DATA* ROOM_DATA::first_character() const
{
	CHAR_DATA* first = people.empty() ? nullptr : *people.begin();

	return first;
}

void ROOM_DATA::cleanup_script()
{
	script->cleanup();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
