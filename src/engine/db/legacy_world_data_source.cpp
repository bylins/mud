// Part of Bylins http://www.mud.ru
// Legacy world data source implementation

#include "legacy_world_data_source.h"
#include "db.h"
#include "utils/logger.h"

// Forward declarations for OLC save functions
void zedit_save_to_disk(int zone_rnum);
void redit_save_to_disk(int zone_rnum);
void medit_save_to_disk(int zone_rnum);
void oedit_save_to_disk(int zone_rnum);
void trigedit_save_to_disk(int zone_rnum);

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

void LegacyWorldDataSource::SaveTriggers(int zone_rnum)
{
	trigedit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveRooms(int zone_rnum)
{
	redit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveMobs(int zone_rnum)
{
	medit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveObjects(int zone_rnum)
{
	oedit_save_to_disk(zone_rnum);
}

std::unique_ptr<IWorldDataSource> CreateLegacyDataSource()
{
	return std::make_unique<LegacyWorldDataSource>();
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
