#ifndef BYLINS_SRC_ENGINE_DB_NULL_WORLD_DATA_SOURCE_H_
#define BYLINS_SRC_ENGINE_DB_NULL_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"

namespace world_loader {

/**
 * \brief Null Object implementation of IWorldDataSource.
 *
 * Used as a default fallback when no real data source is available.
 * All operations are no-ops.
 */
class NullWorldDataSource : public IWorldDataSource {
public:
	NullWorldDataSource() = default;
	~NullWorldDataSource() override = default;

	std::string GetName() const override { return "NullDataSource"; }

	void LoadZones() override {}
	void LoadTriggers() override {}
	void LoadRooms() override {}
	void LoadMobs() override {}
	void LoadObjects() override {}

	void SaveZone(int zone_rnum) override { (void)zone_rnum; }
	void SaveTriggers(int zone_rnum, int specific_vnum = -1) override { (void)zone_rnum; (void)specific_vnum; }
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override { (void)zone_rnum; (void)specific_vnum; }
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override { (void)zone_rnum; (void)specific_vnum; }
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override { (void)zone_rnum; (void)specific_vnum; }
};

} // namespace world_loader

#endif // BYLINS_SRC_ENGINE_DB_NULL_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
