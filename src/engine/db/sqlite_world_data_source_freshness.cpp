// Part of Bylins http://www.mud.ru
// Freshness / membership bookkeeping for the SQLite world data source.
// Kept in a separate (ASCII-only) translation unit so the KOI8-R
// sqlite_world_data_source.cpp does not need touching.

#include "sqlite_world_data_source.h"

#ifdef HAVE_SQLITE

#include "engine/entities/zone.h"
#include "utils/logger.h"

#include <ctime>
#include <string>
#include <vector>

namespace world_loader
{

void SqliteWorldDataSource::EnsureSyncTables()
{
	if (!OpenDatabase())
	{
		return;
	}
	// IF NOT EXISTS so an older world.db (built before this feature, or by the
	// Python converter) keeps working -- it simply reports freshness 0 until the
	// first sync repopulates these tables.
	const char *ddl =
		"CREATE TABLE IF NOT EXISTS zone_sync ("
		"  zone_vnum INTEGER PRIMARY KEY,"
		"  sync_mtime INTEGER NOT NULL);"
		"CREATE TABLE IF NOT EXISTS world_meta ("
		"  key TEXT PRIMARY KEY,"
		"  value INTEGER NOT NULL);";
	char *err = nullptr;
	if (sqlite3_exec(m_db, ddl, nullptr, nullptr, &err) != SQLITE_OK)
	{
		log("SYSERR: SQLite EnsureSyncTables failed: %s", err ? err : "?");
		sqlite3_free(err);
	}
}

Freshness SqliteWorldDataSource::GetMetaFreshness(const char *key) const
{
	const_cast<SqliteWorldDataSource *>(this)->EnsureSyncTables();
	if (!m_db)
	{
		return 0;
	}
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, "SELECT value FROM world_meta WHERE key = ?", -1, &stmt, nullptr)
		!= SQLITE_OK)
	{
		return 0;
	}
	sqlite3_bind_text(stmt, 1, key, -1, SQLITE_TRANSIENT);
	Freshness value = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		value = static_cast<Freshness>(sqlite3_column_int64(stmt, 0));
	}
	sqlite3_finalize(stmt);
	return value;
}

std::vector<int> SqliteWorldDataSource::ListZoneVnums() const
{
	const_cast<SqliteWorldDataSource *>(this)->EnsureSyncTables();
	std::vector<int> zones;
	if (!m_db)
	{
		return zones;
	}
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, "SELECT vnum FROM zones", -1, &stmt, nullptr) != SQLITE_OK)
	{
		return zones;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		zones.push_back(sqlite3_column_int(stmt, 0));
	}
	sqlite3_finalize(stmt);
	return zones;
}

Freshness SqliteWorldDataSource::GetZoneFreshness(int zone_vnum) const
{
	const_cast<SqliteWorldDataSource *>(this)->EnsureSyncTables();
	if (!m_db)
	{
		return 0;
	}
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, "SELECT sync_mtime FROM zone_sync WHERE zone_vnum = ?", -1, &stmt,
			nullptr) != SQLITE_OK)
	{
		return 0;
	}
	sqlite3_bind_int(stmt, 1, zone_vnum);
	Freshness value = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		value = static_cast<Freshness>(sqlite3_column_int64(stmt, 0));
	}
	sqlite3_finalize(stmt);
	return value;
}

Freshness SqliteWorldDataSource::GetIndexFreshness() const
{
	// Membership stamp: bumped whenever a previously-unknown zone is first
	// written (see TouchZoneSync). Arbitrates zone add/remove between sources.
	return GetMetaFreshness("zone_index_mtime");
}

void SqliteWorldDataSource::TouchZoneSync(int zone_vnum, Freshness version)
{
	EnsureSyncTables();
	if (!m_db)
	{
		return;
	}

	// Is this zone already tracked? A first-time insert is a membership change
	// and bumps the index stamp too (to the same content version).
	bool existed = false;
	{
		sqlite3_stmt *q = nullptr;
		if (sqlite3_prepare_v2(m_db, "SELECT 1 FROM zone_sync WHERE zone_vnum = ?", -1, &q, nullptr)
			== SQLITE_OK)
		{
			sqlite3_bind_int(q, 1, zone_vnum);
			existed = (sqlite3_step(q) == SQLITE_ROW);
			sqlite3_finalize(q);
		}
	}

	{
		sqlite3_stmt *up = nullptr;
		if (sqlite3_prepare_v2(m_db,
				"REPLACE INTO zone_sync (zone_vnum, sync_mtime) VALUES (?, ?)", -1, &up, nullptr)
			== SQLITE_OK)
		{
			sqlite3_bind_int(up, 1, zone_vnum);
			sqlite3_bind_int64(up, 2, static_cast<sqlite3_int64>(version));
			sqlite3_step(up);
			sqlite3_finalize(up);
		}
	}

	if (!existed)
	{
		MarkIndexSynced(version);
	}
}

bool SqliteWorldDataSource::IsWritable() const
{
	const_cast<SqliteWorldDataSource *>(this)->EnsureSyncTables();
	if (!m_db)
	{
		return false;
	}
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(m_db,
			"SELECT 1 FROM sqlite_master WHERE type='table' AND name='zones'", -1, &stmt, nullptr)
		!= SQLITE_OK)
	{
		return false;
	}
	const bool has_schema = (sqlite3_step(stmt) == SQLITE_ROW);
	sqlite3_finalize(stmt);
	return has_schema;
}

void SqliteWorldDataSource::MarkZoneSynced(int zone_rnum, Freshness version)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		return;
	}
	TouchZoneSync(zone_table[zone_rnum].vnum, version);
}

void SqliteWorldDataSource::MarkIndexSynced(Freshness version)
{
	EnsureSyncTables();
	if (!m_db)
	{
		return;
	}
	sqlite3_stmt *mi = nullptr;
	if (sqlite3_prepare_v2(m_db,
			"REPLACE INTO world_meta (key, value) VALUES ('zone_index_mtime', ?)", -1, &mi, nullptr)
		== SQLITE_OK)
	{
		sqlite3_bind_int64(mi, 1, static_cast<sqlite3_int64>(version));
		sqlite3_step(mi);
		sqlite3_finalize(mi);
	}
}

} // namespace world_loader

#endif // HAVE_SQLITE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
