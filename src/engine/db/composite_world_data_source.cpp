// Part of Bylins http://www.mud.ru
// Composite world data source implementation.

#include "composite_world_data_source.h"

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

Freshness CompositeWorldDataSource::OverallFreshness(const IWorldDataSource &src) const
{
	Freshness best = src.GetIndexFreshness();
	for (int vnum : src.ListZoneVnums())
	{
		best = std::max(best, src.GetZoneFreshness(vnum));
	}
	return best;
}

void CompositeWorldDataSource::SelectReadSource()
{
	if (m_read_source_selected)
	{
		return;
	}
	m_read_source_selected = true;

	// Compute each source's overall freshness once.
	std::vector<Freshness> fresh(m_sources.size(), 0);
	size_t winner = 0;
	for (size_t i = 0; i < m_sources.size(); ++i)
	{
		fresh[i] = OverallFreshness(*m_sources[i]);
		// Strictly-greater wins; on ties keep the earlier (higher-priority) one.
		if (fresh[i] > fresh[winner])
		{
			winner = i;
		}
		log("World source [%zu] '%s': freshness=%llu",
			i, m_sources[i]->GetName().c_str(),
			static_cast<unsigned long long>(fresh[i]));
	}

	m_read_source = m_sources[winner].get();
	log("World read source: '%s' (priority %zu, freshness=%llu)",
		m_read_source->GetName().c_str(), winner,
		static_cast<unsigned long long>(fresh[winner]));

	// Every source strictly staler than the winner is rewritten after boot so
	// all backends converge to the loaded world.
	m_stale_sources.clear();
	for (size_t i = 0; i < m_sources.size(); ++i)
	{
		if (i == winner)
		{
			continue;
		}
		if (fresh[i] < fresh[winner])
		{
			m_stale_sources.push_back(m_sources[i].get());
			log("World source '%s' is stale -- will be rewritten after boot",
				m_sources[i]->GetName().c_str());
		}
	}
}

void CompositeWorldDataSource::LoadZones()
{
	SelectReadSource();
	m_read_source->LoadZones();
}

void CompositeWorldDataSource::LoadTriggers() { m_read_source->LoadTriggers(); }
void CompositeWorldDataSource::LoadRooms() { m_read_source->LoadRooms(); }
void CompositeWorldDataSource::LoadMobs() { m_read_source->LoadMobs(); }
void CompositeWorldDataSource::LoadObjects() { m_read_source->LoadObjects(); }

void CompositeWorldDataSource::SaveZone(int zone_rnum)
{
	for (auto &src : m_sources)
	{
		src->SaveZone(zone_rnum);
		src->MarkZoneSynced(zone_rnum);
	}
}

bool CompositeWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	bool ok = true;
	for (auto &src : m_sources)
	{
		ok = src->SaveTriggers(zone_rnum, specific_vnum, notify_level) && ok;
		src->MarkZoneSynced(zone_rnum);
	}
	return ok;
}

void CompositeWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	for (auto &src : m_sources)
	{
		src->SaveRooms(zone_rnum, specific_vnum);
		src->MarkZoneSynced(zone_rnum);
	}
}

void CompositeWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	for (auto &src : m_sources)
	{
		src->SaveMobs(zone_rnum, specific_vnum);
		src->MarkZoneSynced(zone_rnum);
	}
}

void CompositeWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	for (auto &src : m_sources)
	{
		src->SaveObjects(zone_rnum, specific_vnum);
		src->MarkZoneSynced(zone_rnum);
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
