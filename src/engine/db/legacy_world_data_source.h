// Part of Bylins http://www.mud.ru
// Legacy world data source - wraps existing file-based loading

#ifndef LEGACY_WORLD_DATA_SOURCE_H_
#define LEGACY_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"

namespace world_loader
{

// Legacy implementation that uses the existing file-based loading
// This wraps the current BootIndex() calls to provide the IWorldDataSource interface
class LegacyWorldDataSource : public IWorldDataSource
{
public:
	// world_dir is the directory the world lives in: Load* read from it and
	// Save* write to it (both via "world/{wld,mob,obj,zon,trg}/" relative to
	// it). Empty (the default) means the current working directory -- the
	// running server's mode, where cwd is the data dir after the main
	// chdir(-d). A non-empty value is used by GameLoader::ResaveWorld to
	// point a save-only instance at an export directory.
	explicit LegacyWorldDataSource(std::string world_dir = "");
	~LegacyWorldDataSource() override = default;

	std::string GetName() const override { return "Legacy file-based loader"; }

	// Legacy's BootIndex() is the original CircleMUD loader: it parses and
	// places entities directly into trig_index/world/mob_proto+mob_index/
	// obj_proto itself (and attaches triggers), so these always return an
	// empty vector -- SupportsZoneFilter() being false tells the boot
	// orchestrator (db.cpp) not to expect anything to place and to leave
	// what BootIndex just built alone. Never mixed into a composite in
	// practice (full-world-archive-only), so zone_filter is ignored.
	void LoadZones() override;
	std::vector<LoadedTrigger> LoadTriggers(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedRoom> LoadRooms(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedMob> LoadMobs(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedObject> LoadObjects(const std::vector<int> *zone_filter = nullptr) override;

	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;

	// Regenerate the per-subdir "index" files under world_dir/world/*. The
	// OLC save_to_disk routines write individual <vnum>.<ext> files but do
	// not maintain the boot indexes, so a freshly resaved tree is not
	// bootable without this step. No-op for the in-place (empty world_dir)
	// server mode.
	void FinalizeResave() override;

private:
	std::string m_world_dir;
};

// Factory for the legacy data source. world_dir defaults to the current
// working directory (the running server's mode); ResaveWorld passes an
// explicit export directory, symmetric with the YAML/SQLite factories.
std::unique_ptr<IWorldDataSource> CreateLegacyDataSource(std::string world_dir = "");

} // namespace world_loader

#endif // LEGACY_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
