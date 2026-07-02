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

	// Parse trigger script from string into cmdlist
	// Used by both YAML and SQLite loaders - 100% identical code
	// Public so unit tests can verify parsing behavior (line numbering, etc.)
	static void ParseTriggerScript(Trigger *trig, const std::string &script);

protected:
	// Initialize zone runtime fields to defaults
	// Nearly identical between loaders (only under_construction differs)
	static void InitializeZoneRuntimeFields(ZoneData &zone, int under_construction = 0);

	// Create trigger index entry
	// Used by both YAML and SQLite loaders - 100% identical code
	static IndexData* CreateTriggerIndex(int vnum, Trigger *trig);

	// Append one mob_index[] entry at rnum=top_of_mobt and advance top_of_mobt.
	// Mirrors CreateTriggerIndex's bookkeeping role for mobs -- the CharData
	// (mob_proto[]) field population itself stays in each backend (too large
	// and backend-specific to share); the caller fills mob_proto[<returned
	// rnum>] and calls CharData::set_rnum(<returned rnum>) itself. Used by the
	// per-zone LoadZoneMobs() implementations, where top_of_mobt/mob_proto/
	// mob_index have already been pre-allocated and reset by the boot
	// orchestrator (see db.cpp's per-zone boot path) before any zone is
	// loaded, so this can be called once per mob across an arbitrary number of
	// LoadZoneMobs() calls without re-allocating.
	static MobRnum AppendMobIndex(MobVnum vnum);

	// Attach trigger to room/mob/object
	// Common logic for all loaders - checks trigger exists and adds to proto_script
	static void AttachTriggerToRoom(RoomRnum room_rnum, int trigger_vnum, RoomVnum room_vnum);
	static void AttachTriggerToMob(MobRnum mob_rnum, int trigger_vnum, MobVnum mob_vnum);
	static void AttachTriggerToObject(ObjRnum obj_rnum, int trigger_vnum, ObjVnum obj_vnum);

};

} // namespace world_loader

#endif // WORLD_DATA_SOURCE_BASE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
