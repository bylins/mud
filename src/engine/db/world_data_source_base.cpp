// Part of Bylins http://www.mud.ru
// Base class for world data sources - implementation

#include "world_data_source_base.h"
#include "db.h"
#include "obj_prototypes.h"
#include "utils/utils.h"
#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include "engine/entities/room_data.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/scripting/dg_db_scripts.h"
#include "engine/scripting/dg_olc.h"

#include <sstream>
#include <string>

// External declarations
extern IndexData **trig_index;
extern int top_of_trigt;
extern CharData *mob_proto;
extern void ExtractTrigger(Trigger *trig);

namespace world_loader

{

// Parse trigger script from string into cmdlist
// Extracted from YAML/SQLite loaders (100% identical code)
void WorldDataSourceBase::ParseTriggerScript(Trigger *trig, const std::string &script)
{
	if (!script.empty())
	{
		std::istringstream ss(script);
		int indlev = 0;
		int line_num = 1;
		std::string line;
		cmdlist_element::shared_ptr head = nullptr;
		cmdlist_element::shared_ptr tail = nullptr;

		while (std::getline(ss, line))
		{
			// Skip empty lines BEFORE trim (same as Legacy loader)
			// Lines with whitespace are not considered empty - they become "" after trim
			if (line.empty() || line == "\r")
			{
				continue;
			}

			utils::TrimRight(line);
			auto cmd = std::make_shared<cmdlist_element>();
			indent_trigger(line, &indlev);
			cmd->cmd = line;
			cmd->line_num = line_num++;
			cmd->next = nullptr;

			// lowercase the command (first word) for faster comparison at runtime
			auto it = cmd->cmd.begin();
			while (it != cmd->cmd.end() && (*it == ' ' || *it == '\t')) ++it;
			while (it != cmd->cmd.end() && *it != ' ') { *it = LOWER(*it); ++it; }

			if (!head)
			{
				head = cmd;
				tail = cmd;
			}
			else
			{
				tail->next = cmd;
				tail = cmd;
			}
		}

		trig->cmdlist = std::make_shared<cmdlist_element::shared_ptr>(head);
	}
}

// Initialize zone runtime fields to defaults
// Extracted from YAML/SQLite loaders (nearly identical code)
void WorldDataSourceBase::InitializeZoneRuntimeFields(ZoneData &zone, int under_construction)
{
	zone.age = 0;
	zone.time_awake = 0;
	zone.traffic = 0;
	zone.under_construction = under_construction;
	zone.locked = false;
	zone.used = false;
	zone.activity = 0;
	zone.mob_level = 0;
	zone.is_town = false;
	zone.count_reset = 0;
	zone.typeA_count = 0;
	zone.typeA_list = nullptr;
	zone.typeB_count = 0;
	zone.typeB_list = nullptr;
	zone.typeB_flag = nullptr;
	zone.cmd = nullptr;
	zone.RnumTrigsLocation.first = -1;
	zone.RnumTrigsLocation.second = -1;
	zone.RnumMobsLocation.first = -1;
	zone.RnumMobsLocation.second = -1;
	zone.RnumRoomsLocation.first = -1;
	zone.RnumRoomsLocation.second = -1;
}

// Create trigger index entry
// Extracted from YAML/SQLite loaders (100% identical code)
IndexData* WorldDataSourceBase::CreateTriggerIndex(int vnum, Trigger *trig)
{
	IndexData *index;
	CREATE(index, 1);
	index->vnum = vnum;
	index->total_online = 0;
	index->func = nullptr;
	index->proto = trig;

	trig_index[top_of_trigt++] = index;
	return index;
}

// Attach trigger to room
// Common logic: create proto_script if needed, validate trigger exists, add vnum
void WorldDataSourceBase::AttachTriggerToRoom(RoomRnum room_rnum, int trigger_vnum, RoomVnum room_vnum)
{
	if (!world[room_rnum]->proto_script)
	{
		world[room_rnum]->proto_script = std::make_shared<ObjData::triggers_list_t>();
	}

	const auto trigger_rnum = GetTriggerRnum(trigger_vnum);
	if (trigger_rnum < 0)
	{
		log("SYSERR: Room %d references non-existent trigger %d, skipping.", room_vnum, trigger_vnum);
		return;
	}
	world[room_rnum]->proto_script->push_back(trigger_vnum);

	// п■п╩я▐ п╨п╬п╪п╫п╟я┌, п╡ п╬я┌п╩п╦я┤п╦п╣ п╬я┌ п╪п╬п╠п╬п╡ п╦ п╬п╠я┼п╣п╨я┌п╬п╡, п╫п╣я┌ "п╪п╬п╪п╣п╫я┌п╟ п╦п╫я│я┌п╟п╫я├п╦п╟я├п╦п╦" -
	// я│п╟п╪п╟ RoomData п╦ п╣я│я┌я▄ я─п╟п╫я┌п╟п╧п╪-я│я┐я┴п╫п╬я│я┌я▄. п÷п╬я█я┌п╬п╪я┐ п©п╬п╪п╦п╪п╬ proto_script п╫п╟п╢п╬
	// я│я─п╟п╥я┐ я│п╬п╥п╢п╟я┌я▄ я─п╟п╫я┌п╟п╧п╪-п╦п╫я│я┌п╟п╫я│ я┌я─п╦пЁпЁп╣я─п╟ п╦ п©п╬п╩п╬п╤п╦я┌я▄ п╡ SCRIPT(room),
	// п╦п╫п╟я┤п╣ reset_wtrigger / random_wtrigger п╫п╦п╨п╬пЁп╢п╟ п╫п╣ п╫п╟п╧п╢я┐я┌ п╣пЁп╬ п╡
	// script_trig_list. п╜я┌п╬ я│п╬п╡п©п╟п╢п╟п╣я┌ я│ я┌п╣п╪, я┤я┌п╬ п╢п╣п╩п╟п╣я┌ п╩п╣пЁп╟я│п╫я▀п╧
	// dg_read_trigger п╡ boot_data_files.cpp:221.
	auto *trigger_instance = read_trigger(trigger_rnum);
	if (add_trigger(SCRIPT(world[room_rnum]).get(), trigger_instance, -1))
	{
		add_trig_to_owner(-1, trigger_vnum, room_vnum);
	}
	else
	{
		ExtractTrigger(trigger_instance);
	}
}

// Attach trigger to mob
// Common logic: create proto_script if needed, validate trigger exists, add vnum
void WorldDataSourceBase::AttachTriggerToMob(MobRnum mob_rnum, int trigger_vnum, MobVnum mob_vnum)
{
	if (!mob_proto[mob_rnum].proto_script)
	{
		mob_proto[mob_rnum].proto_script = std::make_shared<ObjData::triggers_list_t>();
	}

	if (GetTriggerRnum(trigger_vnum) >= 0)
	{
		mob_proto[mob_rnum].proto_script->push_back(trigger_vnum);
	}
	else
	{
		log("SYSERR: Mob %d references non-existent trigger %d, skipping.", mob_vnum, trigger_vnum);
	}
}

// Attach trigger to object
// Common logic: validate trigger exists and add vnum
// Note: proto_script is always initialized in CObjectPrototype constructor
void WorldDataSourceBase::AttachTriggerToObject(ObjRnum obj_rnum, int trigger_vnum, ObjVnum obj_vnum)
{
	if (GetTriggerRnum(trigger_vnum) >= 0)
	{
		obj_proto.at(obj_rnum)->add_proto_script(trigger_vnum);
	}
	else
	{
		log("SYSERR: Object %d references non-existent trigger %d, skipping.", obj_vnum, trigger_vnum);
	}
}


// Assign triggers to all loaded rooms
// Common post-processing step for all loaders
void WorldDataSourceBase::AssignTriggersToLoadedRooms()
{
	log("Assigning triggers to rooms...");
	int assigned_count = 0;

	for (RoomRnum rnum = kFirstRoom; rnum <= top_of_world; ++rnum)
	{
		if (world[rnum]->proto_script && !world[rnum]->proto_script->empty())
		{
			assign_triggers(world[rnum], WLD_TRIGGER);
			++assigned_count;
		}
	}

	if (assigned_count > 0)
	{
		log("  Assigned triggers to %d rooms.", assigned_count);
	}
}
} // namespace world_loader



// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
