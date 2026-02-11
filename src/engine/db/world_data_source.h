// Part of Bylins http://www.mud.ru
// World data source interface for pluggable world loading

#ifndef WORLD_DATA_SOURCE_H_
#define WORLD_DATA_SOURCE_H_

#include <memory>
#include <string>

namespace world_loader
{

// Abstract interface for world data sources
// Allows different implementations (legacy files, YAML, database, etc.)
class IWorldDataSource
{
public:
	virtual ~IWorldDataSource() = default;

	// Returns the name/description of this data source for logging
	virtual std::string GetName() const = 0;

	// Load world data in the correct order
	// Each method populates the global data structures
	virtual void LoadZones() = 0;
	virtual void LoadTriggers() = 0;
	virtual void LoadRooms() = 0;
	virtual void LoadMobs() = 0;
	virtual void LoadObjects() = 0;

	// Save world data per zone (used by OLC)
	// zone_rnum is the runtime zone index
	// specific_vnum = -1 means save all entities in the zone
	// specific_vnum >= 0 means save only that specific entity
	// notify_level is the minimum level to receive mudlog notifications
	// Returns true on success, false on error
	virtual void SaveZone(int zone_rnum) = 0;
	virtual bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) = 0;
	virtual void SaveRooms(int zone_rnum, int specific_vnum = -1) = 0;
	virtual void SaveMobs(int zone_rnum, int specific_vnum = -1) = 0;
	virtual void SaveObjects(int zone_rnum, int specific_vnum = -1) = 0;
};

// Factory function type for creating data sources
using DataSourceFactory = std::unique_ptr<IWorldDataSource>(*)();

} // namespace world_loader

#endif // WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
