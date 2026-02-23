// Part of Bylins http://www.mud.ru
// Legacy world data source implementation

#include "legacy_world_data_source.h"
#include "db.h"
#include "obj_prototypes.h"
#include "utils/logger.h"

// Forward declarations for OLC save functions
void zedit_save_to_disk(int zone_rnum);
void redit_save_to_disk(int zone_rnum);
void medit_save_to_disk(int zone_rnum);
void oedit_save_to_disk(int zone_rnum);
bool trigedit_save_to_disk(int zone_rnum, int notify_level);

namespace world_loader
{

void LegacyWorldDataSource::LoadZones()
{
	log("Loading zone table.");
	GameLoader::BootIndex(DB_BOOT_ZON);
}

void LegacyWorldDataSource::LoadTriggers()
{
	log("Loading triggers and generating index.");
	GameLoader::BootIndex(DB_BOOT_TRG);
}

void LegacyWorldDataSource::LoadRooms()
{
	log("Loading rooms.");
	GameLoader::BootIndex(DB_BOOT_WLD);
}

void LegacyWorldDataSource::LoadMobs()
{
	log("Loading mobs and generating index.");
	GameLoader::BootIndex(DB_BOOT_MOB);
}

void LegacyWorldDataSource::LoadObjects()
{
	log("Loading objs and generating index.");
	GameLoader::BootIndex(DB_BOOT_OBJ);
}

void LegacyWorldDataSource::SaveZone(int zone_rnum)
{
	zedit_save_to_disk(zone_rnum);
}

bool LegacyWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	return trigedit_save_to_disk(zone_rnum, notify_level);
}

void LegacyWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	redit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	medit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	oedit_save_to_disk(zone_rnum);
}

std::unique_ptr<IWorldDataSource> CreateLegacyDataSource()
{
	return std::make_unique<LegacyWorldDataSource>();
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
