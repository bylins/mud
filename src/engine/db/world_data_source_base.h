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

	// Create trigger index entry (places it at trig_index[top_of_trigt++]).
	// Used by both YAML and SQLite loaders, and by the boot orchestrator
	// (db.cpp), which is the single place that now places every loaded
	// trigger -- regardless of which source(s) supplied them -- into
	// trig_index. Public so that orchestrator can call it directly (it isn't
	// a subclass of WorldDataSourceBase).
	static IndexData* CreateTriggerIndex(int vnum, Trigger *trig);

	// Append one mob_index[] entry at rnum=top_of_mobt and advance top_of_mobt.
	// Mirrors CreateTriggerIndex's bookkeeping role for mobs -- the CharData
	// (mob_proto[]) field population itself stays with the caller. Used by
	// the boot orchestrator (db.cpp), which places every loaded mob --
	// regardless of which source(s) supplied them -- into mob_proto[]/
	// mob_index[] once top_of_mobt/mob_proto/mob_index have been pre-sized.
	static MobRnum AppendMobIndex(MobVnum vnum);

	// Attach trigger to room/mob/object. Common logic for all loaders --
	// checks the trigger exists and adds it to proto_script. Public so the
	// boot orchestrator (db.cpp) can call it once entities from every source
	// are placed in their final tables (world[]/mob_proto[]/obj_proto).
	static void AttachTriggerToRoom(RoomRnum room_rnum, int trigger_vnum, RoomVnum room_vnum);
	static void AttachTriggerToMob(MobRnum mob_rnum, int trigger_vnum, MobVnum mob_vnum);
	static void AttachTriggerToObject(ObjRnum obj_rnum, int trigger_vnum, ObjVnum obj_vnum);

protected:
	// Initialize zone runtime fields to defaults
	// Nearly identical between loaders (only under_construction differs)
	static void InitializeZoneRuntimeFields(ZoneData &zone, int under_construction = 0);
};

} // namespace world_loader

#endif // WORLD_DATA_SOURCE_BASE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
