// Part of Bylins http://www.mud.ru
// SQLite world data source - loads world from SQLite database

#ifndef SQLITE_WORLD_DATA_SOURCE_H_
#define SQLITE_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"

#ifdef HAVE_SQLITE

#include <sqlite3.h>
#include <string>

namespace world_loader
{

// SQLite implementation for fast world loading
class SqliteWorldDataSource : public IWorldDataSource
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
	void SaveTriggers(int zone_rnum) override;
	void SaveRooms(int zone_rnum) override;
	void SaveMobs(int zone_rnum) override;
	void SaveObjects(int zone_rnum) override;

private:
	bool OpenDatabase();
	void CloseDatabase();
	bool ExecuteQuery(const char *sql, int (*callback)(void*, int, char**, char**), void *data);

	std::string m_db_path;
	sqlite3 *m_db;
};

// Factory function for creating SQLite data source
std::unique_ptr<IWorldDataSource> CreateSqliteDataSource(const std::string &db_path);

} // namespace world_loader

#endif // HAVE_SQLITE

#endif // SQLITE_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
