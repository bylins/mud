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
};

// Factory function for creating legacy data source
std::unique_ptr<IWorldDataSource> CreateLegacyDataSource();

} // namespace world_loader

#endif // LEGACY_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
