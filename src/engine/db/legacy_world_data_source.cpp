// Part of Bylins http://www.mud.ru
// Legacy world data source implementation

#include "legacy_world_data_source.h"
#include "db.h"
#include "obj_prototypes.h"
#include "utils/logger.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

// Forward declarations for OLC save functions
void zedit_save_to_disk(int zone_rnum);
void redit_save_to_disk(int zone_rnum);
void medit_save_to_disk(int zone_rnum);
void oedit_save_to_disk(int zone_rnum);
bool trigedit_save_to_disk(int zone_rnum, int notify_level);

namespace world_loader
{

namespace
{

// RAII helper: chdir into target on construction, restore on destruction.
// Used to redirect WLD_PREFIX-relative paths inside OLC save_to_disk
// routines to a caller-supplied output directory without modifying the
// macros or save routines themselves.
class ScopedChdir
{
public:
	explicit ScopedChdir(const std::filesystem::path &target)
		: m_saved(std::filesystem::current_path())
	{
		std::filesystem::current_path(target);
	}
	~ScopedChdir() noexcept
	{
		std::error_code ec;
		std::filesystem::current_path(m_saved, ec);
	}
	ScopedChdir(const ScopedChdir &) = delete;
	ScopedChdir &operator=(const ScopedChdir &) = delete;

private:
	std::filesystem::path m_saved;
};

// Ensure <target_dir>/world/{wld,mob,obj,zon,trg}/ exist. OLC save routines
// fopen() into these directories without creating them.
void EnsureLegacyWorldLayout(const std::filesystem::path &target_dir)
{
	static const char *const kSubdirs[] = {"wld", "mob", "obj", "zon", "trg"};
	const auto world_root = target_dir / "world";
	std::filesystem::create_directories(world_root);
	for (const auto *sub : kSubdirs) {
		std::filesystem::create_directories(world_root / sub);
	}
}

void WriteIndexFile(const std::filesystem::path &dir, const std::string &extension)
{
	if (!std::filesystem::is_directory(dir)) {
		return;
	}
	std::vector<std::string> names;
	for (const auto &entry : std::filesystem::directory_iterator(dir)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const auto &path = entry.path();
		if (path.extension() != extension) {
			continue;
		}
		names.push_back(path.filename().string());
	}
	// Sort numerically by leading vnum, falling back to lexicographic.
	std::sort(names.begin(), names.end(),
		[](const std::string &a, const std::string &b) {
			const long va = std::strtol(a.c_str(), nullptr, 10);
			const long vb = std::strtol(b.c_str(), nullptr, 10);
			if (va != vb) {
				return va < vb;
			}
			return a < b;
		});
	const auto tmp = dir / "index.new";
	{
		std::ofstream os(tmp, std::ios::trunc);
		if (!os) {
			throw std::runtime_error("cannot open " + tmp.string());
		}
		for (const auto &n : names) {
			os << n << "\n";
		}
		os << "$\n";
	}
	std::filesystem::rename(tmp, dir / "index");
}

} // namespace

LegacyWorldDataSource::LegacyWorldDataSource(std::string target_dir)
	: m_target_dir(std::move(target_dir))
{
}

void LegacyWorldDataSource::LoadZones()
{
	log("Loading zone table.");
	GameLoader::BootIndex(DB_BOOT_ZON);
}

void LegacyWorldDataSource::LoadTriggers()
{
	log("Loading triggers and generating index.");
	GameLoader::BootIndex(DB_BOOT_TRG);
}

void LegacyWorldDataSource::LoadRooms()
{
	log("Loading rooms.");
	GameLoader::BootIndex(DB_BOOT_WLD);
}

void LegacyWorldDataSource::LoadMobs()
{
	log("Loading mobs and generating index.");
	GameLoader::BootIndex(DB_BOOT_MOB);
}

void LegacyWorldDataSource::LoadObjects()
{
	log("Loading objs and generating index.");
	GameLoader::BootIndex(DB_BOOT_OBJ);
}

void LegacyWorldDataSource::SaveZone(int zone_rnum)
{
	if (m_target_dir.empty()) {
		zedit_save_to_disk(zone_rnum);
		return;
	}
	EnsureLegacyWorldLayout(m_target_dir);
	ScopedChdir guard(m_target_dir);
	zedit_save_to_disk(zone_rnum);
}

bool LegacyWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (m_target_dir.empty()) {
		return trigedit_save_to_disk(zone_rnum, notify_level);
	}
	EnsureLegacyWorldLayout(m_target_dir);
	ScopedChdir guard(m_target_dir);
	return trigedit_save_to_disk(zone_rnum, notify_level);
}

void LegacyWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (m_target_dir.empty()) {
		redit_save_to_disk(zone_rnum);
		return;
	}
	EnsureLegacyWorldLayout(m_target_dir);
	ScopedChdir guard(m_target_dir);
	redit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (m_target_dir.empty()) {
		medit_save_to_disk(zone_rnum);
		return;
	}
	EnsureLegacyWorldLayout(m_target_dir);
	ScopedChdir guard(m_target_dir);
	medit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (m_target_dir.empty()) {
		oedit_save_to_disk(zone_rnum);
		return;
	}
	EnsureLegacyWorldLayout(m_target_dir);
	ScopedChdir guard(m_target_dir);
	oedit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::RebuildIndexes()
{
	if (m_target_dir.empty()) {
		return;
	}
	const std::filesystem::path world_root = std::filesystem::path(m_target_dir) / "world";
	WriteIndexFile(world_root / "wld", ".wld");
	WriteIndexFile(world_root / "mob", ".mob");
	WriteIndexFile(world_root / "obj", ".obj");
	WriteIndexFile(world_root / "zon", ".zon");
	WriteIndexFile(world_root / "trg", ".trg");
}

std::unique_ptr<IWorldDataSource> CreateLegacyDataSource()
{
	return std::make_unique<LegacyWorldDataSource>();
}

std::unique_ptr<IWorldDataSource> CreateLegacyDataSource(std::string target_dir)
{
	return std::make_unique<LegacyWorldDataSource>(std::move(target_dir));
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
