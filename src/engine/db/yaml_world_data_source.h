// Part of Bylins http://www.mud.ru
// YAML world data source - loads world from YAML files

#ifndef YAML_WORLD_DATA_SOURCE_H_
#define YAML_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"
#include "world_data_source_base.h"

#ifdef HAVE_YAML

#include "engine/structs/flag_data.h"
#include "description.h"

#include <yaml-cpp/yaml.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iosfwd>

class ZoneData;
struct RoomData;
class CharData;
struct Trigger;
class CObjectPrototype;
class Koi8rYamlEmitter;

namespace utils {
	class ThreadPool;
}

namespace world_loader
{

// Physical layout of a zone's per-entity YAML files.
//   PerFile -- one file per entity in a sub-directory (mobs/00.yaml, mobs/01.yaml, ...)
//              plus a mobs/index.yaml listing the rel-numbers.
//   Flat    -- all entities of a type in one file (mobs.yaml) as a map keyed by
//              rel-number; the file is its own index, no index.yaml.
// Loading auto-detects the layout per (zone, sub-type); this only selects the
// layout used when WRITING.
enum class YamlLayout { PerFile, Flat };

// One unit of entity-load work: a single YAML file to parse, plus the vnums it
// contains. Per-file layout -> exactly one vnum (the file IS the entity node).
// Flat layout -> all vnums of that type in the zone (each entity is the node at
// the rel-number map key). Distributed across worker threads; each task is
// parsed entirely by one thread (so the loaded root node is never shared).
struct EntityFileTask {
	bool flat = false;
	std::string path;
	std::vector<int> vnums;  // sorted ascending
	// Flat layout only: DiscoverEntityFiles() already parses the whole file to
	// enumerate these vnums (the file's keys); stashing the parsed root here
	// lets the worker reuse it instead of calling YAML::LoadFile() a second
	// time on the same file. Per-file layout leaves this unset -- discovery
	// there only reads the tiny <sub>/index.yaml, never the entity file
	// itself, so the worker's own YAML::LoadFile(task.path) is the first (and
	// only) parse either way.
	YAML::Node parsed_root;
};

// Result of parsing rooms in a single thread
struct ParsedRoomBatch {
	LocalDescriptionIndex descriptions;  // Thread-local description index
	std::vector<std::tuple<int, RoomData*, size_t>> rooms;  // (vnum, room, local_desc_idx)
	std::map<int, std::vector<int>> triggers;  // Room triggers (room_vnum -> list of trigger vnums)
};

// YAML implementation for human-readable world files
class YamlWorldDataSource : public WorldDataSourceBase
{
public:
	explicit YamlWorldDataSource(const std::string &world_dir);
	~YamlWorldDataSource() override = default;

	std::string GetName() const override { return "YAML files: " + m_world_dir; }
	std::string GetKind() const override { return "yaml"; }

	void LoadZones() override;
	std::vector<LoadedTrigger> LoadTriggers(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedRoom> LoadRooms(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedMob> LoadMobs(const std::vector<int> *zone_filter = nullptr) override;
	std::vector<LoadedObject> LoadObjects(const std::vector<int> *zone_filter = nullptr) override;

	// Save methods (YAML is read-only for now)
	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;

	// Freshness / membership (for CompositeWorldDataSource). Freshness is the
	// newest file mtime (unix seconds) under the zone directory; the index
	// freshness is the mtime of zones/index.yaml (changes on zone add/remove).
	std::vector<int> ListZoneVnums() const override;
	Freshness GetZoneFreshness(int zone_vnum) const override;
	Freshness GetIndexFreshness() const override;

	bool SupportsZoneFilter() const override { return true; }

	// Exposed for unit tests: parses a single mob YAML file into a CharData.
	// Stateless aside from reading the global DictionaryManager singleton --
	// callers are responsible for loading dictionaries beforehand.
	CharData ParseMobFile(const std::string &file_path);
	// Parses a mob from an already-loaded node (per-file and flat layouts).
	CharData ParseMobNode(const YAML::Node &root);

private:
	// Initialize dictionaries
	bool LoadDictionaries();
	
	// Load world configuration (line endings, etc)
	bool LoadWorldConfig();

	// Get list of zone vnums from index.yaml
	std::vector<int> GetZoneList();

	// Discover the entity files for one sub-type ("mobs"/"objects"/"rooms"/
	// "triggers"), auto-detecting per zone whether the data lives in a flat
	// <sub>.yaml or a per-file <sub>/ directory. `zone_filter` null means every
	// zone (GetZoneList()); this is the only discovery entry point -- a full
	// zone list and no filter are the same request, so there is no separate
	// single-zone variant. Distributed across the thread pool (each zone is
	// independent work); see LoadTriggers/Rooms/Mobs/Objects for how the
	// results get used.
	std::vector<EntityFileTask> DiscoverEntityFiles(const std::string &sub, const std::vector<int> *zone_filter = nullptr);

	// Zone loading helpers
	void LoadZoneCommands(ZoneData &zone, const YAML::Node &commands_node);

	// Room loading helpers
	void LoadRoomExits(RoomData *room, const YAML::Node &exits_node, int room_vnum);
	void LoadRoomExtraDescriptions(RoomData *room, const YAML::Node &extras_node);

	// Flag parsing using dictionaries
	FlagData ParseFlags(const YAML::Node &node, const std::string &dict_name) const;
	int ParseEnum(const YAML::Node &node, const std::string &dict_name, int default_val = 0) const;
	int ParsePosition(const YAML::Node &node) const;
	int ParseGender(const YAML::Node &node, int default_val = 1) const;

	// Utility functions
	std::string GetText(const YAML::Node &node, const std::string &key, const std::string &default_val = "") const;
	int GetInt(const YAML::Node &node, const std::string &key, int default_val = 0) const;
	long GetLong(const YAML::Node &node, const std::string &key, long default_val = 0) const;

	// Convert UTF-8 from YAML to KOI8-R
	std::string ConvertToKoi8r(const std::string &utf8_str) const;

	// Helper methods for save operations
	std::string ConvertToUtf8(const std::string &koi8r_str) const;
	template<class FlagsT>
	std::vector<std::string> ConvertFlagsToNames(const FlagsT &flags, const std::string &dict_name) const;
	std::string ReverseLookupEnum(const std::string &dict_name, int value) const;
	bool WriteYamlAtomic(const std::string &filepath, const YAML::Node &node) const;

	// Rewrite an `index.yaml` file as a snapshot of the current in-memory
	// state. `top_key` is the YAML root key ("zones"/"mobs"/"objects"/...);
	// `values` is the sorted list of vnums or rel-numbers. Save* methods call
	// these after writing entity files so the on-disk index stays consistent
	// with what loader will see on next boot (no stale rnums, new entities
	// from OLC visible, no leftover deletions).
	bool WriteIndexYaml(const std::string &filepath,
						const std::string &top_key,
						const std::vector<int> &values) const;

	// Rebuild the per-zone index for one of the entity sub-directories
	// ("mobs"/"objects"/"rooms"/"triggers"). Iterates the corresponding
	// global prototype table, collects entries whose vnum/100 == zone_vnum,
	// writes the rel-numbers (vnum % 100) into <m_world_dir>/zones/<zone>/<sub>/index.yaml.
	bool RebuildPerZoneIndex(int zone_vnum, const std::string &sub) const;

	// Emit one entity's body (everything except the vnum, which is implied by
	// the filename in per-file layout or the map key in flat layout) at the
	// emitter's current indent. Shared by the per-file save loops and the flat
	// save path, which calls them one indent level deeper under a rel-number key.
	void EmitTriggerBody(Koi8rYamlEmitter &yaml, Trigger *trig);
	void EmitRoomBody(Koi8rYamlEmitter &yaml, std::ostream &out, RoomData *room);
	void EmitMobBody(Koi8rYamlEmitter &yaml, std::ostream &out, CharData &mob);
	void EmitObjectBody(Koi8rYamlEmitter &yaml, std::ostream &out, CObjectPrototype *obj);

	// Remove the artifacts of the layout we did NOT just write for a zone's
	// sub-type, so a save fully migrates between layouts (no leftovers):
	// after a flat save, drop the <sub>/ directory; after a per-file save,
	// drop the <sub>.yaml file.
	void CleanupOtherLayout(int zone_vnum, const std::string &sub, YamlLayout written) const;

	// Parallel zone-table loading (only used when m_num_threads > 1).
	void LoadZonesParallel();

	// Lazily create m_thread_pool/m_num_threads on first use. LoadZones() is
	// not guaranteed to have run on this instance before LoadTriggers/Rooms/
	// Mobs/Objects are called (a composite may have picked a different source
	// for the zone index) -- idempotent, safe to call from every entry point.
	void EnsureThreadPool();

	// Worker functions (thread-safe, parse single file)
	ZoneData ParseZoneFile(const std::string &file_path);
	Trigger* ParseTriggerFile(const std::string &file_path);
	RoomData* ParseRoomFile(const std::string &file_path, int zone_rnum, LocalDescriptionIndex &local_index, size_t &local_desc_idx);
	// ParseMobFile lives in the public section above (exposed for tests).
	CObjectPrototype* ParseObjectFile(const std::string &file_path, int vnum);

	// Node-level parsers shared by per-file and flat layouts (the *File
	// variants above are thin wrappers that LoadFile then delegate here).
	Trigger* ParseTriggerNode(const YAML::Node &root);
	RoomData* ParseRoomNode(const YAML::Node &root, int vnum, int zone_rnum, LocalDescriptionIndex &local_index, size_t &local_desc_idx);
	CObjectPrototype* ParseObjectNode(const YAML::Node &root, int vnum);

	// Helper: get configured thread count from runtime config
	size_t GetConfiguredThreadCount() const;

	std::string m_world_dir;
	bool m_dictionaries_loaded = false;
	bool m_convert_lf_to_crlf = false;  // Convert LF to CR+LF for DOS line endings
	YamlLayout m_save_layout = YamlLayout::Flat;  // Layout used when writing zones (default: flat)

	// Threading support
	std::unique_ptr<utils::ThreadPool> m_thread_pool;
	size_t m_num_threads;
};

// Factory function for creating YAML data source
std::unique_ptr<IWorldDataSource> CreateYamlDataSource(const std::string &world_dir);

} // namespace world_loader

#endif // HAVE_YAML

#endif // YAML_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
