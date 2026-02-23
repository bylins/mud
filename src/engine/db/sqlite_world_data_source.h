// Part of Bylins http://www.mud.ru
// SQLite world data source - loads world from SQLite database

#ifndef SQLITE_WORLD_DATA_SOURCE_H_
#define SQLITE_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"
#include "world_data_source_base.h"

#ifdef HAVE_SQLITE

#include <sqlite3.h>
#include <string>
#include <map>

class ZoneData;
class RoomData;
class CharData;
class CObjectPrototype;
class Trigger;
struct reset_com;

namespace world_loader
{

// SQLite implementation for fast world loading
class SqliteWorldDataSource : public WorldDataSourceBase
{
public:
	explicit SqliteWorldDataSource(const std::string &db_path);
	~SqliteWorldDataSource() override;

	std::string GetName() const override { return "SQLite database: " + m_db_path; }

	void LoadZones() override;
	void LoadTriggers() override;
	void LoadRooms() override;
	void LoadMobs() override;
	void LoadObjects() override;

	// Save methods (SQLite is read-only for now)
	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;

private:
	bool OpenDatabase();
	void CloseDatabase();
	int GetCount(const char *table);
	std::string GetText(sqlite3_stmt *stmt, int col);

	// Zone loading helpers
	void LoadZoneCommands(ZoneData &zone);
	void LoadZoneGroups(ZoneData &zone);

	// Room loading helpers
	void LoadRoomExits(const std::map<int, int> &vnum_to_rnum);
	void LoadRoomFlags(const std::map<int, int> &vnum_to_rnum);
	void LoadRoomTriggers(const std::map<int, int> &vnum_to_rnum);
	void LoadRoomExtraDescriptions(const std::map<int, int> &vnum_to_rnum);

	// Mob loading helpers
	void LoadMobFlags();
	void LoadMobSkills();
	void LoadMobTriggers();
	void LoadMobResistances();
	void LoadMobSaves();
	void LoadMobFeats();
	void LoadMobSpells();
	void LoadMobHelpers();
	void LoadMobDestinations();

	// Object loading helpers
	void LoadObjectFlags();
	void LoadObjectApplies();
	void LoadObjectTriggers();
	void LoadObjectExtraDescriptions();

	// Transaction helpers
	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();
	bool ExecuteStatement(const std::string &sql, const std::string &operation);

	// Save helpers
	void SaveZoneRecord(const ZoneData &zone);
	void SaveZoneCommands(int zone_vnum, const struct reset_com *commands);
	void SaveZoneGroups(int zone_vnum, const ZoneData &zone);
	void SaveRoomRecord(RoomData *room);
	void SaveTriggerRecord(int trig_vnum, const Trigger *trig);
	void SaveMobRecord(int mob_vnum, CharData &mob);
	void SaveObjectRecord(int obj_vnum, CObjectPrototype *obj);

	std::string m_db_path;
	sqlite3 *m_db;
};

// Factory function for creating SQLite data source
std::unique_ptr<IWorldDataSource> CreateSqliteDataSource(const std::string &db_path);

} // namespace world_loader

#endif // HAVE_SQLITE

#endif // SQLITE_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
