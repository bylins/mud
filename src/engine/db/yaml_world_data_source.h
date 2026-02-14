// Part of Bylins http://www.mud.ru
// YAML world data source - loads world from YAML files

#ifndef YAML_WORLD_DATA_SOURCE_H_
#define YAML_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"
#include "world_data_source_base.h"
#include "world_data_source_base.h"

#ifdef HAVE_YAML

#include "engine/structs/flag_data.h"
#include "description.h"

#include <yaml-cpp/yaml.h>
#include <string>
#include <map>
#include <vector>
#include <memory>

class ZoneData;
class RoomData;
class CharData;
struct Trigger;
class CObjectPrototype;

namespace utils {
	class ThreadPool;
}

namespace world_loader
{

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

	void LoadZones() override;
	void LoadTriggers() override;
	void LoadRooms() override;
	void LoadMobs() override;
	void LoadObjects() override;

	// Save methods (YAML is read-only for now)
	void SaveZone(int zone_rnum) override;
	bool SaveTriggers(int zone_rnum, int specific_vnum, int notify_level) override;
	void SaveRooms(int zone_rnum, int specific_vnum = -1) override;
	void SaveMobs(int zone_rnum, int specific_vnum = -1) override;
	void SaveObjects(int zone_rnum, int specific_vnum = -1) override;

private:
	// Initialize dictionaries
	bool LoadDictionaries();
	
	// Load world configuration (line endings, etc)
	bool LoadWorldConfig();

	// Get list of zone vnums from index.yaml
	std::vector<int> GetZoneList();
	std::vector<int> GetMobList();
	std::vector<int> GetObjectList();
	std::vector<int> GetTriggerList();

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
	std::vector<std::string> ConvertFlagsToNames(const FlagData &flags, const std::string &dict_name) const;
	std::string ReverseLookupEnum(const std::string &dict_name, int value) const;
	bool WriteYamlAtomic(const std::string &filepath, const YAML::Node &node) const;

	// Parallel loading methods (only used when m_num_threads > 1)
	void LoadZonesParallel();
	void LoadTriggersParallel();
	void LoadRoomsParallel();
	void LoadMobsParallel();
	void LoadObjectsParallel();

	// Worker functions (thread-safe, parse single file)
	ZoneData ParseZoneFile(const std::string &file_path);
	Trigger* ParseTriggerFile(const std::string &file_path);
	RoomData* ParseRoomFile(const std::string &file_path, int zone_rnum, LocalDescriptionIndex &local_index, size_t &local_desc_idx);
	CharData ParseMobFile(const std::string &file_path);
	CObjectPrototype* ParseObjectFile(const std::string &file_path);

	// Helper: get configured thread count from runtime config
	size_t GetConfiguredThreadCount() const;

	std::string m_world_dir;
	bool m_dictionaries_loaded = false;
	bool m_convert_lf_to_crlf = false;  // Convert LF to CR+LF for DOS line endings

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
