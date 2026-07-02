// Part of Bylins http://www.mud.ru
// Composite world data source implementation.

#include "composite_world_data_source.h"

#include "engine/entities/zone.h"
#include "gameplay/mechanics/dungeons.h"
#include "utils/logger.h"

#include <algorithm>

namespace world_loader
{

CompositeWorldDataSource::CompositeWorldDataSource(
	std::vector<std::unique_ptr<IWorldDataSource>> sources)
	: m_sources(std::move(sources))
{
}

std::string CompositeWorldDataSource::GetName() const
{
	std::string name = "Composite[";
	for (size_t i = 0; i < m_sources.size(); ++i)
	{
		if (i)
		{
			name += " > ";
		}
		name += m_sources[i]->GetName();
	}
	name += "]";
	return name;
}

bool CompositeWorldDataSource::AllZonesSupportPerZoneLoad() const
{
	for (const auto &src : m_sources)
	{
		if (!src->SupportsPerZoneLoad())
		{
			return false;
		}
	}
	return true;
}

IWorldDataSource *CompositeWorldDataSource::SourceForZone(int zone_vnum) const
{
	auto it = m_zone_source.find(zone_vnum);
	return it != m_zone_source.end() ? it->second : nullptr;
}

void CompositeWorldDataSource::SelectZoneSources()
{
	if (m_zone_sources_selected)
	{
		return;
	}
	m_zone_sources_selected = true;

	// Index winner: whole-source, decides zone_table membership/order.
	std::vector<Freshness> index_fresh(m_sources.size(), 0);
	size_t index_winner = 0;
	for (size_t i = 0; i < m_sources.size(); ++i)
	{
		index_fresh[i] = m_sources[i]->GetIndexFreshness();
		if (index_fresh[i] > index_fresh[index_winner])
		{
			index_winner = i;
		}
	}
	m_index_source = m_sources[index_winner].get();
	log("World index source: '%s' (priority %zu, freshness=%llu)",
		m_index_source->GetName().c_str(), index_winner,
		static_cast<unsigned long long>(index_fresh[index_winner]));

	// Content winner: per zone, strictly-greater freshness wins; ties keep
	// the earlier (higher-priority) source. Union of every source's zone
	// list, so a zone can be picked even if it's not in the index winner.
	std::vector<int> all_zones = ListZoneVnums();
	m_zone_source.clear();
	m_stale_zones.clear();

	std::map<IWorldDataSource *, int> won_count;
	for (int zone_vnum : all_zones)
	{
		if (zone_vnum >= dungeons::kZoneStartDungeons)
		{
			// Dungeon-instance zones are never persisted; skip freshness
			// bookkeeping for them entirely (they shouldn't appear in
			// ListZoneVnums() from a real backend anyway, but be defensive).
			continue;
		}

		std::vector<Freshness> fresh(m_sources.size(), 0);
		size_t winner = 0;
		for (size_t i = 0; i < m_sources.size(); ++i)
		{
			fresh[i] = m_sources[i]->GetZoneFreshness(zone_vnum);
			if (fresh[i] > fresh[winner])
			{
				winner = i;
			}
		}

		IWorldDataSource *winner_src = m_sources[winner].get();
		m_zone_source[zone_vnum] = winner_src;
		++won_count[winner_src];

		for (size_t i = 0; i < m_sources.size(); ++i)
		{
			if (i == winner)
			{
				continue;
			}
			if (fresh[i] < fresh[winner])
			{
				m_stale_zones[m_sources[i].get()].push_back(zone_vnum);
			}
		}
	}

	// Roll-up log, not one line per zone -- a full-world boot can have
	// hundreds of zones.
	for (const auto &src : m_sources)
	{
		log("World source '%s': won %d zone(s), %zu stale zone(s) to resync",
			src->GetName().c_str(), won_count[src.get()], m_stale_zones[src.get()].size());
	}
}

void CompositeWorldDataSource::LoadZones()
{
	SelectZoneSources();
	m_index_source->LoadZones();
}

void CompositeWorldDataSource::LoadTriggers() { m_index_source->LoadTriggers(); }
void CompositeWorldDataSource::LoadRooms() { m_index_source->LoadRooms(); }
void CompositeWorldDataSource::LoadMobs() { m_index_source->LoadMobs(); }
void CompositeWorldDataSource::LoadObjects() { m_index_source->LoadObjects(); }

Freshness CompositeWorldDataSource::CanonicalZoneVersion(int zone_rnum) const
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		return 0;
	}
	const int vnum = zone_table[zone_rnum].vnum;
	Freshness version = 0;
	for (const auto &src : m_sources)
	{
		version = std::max(version, src->GetZoneFreshness(vnum));
	}
	return version;
}

void CompositeWorldDataSource::SaveZone(int zone_rnum)
{
	for (auto &src : m_sources)
	{
		src->SaveZone(zone_rnum);
	}
	const Freshness version = CanonicalZoneVersion(zone_rnum);
	for (auto &src : m_sources)
	{
		src->MarkZoneSynced(zone_rnum, version);
	}
}

bool CompositeWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	bool ok = true;
	for (auto &src : m_sources)
	{
		ok = src->SaveTriggers(zone_rnum, specific_vnum, notify_level) && ok;
	}
	const Freshness version = CanonicalZoneVersion(zone_rnum);
	for (auto &src : m_sources)
	{
		src->MarkZoneSynced(zone_rnum, version);
	}
	return ok;
}

void CompositeWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	for (auto &src : m_sources)
	{
		src->SaveRooms(zone_rnum, specific_vnum);
	}
	const Freshness version = CanonicalZoneVersion(zone_rnum);
	for (auto &src : m_sources)
	{
		src->MarkZoneSynced(zone_rnum, version);
	}
}

void CompositeWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	for (auto &src : m_sources)
	{
		src->SaveMobs(zone_rnum, specific_vnum);
	}
	const Freshness version = CanonicalZoneVersion(zone_rnum);
	for (auto &src : m_sources)
	{
		src->MarkZoneSynced(zone_rnum, version);
	}
}

void CompositeWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	for (auto &src : m_sources)
	{
		src->SaveObjects(zone_rnum, specific_vnum);
	}
	const Freshness version = CanonicalZoneVersion(zone_rnum);
	for (auto &src : m_sources)
	{
		src->MarkZoneSynced(zone_rnum, version);
	}
}

void CompositeWorldDataSource::FinalizeZoneRooms()
{
	for (auto &src : m_sources)
	{
		src->FinalizeZoneRooms();
	}
}

void CompositeWorldDataSource::FinalizeZoneEntities()
{
	for (auto &src : m_sources)
	{
		src->FinalizeZoneEntities();
	}
}

void CompositeWorldDataSource::FinalizeResave()
{
	for (auto &src : m_sources)
	{
		src->FinalizeResave();
	}
}

std::vector<int> CompositeWorldDataSource::ListZoneVnums() const
{
	std::vector<int> all;
	for (const auto &src : m_sources)
	{
		auto v = src->ListZoneVnums();
		all.insert(all.end(), v.begin(), v.end());
	}
	std::sort(all.begin(), all.end());
	all.erase(std::unique(all.begin(), all.end()), all.end());
	return all;
}

Freshness CompositeWorldDataSource::GetZoneFreshness(int zone_vnum) const
{
	Freshness best = 0;
	for (const auto &src : m_sources)
	{
		best = std::max(best, src->GetZoneFreshness(zone_vnum));
	}
	return best;
}

Freshness CompositeWorldDataSource::GetIndexFreshness() const
{
	Freshness best = 0;
	for (const auto &src : m_sources)
	{
		best = std::max(best, src->GetIndexFreshness());
	}
	return best;
}

std::unique_ptr<IWorldDataSource> CreateCompositeDataSource(
	std::vector<std::unique_ptr<IWorldDataSource>> sources)
{
	return std::make_unique<CompositeWorldDataSource>(std::move(sources));
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
