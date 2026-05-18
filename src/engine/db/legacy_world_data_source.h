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
	LegacyWorldDataSource() = default;
	// target_dir, if non-empty, redirects Save* methods to write under
	// <target_dir>/world/{wld,mob,obj,zon,trg}/ instead of the default
	// "world/..." paths relative to cwd. Used by GameLoader::ResaveWorld
	// for cross-format round-trip diagnostics.
	explicit LegacyWorldDataSource(std::string target_dir);
	~LegacyWorldDataSource() override = default;

	std::string GetName() const override { return "Legacy file-based loader"; }

	void LoadZones() override;
	void LoadTriggers() override;
	void LoadRooms() override;
	void LoadMobs() override;
	void LoadObjects() override;

	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;

	// Scan <target_dir>/world/{wld,mob,obj,zon,trg}/ and regenerate the
	// per-subdir "index" files. The OLC save_to_disk routines write
	// individual <vnum>.<ext> files but do not maintain the boot indexes,
	// so a freshly resaved world is not bootable without this step.
	// No-op when target_dir is empty (in-place edits keep existing indexes).
	void RebuildIndexes();

private:
	std::string m_target_dir;
};

// Factory function for creating legacy data source
std::unique_ptr<IWorldDataSource> CreateLegacyDataSource();
std::unique_ptr<IWorldDataSource> CreateLegacyDataSource(std::string target_dir);

} // namespace world_loader

#endif // LEGACY_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
