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

	// Reads: LoadZones() picks the index-freshness winner for zone_table,
	// then resolves a per-zone content winner for everything else. If not
	// every source supports per-zone loading (AllZonesSupportPerZoneLoad()
	// is false), LoadTriggers/Rooms/Mobs/Objects fall back to whole-source
	// delegation to the index winner -- the orchestrator (GameLoader::
	// BootWorld) checks that flag and only drives the per-zone LoadZone*
	// loop when every source can support it; otherwise it calls these
	// fallback methods exactly like a single source would.
	void LoadZones() override;
	void LoadTriggers() override;
	void LoadRooms() override;
	void LoadMobs() override;
	void LoadObjects() override;

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

	// True if every child source supports per-zone loading. Gates whether
	// the boot orchestrator uses the per-zone LoadZone* loop or falls back
	// to whole-source LoadTriggers/Rooms/Mobs/Objects (e.g. a composite that
	// includes the legacy backend, which never implemented LoadZone*).
	bool AllZonesSupportPerZoneLoad() const;

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

	std::vector<std::unique_ptr<IWorldDataSource>> m_sources;  // [0] = highest priority
	IWorldDataSource *m_index_source = nullptr;                 // zone_table/index winner
	std::map<int, IWorldDataSource *> m_zone_source;            // per-zone content winner
	std::map<IWorldDataSource *, std::vector<int>> m_stale_zones;  // per-source dirty-zone lists
	bool m_zone_sources_selected = false;
};

// Factory: build a composite from an ordered list of sub-sources.
std::unique_ptr<IWorldDataSource> CreateCompositeDataSource(
	std::vector<std::unique_ptr<IWorldDataSource>> sources);

} // namespace world_loader

#endif // COMPOSITE_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
