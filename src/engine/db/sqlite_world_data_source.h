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
#include <set>

class ZoneData;
struct RoomData;
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
	std::string GetKind() const override { return "sqlite"; }

	void LoadZones() override;
	std::vector<LoadedTrigger> LoadTriggers(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedRoom> LoadRooms(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedMob> LoadMobs(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedObject> LoadObjects(const std::vector<int> *zone_filter = nullptr) override;

	// Save methods (SQLite is read-only for now)
	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;

	// Freshness / membership (for CompositeWorldDataSource). Freshness is the
	// wall-clock time (unix seconds) at which each zone was last written to the
	// database -- recorded in the zone_sync table. Because a freshly synced
	// zone gets "now" (>= the source mtime it was built from), SQLite naturally
	// wins the freshness comparison until the YAML source is edited again.
	std::vector<int> ListZoneVnums() const override;
	Freshness GetZoneFreshness(int zone_vnum) const override;
	Freshness GetIndexFreshness() const override;
	void MarkZoneSynced(int zone_rnum, Freshness version) override;
	void MarkIndexSynced(Freshness version) override;
	bool IsWritable() const override;

	// LoadRooms/Mobs/Objects load ONLY each entity type's primary table --
	// they do NOT call the child sub-loaders (flags/skills/exits/applies/
	// triggers/extra-descriptions/...), because those sub-loader queries have
	// no zone filter (full table scan) and would be WRONG if run against
	// zones this source didn't actually supply this boot (it would overwrite
	// a YAML-sourced zone's data with possibly-stale SQLite child rows).
	// Instead every LoadRooms/Mobs/Objects call records the zone_vnums it
	// actually returned in m_processed_zone_vnums (covering the whole zone
	// list on an unfiltered call, same as a plain non-composite boot), and
	// the orchestrator (db.cpp) calls FinalizeZoneRooms()/
	// FinalizeZoneEntities() once to run all the child sub-loaders scoped to
	// that processed-zone set, at the two points in the boot sequence
	// documented on IWorldDataSource.
	bool SupportsZoneFilter() const override { return true; }
	void FinalizeZoneRooms() override;
	void FinalizeZoneEntities() override;

	// Wrap the whole resync/resave in one transaction (see IWorldDataSource
	// for why) -- BeginTransaction()/CommitTransaction() are already
	// savepoint-nesting-safe, so this just brackets the outermost level; the
	// per-zone SaveZone/SaveRooms/... calls in between nest inside it instead
	// of each opening/closing their own top-level transaction.
	void BeginBulkWrite() override { BeginTransaction(); }
	void EndBulkWrite() override { CommitTransaction(); }

private:
	// True if `vnum`'s zone (vnum/100) was returned by this boot's LoadRooms/
	// Mobs/Objects call (filtered or not -- every returned row's zone is
	// recorded in m_processed_zone_vnums as it's parsed). The `.empty()`
	// fallback only matters if that call never populated anything at all
	// (e.g. OpenDatabase() failed before the query ran) -- default to
	// permissive rather than silently skipping every child sub-loader.
	bool IsZoneProcessed(int vnum) const;
	std::set<int> m_processed_zone_vnums;
	// Create the zone_sync / world_meta bookkeeping tables if absent, so an
	// older world.db produced before this feature still works (it just reports
	// freshness 0 until the first sync repopulates it).
	void EnsureSyncTables();
	// Bootstrap the core world schema (zones/rooms/mobs/objects/triggers/...)
	// into a brand-new, schema-less world.db, so the engine can create its own
	// SQLite cache from scratch without the Python converter. No-op if the
	// 'zones' table is already present (converter-built or previously
	// bootstrapped db). See sqlite_world_data_source_freshness.cpp for the
	// embedded DDL (kept in sync with tools/converter/world_schema.sql).
	void EnsureCoreSchema();
	// Record a zone's content version (a Freshness/mtime) and, for a
	// first-seen zone, bump the membership stamp. Called from MarkZoneSynced so
	// dual-writes/resyncs keep the freshness bookkeeping current.
	void TouchZoneSync(int zone_vnum, Freshness version);
	Freshness GetMetaFreshness(const char *key) const;

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
	// Parse one row of the primary `mobs` query into a freshly-allocated
	// CharData (not yet placed in mob_proto[]/mob_index[]). Caller (LoadMobs)
	// owns the prepared statement's column order and lifetime.
	LoadedMob LoadMobRow(sqlite3_stmt *stmt);
	void LoadMobFlags();
	void LoadMobSkills();
	void LoadMobTriggers();
	void LoadMobResistances();
	void LoadMobSaves();
	void LoadMobFeats();
	void LoadMobSpells();
	void LoadMobHelpers();
	void LoadMobDestinations();
	void LoadMobDeathLoad();

	// Object loading helpers
	// Parse one row of the primary `objects` query into a freshly-built
	// CObjectPrototype (not yet placed in obj_proto). Caller (LoadObjects)
	// owns the prepared statement's column order and lifetime.
	LoadedObject LoadObjectRow(sqlite3_stmt *stmt);
	void LoadObjectFlags();
	void LoadObjectApplies();
	void LoadObjectSkills();
	void LoadObjectExtraValues();
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
	// Nesting depth of Begin/CommitTransaction: 0 = no active transaction.
	// Depth 0->1 opens the real BEGIN/COMMIT; deeper levels use SAVEPOINT/
	// RELEASE so an outer BeginBulkWrite() and the per-zone Save* calls
	// nested inside it collapse into one real transaction (see BeginBulkWrite
	// above), while each SAVEPOINT can still be independently rolled back if
	// a single Save* fails without undoing already-committed sibling zones.
	int m_transaction_depth = 0;
};

// Bind a runtime KOI8-R string as UTF-8 (the on-disk text encoding, matching
// the converter and the utf8->koi8 read path). Binds NULL for a null pointer;
// ASCII passes through unchanged. Used by every Save*Record (and the flag/enum
// table writers) so engine writes round-trip identically to converter output.
// Free function so both member and helper-function call sites can use it.
void BindTextKoi(sqlite3_stmt *stmt, int col, const char *koi8);

// Factory function for creating SQLite data source
std::unique_ptr<IWorldDataSource> CreateSqliteDataSource(const std::string &db_path);

} // namespace world_loader

#endif // HAVE_SQLITE

#endif // SQLITE_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
