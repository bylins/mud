// Part of Bylins http://www.mud.ru
// Base class for world data sources with shared code

#ifndef WORLD_DATA_SOURCE_BASE_H_
#define WORLD_DATA_SOURCE_BASE_H_

#include "world_data_source.h"
#include "engine/scripting/dg_scripts.h"

#include <string>
#include <memory>

class ZoneData;
struct IndexData;

namespace world_loader
{

// Base class providing common functionality for world loaders
// Extracted to eliminate code duplication between YAML/SQLite loaders
class WorldDataSourceBase : public IWorldDataSource
{
public:
	// Assign triggers from proto_script to actual room scripts
	// Call after all rooms are loaded - common post-processing step
	static void AssignTriggersToLoadedRooms();

protected:
	// Parse trigger script from string into cmdlist
	// Used by both YAML and SQLite loaders - 100% identical code
	static void ParseTriggerScript(Trigger *trig, const std::string &script);

	// Initialize zone runtime fields to defaults
	// Nearly identical between loaders (only under_construction differs)
	static void InitializeZoneRuntimeFields(ZoneData &zone, int under_construction = 0);

	// Create trigger index entry
	// Used by both YAML and SQLite loaders - 100% identical code
	static IndexData* CreateTriggerIndex(int vnum, Trigger *trig);

	// Attach trigger to room/mob/object
	// Common logic for all loaders - checks trigger exists and adds to proto_script
	static void AttachTriggerToRoom(RoomRnum room_rnum, int trigger_vnum, RoomVnum room_vnum);
	static void AttachTriggerToMob(MobRnum mob_rnum, int trigger_vnum, MobVnum mob_vnum);
	static void AttachTriggerToObject(ObjRnum obj_rnum, int trigger_vnum, ObjVnum obj_vnum);

};

} // namespace world_loader

#endif // WORLD_DATA_SOURCE_BASE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
