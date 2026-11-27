// Part of Bylins http://www.mud.ru
// Legacy world data source implementation

#include "legacy_world_data_source.h"
#include "db.h"
#include "obj_prototypes.h"
#include "engine/olc/olc.h"
#include "engine/scripting/dg_olc.h"
#include "utils/logger.h"

#include <utility>

namespace world_loader
{

LegacyWorldDataSource::LegacyWorldDataSource(std::filesystem::path world_dir, bool rebuild_indexes)
	: m_files(LegacyFiles::World(std::move(world_dir))),
	m_rebuild_indexes(rebuild_indexes)
{
}

void LegacyWorldDataSource::LoadZones()
{
	log("Loading zone table.");
	GameLoader::BootIndex(DB_BOOT_ZON, m_files);
}

std::vector<LoadedTrigger> LegacyWorldDataSource::LoadTriggers(const std::vector<int> * /*zone_filter*/)
{
	log("Loading triggers and generating index.");
	GameLoader::BootIndex(DB_BOOT_TRG, m_files);
	return {};
}

std::vector<LoadedRoom> LegacyWorldDataSource::LoadRooms(const std::vector<int> * /*zone_filter*/)
{
	log("Loading rooms.");
	GameLoader::BootIndex(DB_BOOT_WLD, m_files);
	return {};
}

std::vector<LoadedMob> LegacyWorldDataSource::LoadMobs(const std::vector<int> * /*zone_filter*/)
{
	log("Loading mobs and generating index.");
	GameLoader::BootIndex(DB_BOOT_MOB, m_files);
	return {};
}

std::vector<LoadedObject> LegacyWorldDataSource::LoadObjects(const std::vector<int> * /*zone_filter*/)
{
	log("Loading objs and generating index.");
	GameLoader::BootIndex(DB_BOOT_OBJ, m_files);
	return {};
}

void LegacyWorldDataSource::SaveZone(int zone_rnum)
{
	m_files.EnsureLayout();
	zedit_save_to_disk(zone_rnum, m_files);
}

bool LegacyWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	m_files.EnsureLayout();
	return trigedit_save_to_disk(zone_rnum, notify_level, m_files);
}

void LegacyWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	m_files.EnsureLayout();
	redit_save_to_disk(zone_rnum, m_files);
}

void LegacyWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	m_files.EnsureLayout();
	medit_save_to_disk(zone_rnum, m_files);
}

void LegacyWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	m_files.EnsureLayout();
	oedit_save_to_disk(zone_rnum, m_files);
}

void LegacyWorldDataSource::FinalizeResave()
{
	// The OLC save_to_disk routines write individual <vnum>.<ext> files but
	// do not maintain the boot indexes. After a full resave regenerate them
	// so the resulting tree is bootable. The server's own in-place saves
	// must not rewrite the live indexes, so only a resave instance does it.
	if (!m_rebuild_indexes) {
		return;
	}
	for (const auto kind : {DB_BOOT_WLD, DB_BOOT_MOB, DB_BOOT_OBJ, DB_BOOT_ZON, DB_BOOT_TRG}) {
		m_files.RebuildIndex(kind);
	}
}

std::unique_ptr<IWorldDataSource> CreateLegacyDataSource(std::filesystem::path world_dir, bool rebuild_indexes)
{
	return std::make_unique<LegacyWorldDataSource>(std::move(world_dir), rebuild_indexes);
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
