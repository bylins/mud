// Part of Bylins http://www.mud.ru
// SQLite world data source implementation

#ifdef HAVE_SQLITE

#include "sqlite_world_data_source.h"
#include "db.h"
#include "utils/logger.h"

#include <cstring>

namespace world_loader
{

SqliteWorldDataSource::SqliteWorldDataSource(const std::string &db_path)
	: m_db_path(db_path)
	, m_db(nullptr)
{
}

SqliteWorldDataSource::~SqliteWorldDataSource()
{
	CloseDatabase();
}

bool SqliteWorldDataSource::OpenDatabase()
{
	if (m_db)
	{
		return true;
	}

	int rc = sqlite3_open_v2(m_db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Cannot open SQLite database '%s': %s", m_db_path.c_str(), sqlite3_errmsg(m_db));
		sqlite3_close(m_db);
		m_db = nullptr;
		return false;
	}

	log("Opened SQLite database: %s", m_db_path.c_str());
	return true;
}

void SqliteWorldDataSource::CloseDatabase()
{
	if (m_db)
	{
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

bool SqliteWorldDataSource::ExecuteQuery(const char *sql, int (*callback)(void*, int, char**, char**), void *data)
{
	char *err_msg = nullptr;
	int rc = sqlite3_exec(m_db, sql, callback, data, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: SQLite error: %s (query: %s)", err_msg, sql);
		sqlite3_free(err_msg);
		return false;
	}
	return true;
}

void SqliteWorldDataSource::LoadZones()
{
	log("Loading zones from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for zone loading.");
		return;
	}

	// TODO: Implement zone loading from SQLite
	// For now, fall back to legacy loading
	log("SYSERR: SQLite zone loading not yet implemented. Use legacy loader.");
}

void SqliteWorldDataSource::LoadTriggers()
{
	log("Loading triggers from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for trigger loading.");
		return;
	}

	// TODO: Implement trigger loading from SQLite
	log("SYSERR: SQLite trigger loading not yet implemented. Use legacy loader.");
}

void SqliteWorldDataSource::LoadRooms()
{
	log("Loading rooms from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for room loading.");
		return;
	}

	// TODO: Implement room loading from SQLite
	log("SYSERR: SQLite room loading not yet implemented. Use legacy loader.");
}

void SqliteWorldDataSource::LoadMobs()
{
	log("Loading mobs from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for mob loading.");
		return;
	}

	// TODO: Implement mob loading from SQLite
	log("SYSERR: SQLite mob loading not yet implemented. Use legacy loader.");
}

void SqliteWorldDataSource::LoadObjects()
{
	log("Loading objects from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for object loading.");
		return;
	}

	// TODO: Implement object loading from SQLite
	log("SYSERR: SQLite object loading not yet implemented. Use legacy loader.");
}

// Save methods - SQLite is read-only for now
void SqliteWorldDataSource::SaveZone(int)
{
	log("SYSERR: SQLite data source does not support saving zones.");
}

void SqliteWorldDataSource::SaveTriggers(int)
{
	log("SYSERR: SQLite data source does not support saving triggers.");
}

void SqliteWorldDataSource::SaveRooms(int)
{
	log("SYSERR: SQLite data source does not support saving rooms.");
}

void SqliteWorldDataSource::SaveMobs(int)
{
	log("SYSERR: SQLite data source does not support saving mobs.");
}

void SqliteWorldDataSource::SaveObjects(int)
{
	log("SYSERR: SQLite data source does not support saving objects.");
}

std::unique_ptr<IWorldDataSource> CreateSqliteDataSource(const std::string &db_path)
{
	return std::make_unique<SqliteWorldDataSource>(db_path);
}

} // namespace world_loader

#endif // HAVE_SQLITE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
