// Part of Bylins http://www.mud.ru
// Composite world data source - combines several backends with per-source
// "actuality" (freshness) and priority.
//
// Model (see configuration.xml <world_loader><sources>):
//   - Each child source has a priority given by its order (first == highest).
//   - Zone INDEX (membership: which zones exist) is a whole-source decision:
//     the source with the highest GetIndexFreshness() wins and supplies
//     LoadZones()/zone_table (zone add/remove is rare, so this stays coarse).
//   - Zone CONTENT (rooms/mobs/objects/triggers) is decided PER ZONE: for
//     each zone vnum (union of every source's ListZoneVnums()), the source
//     with the highest GetZoneFreshness(vnum) wins; ties broken by priority.
//     This is what makes editing one zone live (via OLC/admin-api, already
//     zone-scoped writes) NOT fall back to reloading the entire world from
//     the stale-everywhere-else source -- only that one zone's content comes
//     from the edited source, every other zone still reads its cached copy.
//   - On write, every source is written (fan-out / dual-write).
//   - After a read, for each source, the zones where it lost the per-zone
//     freshness comparison are resynced from the loaded world (self-heal) --
//     NOT the whole zone list, just the ones it actually lost.
//   - ForceSource(kind) (the -F CLI flag) bypasses freshness entirely for one
//     boot: the named source wins every zone unconditionally, and every other
//     source's whole zone list is force-resynced from it. For moving a cached
//     SQLite world.db to a different machine and rebuilding its YAML tree --
//     mtime-based freshness isn't portable across machines, so this is an
//     explicit override, not something freshness comparison should try to
//     infer.

#ifndef COMPOSITE_WORLD_DATA_SOURCE_H_
#define COMPOSITE_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace world_loader
{

class CompositeWorldDataSource : public IWorldDataSource
{
public:
	// sources[0] has the highest priority (wins freshness ties).
	explicit CompositeWorldDataSource(std::vector<std::unique_ptr<IWorldDataSource>> sources);

	std::string GetName() const override;

	// Reads: LoadZones() picks the index-freshness winner for zone_table, then
	// resolves a per-zone content winner for everything else. LoadTriggers/
	// Rooms/Mobs/Objects fan out to each winning source with ITS OWN zone
	// subset (via SourceForZone), gather the results, and return them
	// concatenated -- no different from calling a single source, just with
	// more than one contributor. The caller (GameLoader::BootWorld) does not
	// need to know or care whether ds_ptr is a composite; it always does the
	// same thing: call with a null filter, sort what comes back by vnum, place
	// it in the global tables. If any child source can't honor a zone filter
	// (SupportsZoneFilter() false -- e.g. the legacy backend), this instead
	// delegates the whole call, unfiltered, to the index winner, matching
	// what a single-source deployment would do.
	void LoadZones() override;
	std::vector<LoadedTrigger> LoadTriggers(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedRoom> LoadRooms(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedMob> LoadMobs(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedObject> LoadObjects(const std::vector<int> *zone_filter = nullptr) override;

	// The composite ITSELF always returns real, placeable results from
	// LoadTriggers/Rooms/Mobs/Objects above (it discards nothing) -- whether
	// any CHILD source can't filter is a separate, internal concern handled
	// by AllSourcesSupportZoneFilter()/the unfiltered-delegation fallback.
	// The boot orchestrator (db.cpp) must not treat the composite itself as
	// a self-managing source (that's only true of the legacy backend).
	bool SupportsZoneFilter() const override { return true; }

	// Writes: fan out to every source so all backends stay in sync.
	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;
	void FinalizeResave() override;

	// Fan out to every source -- see IWorldDataSource::FinalizeZoneRooms/
	// FinalizeZoneEntities for when the boot orchestrator calls each.
	void FinalizeZoneRooms() override;
	void FinalizeZoneEntities() override;

	// Aggregated freshness/membership, so composites can themselves nest.
	std::vector<int> ListZoneVnums() const override;
	Freshness GetZoneFreshness(int zone_vnum) const override;
	Freshness GetIndexFreshness() const override;

	// One-shot override for the -F <kind> CLI flag: skip the normal per-zone
	// freshness comparison entirely and make the source whose GetKind() ==
	// `kind` win every zone (index and content), while every OTHER writable
	// source has ALL its zones marked stale -- so the boot's normal resync
	// pass (GameLoader::BootWorld, driven by StaleZonesBySource()) rebuilds
	// them unconditionally from what was just read, instead of treating the
	// forced source as a cache that might lose a freshness comparison. Must
	// be called before LoadZones(). Returns false (and changes nothing) if no
	// child source has that GetKind() -- the caller should treat that as
	// fatal rather than silently booting in normal cache mode.
	bool ForceSource(const std::string &kind);

	// The per-zone content winner for `zone_vnum` (resolved by LoadZones()).
	// null if LoadZones() hasn't run yet or the vnum is unknown to every
	// source.
	IWorldDataSource *SourceForZone(int zone_vnum) const;

	// For each source, the zone vnums where it lost the per-zone freshness
	// comparison (excluding dungeon-instance vnums, which are never
	// persisted) and should be resynced from the loaded world after boot.
	// Empty until LoadZones() has run. The boot path (GameLoader::BootWorld)
	// drives the actual resync, since it owns access to the global world
	// tables.
	const std::map<IWorldDataSource *, std::vector<int>> &StaleZonesBySource() const { return m_stale_zones; }

private:
	// Decide (once) the index-freshness winner (m_index_source) and, per
	// zone, the content-freshness winner (m_zone_source) plus each source's
	// stale-zone list (m_stale_zones).
	void SelectZoneSources();
	// Canonical content version of a zone after a write: the freshest the zone
	// looks across all sources (i.e. the just-written file mtime). Every source
	// is stamped with this so none ends up spuriously newer than the others.
	Freshness CanonicalZoneVersion(int zone_rnum) const;

	// True if every child source can honor a zone filter. False means one of
	// them (e.g. the legacy backend) can only load everything at once, so
	// LoadTriggers/Rooms/Mobs/Objects fall back to unfiltered whole-source
	// delegation to the index winner instead of fanning out per-zone.
	bool AllSourcesSupportZoneFilter() const;

	// Group m_zone_source by winning source (dungeon-instance vnums already
	// excluded by SelectZoneSources), intersected with `filter` if non-null.
	// Shared by LoadTriggers/Rooms/Mobs/Objects, which differ only in which
	// IWorldDataSource method they call per source.
	std::map<IWorldDataSource *, std::vector<int>> WinningZonesBySource(const std::vector<int> *filter) const;

	std::vector<std::unique_ptr<IWorldDataSource>> m_sources;  // [0] = highest priority
	IWorldDataSource *m_index_source = nullptr;                 // zone_table/index winner
	std::map<int, IWorldDataSource *> m_zone_source;            // per-zone content winner
	std::map<IWorldDataSource *, std::vector<int>> m_stale_zones;  // per-source dirty-zone lists
	bool m_zone_sources_selected = false;
	std::string m_forced_kind;  // set by ForceSource(); empty = normal freshness-based selection
};

// Factory: build a composite from an ordered list of sub-sources.
std::unique_ptr<IWorldDataSource> CreateCompositeDataSource(
	std::vector<std::unique_ptr<IWorldDataSource>> sources);

} // namespace world_loader

#endif // COMPOSITE_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
