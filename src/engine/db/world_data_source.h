// Part of Bylins http://www.mud.ru
// World data source interface for pluggable world loading

#ifndef WORLD_DATA_SOURCE_H_
#define WORLD_DATA_SOURCE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace world_loader
{

// Freshness token for a zone (or the zone index) within a data source.
// Encoded as a unix mtime in seconds; larger == more recently changed.
// 0 means "unknown" or "this source does not hold the entity".
using Freshness = std::uint64_t;

// Abstract interface for world data sources
// Allows different implementations (legacy files, YAML, database, etc.)
class IWorldDataSource
{
public:
	virtual ~IWorldDataSource() = default;

	// Returns the name/description of this data source for logging
	virtual std::string GetName() const = 0;

	// Short, stable machine-matchable identifier ("yaml", "sqlite", ...), used
	// by GameLoader::BootWorld to resolve the -F <kind> "force one-shot
	// source" CLI flag against CompositeWorldDataSource's children. Default
	// "unknown" -- backends that don't participate in per-zone freshness
	// (legacy, null) need not override this.
	virtual std::string GetKind() const { return "unknown"; }

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

	// --- Freshness / membership (used by CompositeWorldDataSource) ---
	//
	// These let a composite source decide, per zone, which backend holds the
	// most recently changed data ("actuality"), and reconcile zone membership
	// (additions/deletions). Sources that do not track freshness (legacy,
	// null) keep the defaults and never win a comparison, so single-source
	// setups are unaffected.

	// Vnums of all zones this source currently holds. Default: empty.
	virtual std::vector<int> ListZoneVnums() const { return {}; }

	// Freshness of a single zone's *content* (rooms/mobs/objects/triggers/
	// resets). For file backends this is the newest mtime under the zone's
	// directory; for SQLite it is the sync timestamp recorded on last write.
	// 0 if the zone is absent from this source. Default: 0.
	virtual Freshness GetZoneFreshness(int /*zone_vnum*/) const { return 0; }

	// Freshness of the zone *index* (the set of zones). Changes when a zone is
	// added or removed, so it arbitrates membership differences between
	// sources. 0 if unknown. Default: 0.
	virtual Freshness GetIndexFreshness() const { return 0; }

	// Load a single zone's entities of one kind into the global tables. Used by
	// the composite to assemble a world per-zone from the freshest source.
	// Default: not supported -- callers must check SupportsPerZoneLoad().
	virtual bool SupportsPerZoneLoad() const { return false; }
	virtual void LoadZone(int /*zone_vnum*/) {}
	virtual void LoadZoneTriggers(int /*zone_vnum*/) {}
	virtual void LoadZoneRooms(int /*zone_vnum*/) {}
	virtual void LoadZoneMobs(int /*zone_vnum*/) {}
	virtual void LoadZoneObjects(int /*zone_vnum*/) {}

	// Count of mobs/triggers a zone holds in this source, WITHOUT fully
	// parsing them. Used by the per-zone boot orchestrator to size the
	// pre-allocated mob_index/mob_proto/trig_index arrays once, up front,
	// before the per-zone LoadZoneMobs/LoadZoneTriggers fill pass (those
	// arrays are raw C arrays elsewhere in the engine and cannot grow
	// incrementally). Default: 0 -- callers must check SupportsPerZoneLoad().
	virtual int CountZoneMobs(int /*zone_vnum*/) const { return 0; }
	virtual int CountZoneTriggers(int /*zone_vnum*/) const { return 0; }

	// Called once by the orchestrator, letting a backend run any
	// finalization step it deferred per-zone for cost or correctness reasons
	// (e.g. child-table sub-loaders that only make sense run once, scoped to
	// exactly the zones this source actually loaded -- see
	// SqliteWorldDataSource for a concrete case). Split into a room and an
	// entity phase because they run at different points in the boot
	// sequence: FinalizeZoneRooms() must run BEFORE the door-vnum-to-rnum
	// resolution pass (RosolveWorldDoorToRoomVnumsToRnums in db.cpp), since
	// that pass needs every room's exits already populated; mob/object child
	// data has no such downstream dependency, so FinalizeZoneEntities() runs
	// once at the very end, after every entity type is loaded. Default: no-op.
	virtual void FinalizeZoneRooms() {}
	virtual void FinalizeZoneEntities() {}

	// Record that a zone now holds content as of `version` (a Freshness/mtime),
	// updating this source's freshness bookkeeping. The composite stamps every
	// source with the SAME canonical version after a write/resync, so a freshly
	// synced backend ends up EQUAL to (not newer than) the one it copied from --
	// otherwise the two would trade "stale" roles every boot. Takes a runtime
	// zone rnum (same as Save*). Default: no-op (file backends derive freshness
	// from on-disk mtimes, which the write already updated).
	virtual void MarkZoneSynced(int /*zone_rnum*/, Freshness /*version*/) {}

	// Record that this source's zone *index* (membership) is current as of
	// `version`. Stamped after a resync so the index freshness matches the
	// source it was rebuilt from. Default: no-op.
	virtual void MarkIndexSynced(Freshness /*version*/) {}

	// Whether this source can be written to right now. Default: true.
	virtual bool IsWritable() const { return true; }

	// Called once after a full-world resave (GameLoader::ResaveWorld) so a
	// backend can finalize global structures that aren't maintained
	// incrementally by per-zone Save*. YAML/SQLite rebuild their indexes
	// inside Save* and need nothing here; the legacy backend regenerates its
	// per-subdir boot "index" files. Default: no-op.
	virtual void FinalizeResave() {}
};

// Factory function type for creating data sources
using DataSourceFactory = std::unique_ptr<IWorldDataSource>(*)();

} // namespace world_loader

#endif // WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
