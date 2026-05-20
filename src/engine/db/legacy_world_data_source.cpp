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
#include <optional>
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
//
// TODO(#3301): this chdir dance exists only because the OLC *_save_to_disk
// routines and the legacy loaders hardcode the "world/{wld,mob,...}/" paths
// via compile-time *_PREFIX macros. Teaching them to take a base path would
// let LegacyWorldDataSource carry m_world_dir as a plain path prefix (like
// YAML/SQLite) and drop the global-cwd manipulation entirely.
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

// Enter m_world_dir for the duration of a load/save call so the OLC and
// BootIndex routines, which use paths relative to cwd ("world/wld/" etc.),
// operate on this source's world directory. An empty world_dir means "the
// current working directory" -- the running server's mode (cwd is the data
// dir after the main chdir(-d)) -- and yields no chdir at all.
std::optional<ScopedChdir> EnterWorldDir(const std::string &world_dir)
{
	if (world_dir.empty()) {
		return std::nullopt;
	}
	return std::optional<ScopedChdir>(std::in_place, world_dir);
}

} // namespace

LegacyWorldDataSource::LegacyWorldDataSource(std::string world_dir)
	: m_world_dir(std::move(world_dir))
{
}

void LegacyWorldDataSource::LoadZones()
{
	log("Loading zone table.");
	auto guard = EnterWorldDir(m_world_dir);
	GameLoader::BootIndex(DB_BOOT_ZON);
}

void LegacyWorldDataSource::LoadTriggers()
{
	log("Loading triggers and generating index.");
	auto guard = EnterWorldDir(m_world_dir);
	GameLoader::BootIndex(DB_BOOT_TRG);
}

void LegacyWorldDataSource::LoadRooms()
{
	log("Loading rooms.");
	auto guard = EnterWorldDir(m_world_dir);
	GameLoader::BootIndex(DB_BOOT_WLD);
}

void LegacyWorldDataSource::LoadMobs()
{
	log("Loading mobs and generating index.");
	auto guard = EnterWorldDir(m_world_dir);
	GameLoader::BootIndex(DB_BOOT_MOB);
}

void LegacyWorldDataSource::LoadObjects()
{
	log("Loading objs and generating index.");
	auto guard = EnterWorldDir(m_world_dir);
	GameLoader::BootIndex(DB_BOOT_OBJ);
}

void LegacyWorldDataSource::SaveZone(int zone_rnum)
{
	if (!m_world_dir.empty()) {
		EnsureLegacyWorldLayout(m_world_dir);
	}
	auto guard = EnterWorldDir(m_world_dir);
	zedit_save_to_disk(zone_rnum);
}

bool LegacyWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (!m_world_dir.empty()) {
		EnsureLegacyWorldLayout(m_world_dir);
	}
	auto guard = EnterWorldDir(m_world_dir);
	return trigedit_save_to_disk(zone_rnum, notify_level);
}

void LegacyWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (!m_world_dir.empty()) {
		EnsureLegacyWorldLayout(m_world_dir);
	}
	auto guard = EnterWorldDir(m_world_dir);
	redit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (!m_world_dir.empty()) {
		EnsureLegacyWorldLayout(m_world_dir);
	}
	auto guard = EnterWorldDir(m_world_dir);
	medit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // Legacy format always saves entire zone
	if (!m_world_dir.empty()) {
		EnsureLegacyWorldLayout(m_world_dir);
	}
	auto guard = EnterWorldDir(m_world_dir);
	oedit_save_to_disk(zone_rnum);
}

void LegacyWorldDataSource::FinalizeResave()
{
	// The OLC save_to_disk routines write individual <vnum>.<ext> files but
	// do not maintain the boot indexes. After a full resave regenerate them
	// so the resulting tree is bootable. In-place (empty world_dir) server
	// saves don't rebuild the whole index, so this is a no-op there.
	if (m_world_dir.empty()) {
		return;
	}
	const std::filesystem::path world_root = std::filesystem::path(m_world_dir) / "world";
	WriteIndexFile(world_root / "wld", ".wld");
	WriteIndexFile(world_root / "mob", ".mob");
	WriteIndexFile(world_root / "obj", ".obj");
	WriteIndexFile(world_root / "zon", ".zon");
	WriteIndexFile(world_root / "trg", ".trg");
}

std::unique_ptr<IWorldDataSource> CreateLegacyDataSource(std::string world_dir)
{
	return std::make_unique<LegacyWorldDataSource>(std::move(world_dir));
}

} // namespace world_loader

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
