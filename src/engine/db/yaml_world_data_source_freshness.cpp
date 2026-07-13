// Part of Bylins http://www.mud.ru
// Freshness / membership queries for the YAML world data source.
// Kept in a separate (ASCII-only) translation unit so the large KOI8-R
// yaml_world_data_source.cpp does not need touching.

#include "yaml_world_data_source.h"

#ifdef HAVE_YAML

#include <sys/stat.h>

#include <filesystem>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace world_loader
{

namespace
{
// Unix mtime (seconds) of a single path; 0 if it cannot be stat()ed. Using
// POSIX stat keeps the value on the system (unix) clock so it is directly
// comparable with the SQLite source's time()-based sync stamps. std::filesystem
// last_write_time uses file_clock, whose epoch is unspecified -- not comparable.
Freshness PathMtime(const std::string &path)
{
	struct stat st;
	if (::stat(path.c_str(), &st) != 0)
	{
		return 0;
	}
	return static_cast<Freshness>(st.st_mtime);
}
} // namespace

std::vector<int> YamlWorldDataSource::ListZoneVnums() const
{
	std::vector<int> zones;
	const std::string index_path = m_world_dir + "/zones/index.yaml";
	try
	{
		YAML::Node root = YAML::LoadFile(index_path);
		if (root["zones"] && root["zones"].IsSequence())
		{
			for (const auto &zone_node : root["zones"])
			{
				zones.push_back(zone_node.as<int>());
			}
		}
	}
	catch (const YAML::Exception &)
	{
		// Missing/unreadable index -> empty membership.
	}
	return zones;
}

Freshness YamlWorldDataSource::GetZoneFreshness(int zone_vnum) const
{
	namespace fs = std::filesystem;
	const std::string zone_dir = m_world_dir + "/zones/" + std::to_string(zone_vnum);

	std::error_code ec;
	if (!fs::exists(zone_dir, ec) || ec)
	{
		return 0;
	}

	// Newest mtime among the zone directory and every file beneath it. Editing
	// a single entity file bumps that file's mtime (not the directory's), so we
	// must walk the whole subtree.
	Freshness newest = PathMtime(zone_dir);
	for (fs::recursive_directory_iterator it(zone_dir, ec), end; !ec && it != end; it.increment(ec))
	{
		newest = std::max(newest, PathMtime(it->path().string()));
	}
	return newest;
}

Freshness YamlWorldDataSource::GetIndexFreshness() const
{
	// zones/index.yaml is rewritten whenever the set of zones changes, so its
	// mtime arbitrates membership (zone add/remove) differences.
	return PathMtime(m_world_dir + "/zones/index.yaml");
}

} // namespace world_loader

#endif // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
