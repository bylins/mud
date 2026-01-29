// Part of Bylins http://www.mud.ru
// YAML world data source - loads world from YAML files

#ifndef YAML_WORLD_DATA_SOURCE_H_
#define YAML_WORLD_DATA_SOURCE_H_

#include "world_data_source.h"
#include "world_data_source_base.h"
#include "world_data_source_base.h"

#ifdef HAVE_YAML

#include "engine/structs/flag_data.h"

#include <yaml-cpp/yaml.h>
#include <string>
#include <map>
#include <vector>

class ZoneData;
class RoomData;
class CharData;
class CObjectPrototype;
class Trigger;
struct reset_com;

namespace world_loader
{

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
	void SaveTriggers(int zone_rnum) override;
	void SaveRooms(int zone_rnum) override;
	void SaveMobs(int zone_rnum) override;
	void SaveObjects(int zone_rnum) override;

private:
	// Initialize dictionaries
	bool LoadDictionaries();

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

	// Save helpers - convert KOI8-R to UTF-8 for writing
	// Reverse lookup helpers for saving
	std::string ReverseLookupEnum(const std::string &dict_name, int value) const;
	std::vector<std::string> ConvertFlagsToNames(const FlagData &flags, const std::string &dict_name) const;
	std::vector<std::string> ConvertTriggerTypesToNames(long trigger_type) const;

	std::string ConvertToUtf8(const std::string &koi8r_str) const;

	// Atomic file writing with .new/.old backup
	void WriteYamlAtomic(const std::string &filename, const YAML::Node &node) const;

	// Update index.yaml with zone/mob/obj/trigger vnum
	void UpdateIndexYaml(const std::string &section, int vnum) const;

	// Runtime Б├▓ YAML converters
	YAML::Node ZoneToYaml(const ZoneData &zone) const;
	YAML::Node ZoneCommandToYaml(const struct reset_com &cmd) const;
	YAML::Node RoomToYaml(const RoomData *room) const;
	YAML::Node MobToYaml(const CharData &mob) const;
	YAML::Node ObjectToYaml(const CObjectPrototype *obj) const;
	YAML::Node TriggerToYaml(const Trigger *trig) const;

	std::string m_world_dir;
	bool m_dictionaries_loaded = false;
};

// Factory function for creating YAML data source
std::unique_ptr<IWorldDataSource> CreateYamlDataSource(const std::string &world_dir);

} // namespace world_loader

#endif // HAVE_YAML

#endif // YAML_WORLD_DATA_SOURCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
