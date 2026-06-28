// Part of Bylins http://www.mud.ru
// Composite world data source - combines several backends with per-source
// "actuality" (freshness) and priority.
//
// Model (see configuration.xml <world_loader><sources>):
//   - Each child source has a priority given by its order (first == highest).
//   - On read, the source with the highest freshness wins; ties are broken by
//     priority. (v1 decides at whole-world granularity; per-zone selection is
//     layered on later via the LoadZone* hooks in IWorldDataSource.)
//   - On write, every source is written (fan-out / dual-write).
//   - After a read, any source that was staler than the one we read from is
//     scheduled for a full rewrite so all backends converge (self-heal).

#ifndef COMPOSITE_WORLD_DATA_SOURCE_H_
#define COMPOSITE_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"

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

	// Reads: delegate to the freshest source (decided lazily on first call).
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

	// Aggregated freshness/membership, so composites can themselves nest.
	std::vector<int> ListZoneVnums() const override;
	Freshness GetZoneFreshness(int zone_vnum) const override;
	Freshness GetIndexFreshness() const override;

	// Sources that were staler than the one we read from and should be fully
	// rewritten from the in-memory world after boot. Empty until LoadZones()
	// has run. The boot path (GameLoader::BootWorld) drives the actual rewrite,
	// since it owns access to the global world tables.
	const std::vector<IWorldDataSource *> &StaleSources() const { return m_stale_sources; }

	// The source actually read from (for logging). null until LoadZones().
	IWorldDataSource *ReadSource() const { return m_read_source; }

private:
	// Decide (once) which source to read from and which are stale.
	void SelectReadSource();
	// max(index freshness, freshest zone) for a single source.
	Freshness OverallFreshness(const IWorldDataSource &src) const;
	// Canonical content version of a zone after a write: the freshest the zone
	// looks across all sources (i.e. the just-written file mtime). Every source
	// is stamped with this so none ends up spuriously newer than the others.
	Freshness CanonicalZoneVersion(int zone_rnum) const;

	std::vector<std::unique_ptr<IWorldDataSource>> m_sources;  // [0] = highest priority
	IWorldDataSource *m_read_source = nullptr;                 // winner, lazily chosen
	std::vector<IWorldDataSource *> m_stale_sources;           // to rewrite after boot
	bool m_read_source_selected = false;
};

// Factory: build a composite from an ordered list of sub-sources.
std::unique_ptr<IWorldDataSource> CreateCompositeDataSource(
	std::vector<std::unique_ptr<IWorldDataSource>> sources);

} // namespace world_loader

#endif // COMPOSITE_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
