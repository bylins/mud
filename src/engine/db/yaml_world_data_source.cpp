// Part of Bylins http://www.mud.ru
// YAML world data source implementation

#ifdef HAVE_YAML

#include "yaml_world_data_source.h"
#include "dictionary_loader.h"
#include "db.h"
#include "obj_prototypes.h"
#include "global_objects.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/utils_string.h"
#include "engine/entities/zone.h"
#include "engine/entities/room_data.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/db/description.h"
#include "engine/structs/extra_description.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/scripting/dg_olc.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/skills/skills.h"
#include "utils/thread_pool.h"

#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>

// External declarations
extern ZoneTable &zone_table;
extern IndexData **trig_index;
extern int top_of_trigt;
extern Rooms &world;
extern RoomRnum top_of_world;
extern IndexData *mob_index;
extern MobRnum top_of_mobt;
extern CharData *mob_proto;

namespace world_loader
{

// Convert dictionary index to bitvector flag value
// Handles multi-plane flags (plane 0: bits 0-29, plane 1: bits 30-59, plane 2: bits 60-89)
inline Bitvector IndexToBitvector(long idx)
{
	if (idx < 0)
		return 0;
	if (idx < 30)
		return 1u << idx;
	if (idx < 60)
		return kIntOne | (1u << (idx - 30));
	if (idx < 90)
		return kIntTwo | (1u << (idx - 60));
	return 1u << idx;
}

// ============================================================================
// YamlWorldDataSource implementation
// ============================================================================

YamlWorldDataSource::YamlWorldDataSource(const std::string &world_dir)
	: m_world_dir(world_dir)
{
}

// Helper: get configured thread count from runtime config
size_t YamlWorldDataSource::GetConfiguredThreadCount() const
{
	size_t configured = runtime_config.yaml_threads();
	if (configured == 0)
	{
		size_t hw_threads = std::thread::hardware_concurrency();
		return (hw_threads > 0) ? hw_threads : 1;
	}
	return configured;
}

bool YamlWorldDataSource::LoadDictionaries()
{
	if (m_dictionaries_loaded)
	{
		return true;
	}

	std::string dict_dir = m_world_dir + "/dictionaries";
	if (!DictionaryManager::Instance().LoadDictionaries(dict_dir))
	{
		log("SYSERR: Failed to load dictionaries from %s", dict_dir.c_str());
		return false;
	}

	m_dictionaries_loaded = true;
	return true;
}

std::vector<int> YamlWorldDataSource::GetZoneList()
{
	std::vector<int> zones;
	std::string index_path = m_world_dir + "/zones/index.yaml";

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
	catch (const YAML::Exception &e)
	{
		log("SYSERR: Failed to load zone index '%s': %s", index_path.c_str(), e.what());
	}

	return zones;
}

std::vector<int> YamlWorldDataSource::GetMobList()
{
	std::vector<int> mobs;
	std::string index_path = m_world_dir + "/mobs/index.yaml";

	try
	{
		YAML::Node root = YAML::LoadFile(index_path);
		if (root["mobs"] && root["mobs"].IsSequence())
		{
			for (const auto &mob_node : root["mobs"])
			{
				mobs.push_back(mob_node.as<int>());
			}
		}
	}
	catch (const YAML::Exception &e)
	{
		// Index file is optional, if missing load all mobs
		return mobs;
	}

	return mobs;
}

std::vector<int> YamlWorldDataSource::GetObjectList()
{
	std::vector<int> objects;
	std::string index_path = m_world_dir + "/objects/index.yaml";

	try
	{
		YAML::Node root = YAML::LoadFile(index_path);
		if (root["objects"] && root["objects"].IsSequence())
		{
			for (const auto &obj_node : root["objects"])
			{
				objects.push_back(obj_node.as<int>());
			}
		}
	}
	catch (const YAML::Exception &e)
	{
		// Index file is optional, if missing load all objects
		return objects;
	}

	return objects;
}

std::vector<int> YamlWorldDataSource::GetTriggerList()
{
	std::vector<int> triggers;
	std::string index_path = m_world_dir + "/triggers/index.yaml";

	try
	{
		YAML::Node root = YAML::LoadFile(index_path);
		if (root["triggers"] && root["triggers"].IsSequence())
		{
			for (const auto &trigger_node : root["triggers"])
			{
				triggers.push_back(trigger_node.as<int>());
			}
		}
	}
	catch (const YAML::Exception &e)
	{
		// Index file is optional, if missing load all triggers
		return triggers;
	}

	return triggers;
}

std::string YamlWorldDataSource::ConvertToKoi8r(const std::string &utf8_str) const
{
	if (utf8_str.empty())
	{
		return "";
	}

	static char buffer[65536];
	char *input = const_cast<char*>(utf8_str.c_str());
	char *output = buffer;
	utf8_to_koi(input, output);
	return buffer;
}

std::string YamlWorldDataSource::GetText(const YAML::Node &node, const std::string &key, const std::string &default_val) const
{
	if (node[key])
	{
		// YAML files are already in KOI8-R, no conversion needed
		return node[key].as<std::string>();
	}
	return default_val;
}

int YamlWorldDataSource::GetInt(const YAML::Node &node, const std::string &key, int default_val) const
{
	if (node[key])
	{
		return node[key].as<int>();
	}
	return default_val;
}

long YamlWorldDataSource::GetLong(const YAML::Node &node, const std::string &key, long default_val) const
{
	if (node[key])
	{
		return node[key].as<long>();
	}
	return default_val;
}

FlagData YamlWorldDataSource::ParseFlags(const YAML::Node &node, const std::string &dict_name) const
{
	FlagData flags;

	if (!node || !node.IsSequence())
	{
		return flags;
	}

	auto &dm = DictionaryManager::Instance();

	for (const auto &flag_node : node)
	{
		std::string flag_name = flag_node.as<std::string>();

		// Handle UNUSED_XX flags directly
		if (flag_name.rfind("UNUSED_", 0) == 0)
		{
			int bit = std::stoi(flag_name.substr(7));
			size_t plane = bit / 30;
			int bit_in_plane = bit % 30;
			flags.set_flag(plane, 1 << bit_in_plane);
			continue;
		}

		long bit_pos = dm.Lookup(dict_name, flag_name, -1);
		if (bit_pos >= 0)
		{
			size_t plane = bit_pos / 30;
			int bit = bit_pos % 30;
			flags.set_flag(plane, 1 << bit);
		}
	}

	return flags;
}

int YamlWorldDataSource::ParseEnum(const YAML::Node &node, const std::string &dict_name, int default_val) const
{
	if (!node)
	{
		return default_val;
	}

	std::string name = node.as<std::string>();
	auto &dm = DictionaryManager::Instance();
	long val = dm.Lookup(dict_name, name, default_val);
	return static_cast<int>(val);
}

int YamlWorldDataSource::ParsePosition(const YAML::Node &node) const
{
	return ParseEnum(node, "positions", static_cast<int>(EPosition::kStand));
}

int YamlWorldDataSource::ParseGender(const YAML::Node &node, int default_val) const
{
	return ParseEnum(node, "genders", default_val);
}

// ============================================================================
// Zone Loading
// ============================================================================

// Parse single zone file (thread-safe worker function)
ZoneData YamlWorldDataSource::ParseZoneFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);

	// Extract vnum from filepath (pattern: .../zones/VNUM/zone.yaml)
	size_t last_slash = file_path.rfind("/zone.yaml");
	size_t prev_slash = file_path.rfind('/', last_slash - 1);
	std::string vnum_str = file_path.substr(prev_slash + 1, last_slash - prev_slash - 1);
	int zone_vnum = std::atoi(vnum_str.c_str());

	ZoneData zone;
	zone.vnum = GetInt(root, "vnum", zone_vnum);
	zone.name = GetText(root, "name", "Unknown Zone");
	zone.group = GetInt(root, "zone_group", 1);
	if (zone.group == 0) zone.group = 1;

	// Read metadata subfields
	if (root["metadata"])
	{
		zone.comment = GetText(root["metadata"], "comment");
		zone.location = GetText(root["metadata"], "location");
		zone.author = GetText(root["metadata"], "author");
		zone.description = GetText(root["metadata"], "description");
	}
	zone.top = GetInt(root, "top_room", zone.vnum * 100 + 99);
	zone.lifespan = GetInt(root, "lifespan", 30);
	zone.reset_mode = GetInt(root, "reset_mode", 2);
	zone.reset_idle = GetInt(root, "reset_idle", 0) != 0;
	zone.type = GetInt(root, "zone_type", 0);
	zone.level = GetInt(root, "mode", 0);
	zone.entrance = GetInt(root, "entrance", 0);

	// Initialize runtime fields (uses base class method)
	int under_construction = GetInt(root, "under_construction", 0);
	InitializeZoneRuntimeFields(zone, under_construction);

	// Load zone commands
	if (root["commands"])
	{
		LoadZoneCommands(zone, root["commands"]);
	}
	else
	{
		CREATE(zone.cmd, 1);
		zone.cmd[0].command = 'S';
	}

	// Load zone groups (typeA and typeB)
	if (root["typeA_zones"] && root["typeA_zones"].IsSequence())
	{
		zone.typeA_count = root["typeA_zones"].size();
		CREATE(zone.typeA_list, zone.typeA_count);
		int i = 0;
		for (const auto &z : root["typeA_zones"])
		{
			zone.typeA_list[i++] = z.as<int>();
		}
	}

	if (root["typeB_zones"] && root["typeB_zones"].IsSequence())
	{
		zone.typeB_count = root["typeB_zones"].size();
		CREATE(zone.typeB_list, zone.typeB_count);
		CREATE(zone.typeB_flag, zone.typeB_count);
		int i = 0;
		for (const auto &z : root["typeB_zones"])
		{
			zone.typeB_list[i] = z.as<int>();
			zone.typeB_flag[i] = false;
			i++;
		}
	}

	return zone;
}

// Parallel zone loading
void YamlWorldDataSource::LoadZonesParallel()
{
	std::vector<int> zone_vnums = GetZoneList();
	if (zone_vnums.empty())
	{
		log("No zones found in YAML index.");
		return;
	}

	int zone_count = zone_vnums.size();
	zone_table.reserve(zone_count + dungeons::kNumberOfZoneDungeons);
	zone_table.resize(zone_count);
	log("   %d zones, %zd bytes.", zone_count, sizeof(ZoneData) * zone_count);

	// Build vnum to index mapping
	std::map<int, size_t> vnum_to_idx;
	for (size_t i = 0; i < zone_vnums.size(); ++i)
	{
		vnum_to_idx[zone_vnums[i]] = i;
	}

	// Distribute zones into batches
	auto batches = utils::DistributeBatches(zone_vnums, m_num_threads);
	std::atomic<int> error_count{0};

	// Launch parallel loading
	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &vnum_to_idx, &error_count]() {
			for (size_t i = 0; i < batches[thread_id].size(); ++i)
			{
				int zone_vnum = batches[thread_id][i];
				std::string zone_path = m_world_dir + "/zones/" + std::to_string(zone_vnum) + "/zone.yaml";

				try
				{
					size_t zone_idx = vnum_to_idx.at(zone_vnum);
					zone_table[zone_idx] = ParseZoneFile(zone_path);
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load zone %d from '%s': %s", zone_vnum, zone_path.c_str(), e.what());
					error_count++;
				}
				catch (const std::exception &e)
				{
					log("SYSERR: Failed to load zone %d from '%s': %s", zone_vnum, zone_path.c_str(), e.what());
					error_count++;
				}
			}
		}));
	}

	// Wait for all tasks
	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		log("FATAL: %d zone(s) failed to load. Aborting.", error_count.load());
		exit(1);
	}

	log("Loaded %d zones from YAML (parallel).", zone_count);
}

void YamlWorldDataSource::LoadZones()
{
	log("Loading zones from YAML files.");

	// Load dictionaries first (sequential, writes to singleton)
	if (!LoadDictionaries())
	{
		log("FATAL: Cannot continue without dictionaries. Aborting.");
		exit(1);
	}

	// Get thread count and create thread pool
	m_num_threads = GetConfiguredThreadCount();
	log("YAML loading with %zu threads", m_num_threads);
	m_thread_pool = std::make_unique<utils::ThreadPool>(m_num_threads);

	// Parallel load zones
	LoadZonesParallel();
}

void YamlWorldDataSource::LoadZoneCommands(ZoneData &zone, const YAML::Node &commands_node)
{
	if (!commands_node.IsSequence())
	{
		CREATE(zone.cmd, 1);
		zone.cmd[0].command = 'S';
		return;
	}

	int cmd_count = commands_node.size();
	CREATE(zone.cmd, cmd_count + 1);

	int idx = 0;
	for (const auto &cmd_node : commands_node)
	{
		struct reset_com &cmd = zone.cmd[idx];

		cmd.command = '*';
		cmd.if_flag = GetInt(cmd_node, "if_flag", 0);
		cmd.arg1 = 0;
		cmd.arg2 = 0;
		cmd.arg3 = 0;
		cmd.arg4 = -1;
		cmd.sarg1 = nullptr;
		cmd.sarg2 = nullptr;
		cmd.line = 0;

		std::string cmd_type = GetText(cmd_node, "type", "");

		if (cmd_type == "LOAD_MOB" || cmd_type == "M")
		{
			cmd.command = 'M';
			cmd.arg1 = GetInt(cmd_node, "mob_vnum");
			cmd.arg2 = GetInt(cmd_node, "max_world");
			cmd.arg3 = GetInt(cmd_node, "room_vnum");
			cmd.arg4 = GetInt(cmd_node, "max_room", -1);
		}
		else if (cmd_type == "LOAD_OBJ" || cmd_type == "O")
		{
			cmd.command = 'O';
			cmd.arg1 = GetInt(cmd_node, "obj_vnum");
			cmd.arg2 = GetInt(cmd_node, "max");
			cmd.arg3 = GetInt(cmd_node, "room_vnum");
			cmd.arg4 = GetInt(cmd_node, "load_prob", -1);
		}
		else if (cmd_type == "GIVE_OBJ" || cmd_type == "G")
		{
			cmd.command = 'G';
			cmd.arg1 = GetInt(cmd_node, "obj_vnum");
			cmd.arg2 = GetInt(cmd_node, "max");
			cmd.arg3 = -1;
			cmd.arg4 = GetInt(cmd_node, "load_prob", -1);
		}
		else if (cmd_type == "EQUIP_MOB" || cmd_type == "E")
		{
			cmd.command = 'E';
			cmd.arg1 = GetInt(cmd_node, "obj_vnum");
			cmd.arg2 = GetInt(cmd_node, "max");
			cmd.arg3 = GetInt(cmd_node, "wear_pos");
			cmd.arg4 = GetInt(cmd_node, "load_prob", -1);
		}
		else if (cmd_type == "PUT_OBJ" || cmd_type == "P")
		{
			cmd.command = 'P';
			cmd.arg1 = GetInt(cmd_node, "obj_vnum");
			cmd.arg2 = GetInt(cmd_node, "max");
			cmd.arg3 = GetInt(cmd_node, "container_vnum");
			cmd.arg4 = GetInt(cmd_node, "load_prob", -1);
		}
		else if (cmd_type == "DOOR" || cmd_type == "D")
		{
			cmd.command = 'D';
			cmd.arg1 = GetInt(cmd_node, "room_vnum");
			cmd.arg2 = GetInt(cmd_node, "direction");
			cmd.arg3 = GetInt(cmd_node, "state");
		}
		else if (cmd_type == "REMOVE_OBJ" || cmd_type == "R")
		{
			cmd.command = 'R';
			cmd.arg1 = GetInt(cmd_node, "room_vnum");
			cmd.arg2 = GetInt(cmd_node, "obj_vnum");
		}
		else if (cmd_type == "TRIGGER" || cmd_type == "T")
		{
			cmd.command = 'T';
			cmd.arg1 = GetInt(cmd_node, "trigger_type");
			cmd.arg2 = GetInt(cmd_node, "trigger_vnum");
			cmd.arg3 = GetInt(cmd_node, "room_vnum", -1);
		}
		else if (cmd_type == "VARIABLE" || cmd_type == "V")
		{
			cmd.command = 'V';
			cmd.arg1 = GetInt(cmd_node, "trigger_type");
			cmd.arg2 = GetInt(cmd_node, "context");
			cmd.arg3 = GetInt(cmd_node, "room_vnum");
			std::string var_name = GetText(cmd_node, "var_name");
			std::string var_value = GetText(cmd_node, "var_value");
			if (!var_name.empty()) cmd.sarg1 = str_dup(var_name.c_str());
			if (!var_value.empty()) cmd.sarg2 = str_dup(var_value.c_str());
		}
		else if (cmd_type == "EXTRACT_MOB" || cmd_type == "Q")
		{
			cmd.command = 'Q';
			cmd.arg1 = GetInt(cmd_node, "mob_vnum");
		cmd.if_flag = 0; // Legacy loader forces if_flag = 0 for EXTRACT_MOB
		}
		else if (cmd_type == "FOLLOW" || cmd_type == "F")
		{
			cmd.command = 'F';
			cmd.arg1 = GetInt(cmd_node, "room_vnum");
			cmd.arg2 = GetInt(cmd_node, "leader_mob_vnum");
			cmd.arg3 = GetInt(cmd_node, "follower_mob_vnum");
		}

		idx++;
	}

	zone.cmd[idx].command = 'S';
}

// ============================================================================
// Trigger Loading
// ============================================================================

// Parse single trigger file (thread-safe worker function)
Trigger* YamlWorldDataSource::ParseTriggerFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);

	// Extract vnum from filename
	size_t last_slash = file_path.find_last_of('/');
	size_t last_dot = file_path.find_last_of('.');
	std::string vnum_str = file_path.substr(last_slash + 1, last_dot - last_slash - 1);
	(void)std::atoi(vnum_str.c_str());  // vnum extracted but not used

	std::string name = GetText(root, "name", "");
	int attach_type = ParseEnum(root["attach_type"], "attach_types", 0);

	// Parse trigger types
	long trigger_type = 0;
	if (root["trigger_types"] && root["trigger_types"].IsSequence())
	{
		for (const auto &type_node : root["trigger_types"])
		{
			std::string type_str = type_node.as<std::string>();
			auto &dm = DictionaryManager::Instance();
			long type_bit = dm.Lookup("trigger_types", type_str, -1);
			if (type_bit >= 0)
			{
				trigger_type |= (1L << type_bit);
			}
		}
	}

	int narg = GetInt(root, "narg", 0);
	std::string arglist = GetText(root, "arglist", "");
	std::string script = GetText(root, "script", "");

	// Create trigger (note: rnum will be assigned during merge)
	auto trig = new Trigger(-1, std::move(name), static_cast<byte>(attach_type), trigger_type);
	GET_TRIG_NARG(trig) = narg;
	trig->arglist = arglist;

	// Parse script into cmdlist
	ParseTriggerScript(trig, script);

	// Note: vnum will be passed separately in thread results
	return trig;
}

// Parallel trigger loading
void YamlWorldDataSource::LoadTriggersParallel()
{
	std::vector<int> trigger_vnums = GetTriggerList();
	if (trigger_vnums.empty())
	{
		log("No triggers found in YAML index.");
		return;
	}

	int trig_count = trigger_vnums.size();
	log("   %d triggers.", trig_count);

	// Distribute triggers into batches
	auto batches = utils::DistributeBatches(trigger_vnums, m_num_threads);

	// Thread-local results (each thread collects its triggers)
	std::vector<std::vector<std::pair<int, Trigger*>>> thread_results(batches.size());
	std::atomic<int> error_count{0};

	// Launch parallel loading
	std::vector<std::future<void>> futures;
	log("DEBUG: Starting %zu trigger loading threads", batches.size());
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &thread_results, &error_count]() {
			log("DEBUG: Thread %zu started, processing %zu triggers", thread_id, batches[thread_id].size());
			for (int vnum : batches[thread_id])
			{
				std::string filepath = m_world_dir + "/triggers/" + std::to_string(vnum) + ".yaml";
				try
				{
					log("DEBUG: Thread %zu parsing trigger %d", thread_id, vnum);
					Trigger* trig = ParseTriggerFile(filepath);
					thread_results[thread_id].emplace_back(vnum, trig);
					log("DEBUG: Thread %zu completed trigger %d", thread_id, vnum);
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load trigger from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
				catch (const std::exception &e)
				{
					log("SYSERR: Failed to load trigger from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
			}
			log("DEBUG: Thread %zu finished all triggers", thread_id);
		}));
	}

	// Wait for all tasks
	log("DEBUG: Waiting for all trigger threads to complete");
	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		log("FATAL: %d trigger(s) failed to load. Aborting.", error_count.load());
		exit(1);
	}

	// Merge results into trig_index (sequential)
	CREATE(trig_index, trig_count);
	top_of_trigt = 0;

	for (auto &results : thread_results)
	{
		for (auto &[vnum, trig] : results)
		{
			// Assign rnum
			trig->set_rnum(top_of_trigt);
			// Create index entry
			CreateTriggerIndex(vnum, trig);
		}
	}

	log("Loaded %d triggers from YAML (parallel).", top_of_trigt);
}

void YamlWorldDataSource::LoadTriggers()
{
	log("Loading triggers from YAML files.");

	// Dictionaries already loaded in LoadZones, thread pool already created
	LoadTriggersParallel();
}

// ============================================================================
// Room Loading
// ============================================================================

// Parse single room file (thread-safe worker function)
RoomData* YamlWorldDataSource::ParseRoomFile(const std::string &file_path, int zone_rnum, LocalDescriptionIndex &local_index, size_t &local_desc_idx)
{
	YAML::Node root = YAML::LoadFile(file_path);

	// Extract vnum from filename
	size_t last_slash = file_path.find_last_of('/');
	size_t last_dot = file_path.find_last_of('.');

	std::string filename = file_path.substr(last_slash + 1, last_dot - last_slash - 1);
	int rel_num = std::atoi(filename.c_str());

	// Extract zone vnum from path
	size_t zone_start = file_path.rfind("/zones/") + 7;
	size_t zone_end = file_path.find("/", zone_start);
	int zone_vnum = std::atoi(file_path.substr(zone_start, zone_end - zone_start).c_str());
	int vnum = zone_vnum * 100 + rel_num;

	auto room = new RoomData;
	room->vnum = vnum;
	room->zone_rn = zone_rnum;

	std::string name = GetText(root, "name", "Untitled Room");
	if (!name.empty()) { name[0] = UPPER(name[0]); }
	room->set_name(name);

	std::string description = GetText(root, "description", "");
	if (!description.empty())
	{
		// Add to thread-local index (no mutex needed!)
		local_desc_idx = local_index.add(description);
		// description_num will be set in merge phase
		room->description_num = 0;
	}
	else
	{
		local_desc_idx = 0;  // No description
		room->description_num = 0;
	}

	// Parse flags and sector
	if (root["flags"] && root["flags"].IsSequence())
	{
		auto &dm = DictionaryManager::Instance();
		for (const auto &flag_node : root["flags"])
		{
			std::string flag_name = flag_node.as<std::string>();
			long flag_val = dm.Lookup("room_flags", flag_name, -1);
			if (flag_val >= 0)
			{
				room->set_flag(static_cast<ERoomFlag>(IndexToBitvector(flag_val)));
			}
		}
	}
	room->sector_type = ParseEnum(root["sector"], "sectors", 0);

	// Load exits
	if (root["exits"])
	{
		LoadRoomExits(room, root["exits"], vnum);
	}

	// Load extra descriptions
	if (root["extra_descriptions"])
	{
		LoadRoomExtraDescriptions(room, root["extra_descriptions"]);
	}

	return room;
}

// Parallel room loading
// Parallel room loading
void YamlWorldDataSource::LoadRoomsParallel()
{
	// Creating empty world with kNowhere room (dummy room 0) - same as Legacy loader
	world.push_back(new RoomData);
	top_of_world = kNowhere;

	std::vector<int> zone_vnums = GetZoneList();
	if (zone_vnums.empty())
	{
		log("No zones found in YAML index.");
		return;
	}
	std::set<int> enabled_zones(zone_vnums.begin(), zone_vnums.end());

	// Collect all room files
	std::vector<std::pair<int, std::string>> room_files;
	std::string zones_dir = m_world_dir + "/zones";

	namespace fs = std::filesystem;
	for (const auto &zone_entry : fs::directory_iterator(zones_dir))
	{
		if (!zone_entry.is_directory()) continue;

		std::string zone_dir_name = zone_entry.path().filename().string();
		if (zone_dir_name.empty() || !std::isdigit(zone_dir_name[0])) continue;

		int zone_vnum = std::stoi(zone_dir_name);
		if (enabled_zones.find(zone_vnum) == enabled_zones.end()) continue;

		std::string rooms_dir = zone_entry.path().string() + "/rooms";
		if (!fs::exists(rooms_dir)) continue;

		for (const auto &room_entry : fs::directory_iterator(rooms_dir))
		{
			if (!room_entry.is_regular_file()) continue;
			std::string filename = room_entry.path().filename().string();
			if (filename.size() < 6 || filename.substr(filename.size() - 5) != ".yaml") continue;

			int rel_num = std::atoi(filename.substr(0, filename.size() - 5).c_str());
			int vnum = zone_vnum * 100 + rel_num;
			room_files.emplace_back(vnum, room_entry.path().string());
		}
	}

	std::sort(room_files.begin(), room_files.end());

	int room_count = room_files.size();
	if (room_count == 0)
	{
		log("No rooms found in YAML files.");
		return;
	}

	log("   %d rooms, %zd bytes.", room_count, sizeof(RoomData) * room_count);

	// Distribute rooms into batches for parallel parsing
	auto batches = utils::DistributeBatches(room_files, m_num_threads);
	std::atomic<int> error_count{0};

	// Launch parallel loading with thread-local description indices
	std::vector<std::future<ParsedRoomBatch>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &error_count]() -> ParsedRoomBatch {
			// LOCAL variable (not thread_local!)
			LocalDescriptionIndex local_index;
			std::vector<std::tuple<int, RoomData*, size_t>> parsed_rooms;

			for (const auto &[vnum, filepath] : batches[thread_id])
			{
				try
				{
					// Calculate zone_rnum from vnum
					ZoneRnum zone_rn = 0;
					while (zone_rn < static_cast<ZoneRnum>(zone_table.size()) &&
					       vnum > zone_table[zone_rn].top)
					{
						zone_rn++;
					}

					size_t local_desc_idx = 0;
					RoomData* room = ParseRoomFile(filepath, zone_rn, local_index, local_desc_idx);

					// Store vnum, room, and local description index
					parsed_rooms.push_back(std::make_tuple(vnum, room, local_desc_idx));
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load room from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
				catch (const std::exception &e)
				{
					log("SYSERR: Failed to load room from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
			}

			return ParsedRoomBatch{std::move(local_index), std::move(parsed_rooms)};
		}));
	}

	// Collect results from all threads
	std::vector<ParsedRoomBatch> parsed_batches;
	parsed_batches.reserve(futures.size());
	for (auto &future : futures)
	{
		parsed_batches.push_back(future.get());
	}

	if (error_count > 0)
	{
		log("FATAL: %d room(s) failed to load. Aborting.", error_count.load());
		exit(1);
	}

	// Merge descriptions from all batches
	auto &global_descriptions = GlobalObjects::descriptions();
	std::vector<std::vector<size_t>> local_to_global(parsed_batches.size());

	for (size_t batch_id = 0; batch_id < parsed_batches.size(); ++batch_id)
	{
		local_to_global[batch_id] = global_descriptions.merge(parsed_batches[batch_id].descriptions);
	}

	// Collect all rooms with their batch IDs for description reindexing
	std::vector<std::tuple<int, RoomData*, size_t, size_t>> all_rooms;  // (vnum, room, batch_id, local_desc_idx)
	for (size_t batch_id = 0; batch_id < parsed_batches.size(); ++batch_id)
	{
		for (auto &[vnum, room, local_desc_idx] : parsed_batches[batch_id].rooms)
		{
			all_rooms.push_back(std::make_tuple(vnum, room, batch_id, local_desc_idx));
		}
	}

	// Sort rooms by vnum (CRITICAL for correct indexing)
	std::sort(all_rooms.begin(), all_rooms.end(),
		[](const auto &a, const auto &b) { return std::get<0>(a) < std::get<0>(b); });

	// Add rooms to world vector in sorted order (sequential, using push_back like Legacy)
	for (auto &[vnum, room, batch_id, local_desc_idx] : all_rooms)
	{
		// Update room's description_num with global index
		// local_desc_idx is 1-based (0 = no description, 1 = first description)
		// local_to_global is 0-indexed vector
		if (local_desc_idx > 0)
		{
			size_t local_idx_0based = local_desc_idx - 1;
			if (local_idx_0based < local_to_global[batch_id].size())
			{
				room->description_num = local_to_global[batch_id][local_idx_0based];
			}
		}

		world.push_back(room);
	}

	top_of_world = world.size() - 1;
	log("   Merged %zu unique room descriptions from %zu threads.", global_descriptions.size(), parsed_batches.size());

	// Update zone_table.RnumRoomsLocation (sequential post-processing)
	for (size_t i = 0; i < world.size(); ++i)
	{
		if (world[i])
		{
			ZoneRnum zone_rn = world[i]->zone_rn;
			if (zone_rn < static_cast<ZoneRnum>(zone_table.size()))
			{
				if (zone_table[zone_rn].RnumRoomsLocation.first == -1)
				{
					zone_table[zone_rn].RnumRoomsLocation.first = i;
				}
				zone_table[zone_rn].RnumRoomsLocation.second = i;
			}
		}
	}
}
void YamlWorldDataSource::LoadRooms()
{
	log("Loading rooms from YAML files.");

	// Dictionaries already loaded in LoadZones, thread pool already created
	LoadRoomsParallel();
}

void YamlWorldDataSource::LoadRoomExits(RoomData *room, const YAML::Node &exits_node, int room_vnum)
{
	if (!exits_node.IsSequence())
	{
		return;
	}

	for (const auto &exit_node : exits_node)
	{
		int dir = ParseEnum(exit_node["direction"], "directions", 0);
		if (dir < 0 || dir >= EDirection::kMaxDirNum)
		{
			log("SYSERR: Room %d has invalid exit direction %d, skipping.", room_vnum, dir);
			continue;
		}

		auto exit_data = std::make_shared<ExitData>();

		exit_data->to_room(GetInt(exit_node, "to_room", -1));
		exit_data->key = GetInt(exit_node, "key", -1);
		exit_data->lock_complexity = GetInt(exit_node, "lock_complexity", 0);

		std::string desc = GetText(exit_node, "description", "");
		if (!desc.empty()) exit_data->general_description = desc;

		std::string keywords = GetText(exit_node, "keywords", "");
		if (!keywords.empty()) exit_data->set_keywords(keywords);

		exit_data->exit_info = GetInt(exit_node, "exit_flags", 0);

		room->dir_option_proto[dir] = exit_data;
	}
}

void YamlWorldDataSource::LoadRoomExtraDescriptions(RoomData *room, const YAML::Node &extras_node)
{
	if (!extras_node.IsSequence())
	{
		return;
	}

	for (const auto &ed_node : extras_node)
	{
		std::string keywords = GetText(ed_node, "keywords", "");
		std::string description = GetText(ed_node, "description", "");

		auto ex_desc = std::make_shared<ExtraDescription>();
		ex_desc->set_keyword(keywords);
		ex_desc->set_description(description);
		ex_desc->next = room->ex_description;
		room->ex_description = ex_desc;
	}
}

// ============================================================================
// Mob Loading
// ============================================================================

// Parse single mob file (thread-safe worker function)
CharData YamlWorldDataSource::ParseMobFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);

	// Note: vnum is passed separately by caller, extracted there
	CharData mob;
	mob.player_specials = player_special_data::s_for_mobiles;
	mob.SetNpcAttribute(true);
	mob.player_specials->saved.NameGod = 1001;
	mob.set_move(100);
	mob.set_max_move(100);

	// Names
	YAML::Node names = root["names"];
	if (names)
	{
		mob.SetCharAliases(GetText(names, "aliases"));
		mob.set_npc_name(GetText(names, "nominative"));
		mob.player_data.PNames[ECase::kNom] = GetText(names, "nominative");
		mob.player_data.PNames[ECase::kGen] = GetText(names, "genitive");
		mob.player_data.PNames[ECase::kDat] = GetText(names, "dative");
		mob.player_data.PNames[ECase::kAcc] = GetText(names, "accusative");
		mob.player_data.PNames[ECase::kIns] = GetText(names, "instrumental");
		mob.player_data.PNames[ECase::kPre] = GetText(names, "prepositional");
	}

	// Descriptions
	YAML::Node descs = root["descriptions"];
	if (descs)
	{
		mob.player_data.long_descr = utils::colorCAP(GetText(descs, "short_desc"));
		mob.player_data.description = GetText(descs, "long_desc");
	}

	// Base parameters
	GET_ALIGNMENT(&mob) = GetInt(root, "alignment", 0);

	// Stats
	YAML::Node stats = root["stats"];
	if (stats)
	{
		mob.set_level(GetInt(stats, "level", 1));
		GET_HR(&mob) = GetInt(stats, "hitroll_penalty", 20);
		GET_AC(&mob) = GetInt(stats, "armor", 100);

		YAML::Node hp = stats["hp"];
		if (hp)
		{
			mob.mem_queue.total = GetInt(hp, "dice_count", 1);
			mob.mem_queue.stored = GetInt(hp, "dice_size", 1);
			int hp_bonus = GetInt(hp, "bonus", 0);
			mob.set_hit(hp_bonus);
			mob.set_max_hit(0);
		}

		YAML::Node dmg = stats["damage"];
		if (dmg)
		{
			mob.mob_specials.damnodice = GetInt(dmg, "dice_count", 1);
			mob.mob_specials.damsizedice = GetInt(dmg, "dice_size", 1);
			mob.real_abils.damroll = GetInt(dmg, "bonus", 0);
		}
	}

	// Gold
	YAML::Node gold = root["gold"];
	if (gold)
	{
		mob.mob_specials.GoldNoDs = GetInt(gold, "dice_count", 0);
		mob.mob_specials.GoldSiDs = GetInt(gold, "dice_size", 0);
		mob.set_gold(GetInt(gold, "bonus", 0));
	}

	// Experience
	mob.set_exp(GetInt(root, "experience", 0));

	// Position
	YAML::Node pos = root["position"];
	if (pos)
	{
		mob.mob_specials.default_pos = static_cast<EPosition>(ParsePosition(pos["default"]));
		mob.SetPosition(static_cast<EPosition>(ParsePosition(pos["start"])));
	}
	else
	{
		mob.mob_specials.default_pos = EPosition::kStand;
		mob.SetPosition(EPosition::kStand);
	}

	// Sex
	mob.set_sex(static_cast<EGender>(ParseGender(root["sex"])));

	// Physical attributes
	GET_SIZE(&mob) = GetInt(root, "size", 0);
	GET_HEIGHT(&mob) = GetInt(root, "height", 0);
	GET_WEIGHT(&mob) = GetInt(root, "weight", 0);

	// E-spec attributes - set defaults, then override
	mob.set_str(11);
	mob.set_dex(11);
	mob.set_int(11);
	mob.set_wis(11);
	mob.set_con(11);
	mob.set_cha(11);
	if (root["attributes"])
	{
		YAML::Node attrs = root["attributes"];
		mob.set_str(GetInt(attrs, "strength", 11));
		mob.set_dex(GetInt(attrs, "dexterity", 11));
		mob.set_int(GetInt(attrs, "intelligence", 11));
		mob.set_wis(GetInt(attrs, "wisdom", 11));
		mob.set_con(GetInt(attrs, "constitution", 11));
		mob.set_cha(GetInt(attrs, "charisma", 11));
	}

	// Flags
	auto &dm = DictionaryManager::Instance();
	if (root["action_flags"] && root["action_flags"].IsSequence())
	{
		for (const auto &flag_node : root["action_flags"])
		{
			std::string flag_name = flag_node.as<std::string>();
			long flag_val = dm.Lookup("action_flags", flag_name, -1);
			if (flag_val >= 0)
			{
				mob.SetFlag(static_cast<EMobFlag>(flag_val));
			}
		}
	}

	if (root["affect_flags"] && root["affect_flags"].IsSequence())
	{
		for (const auto &flag_node : root["affect_flags"])
		{
			std::string flag_name = flag_node.as<std::string>();
			long flag_val = dm.Lookup("affect_flags", flag_name, -1);
			if (flag_val >= 0)
			{
				AFF_FLAGS(&mob).set(static_cast<Bitvector>(flag_val));
			}
		}
	}

	// Skills
	if (root["skills"] && root["skills"].IsSequence())
	{
		for (const auto &skill_node : root["skills"])
		{
			int skill_id = GetInt(skill_node, "skill_id", 0);
			int value = GetInt(skill_node, "value", 0);
			mob.set_skill(static_cast<ESkill>(skill_id), value);
		}
	}

	// Enhanced E-spec fields
	if (root["enhanced"])
	{
		YAML::Node enhanced = root["enhanced"];

		mob.set_str_add(GetInt(enhanced, "str_add", 0));
		mob.add_abils.hitreg = GetInt(enhanced, "hp_regen", 0);
		mob.add_abils.armour = GetInt(enhanced, "armour_bonus", 0);
		mob.add_abils.manareg = GetInt(enhanced, "mana_regen", 0);
		mob.add_abils.cast_success = GetInt(enhanced, "cast_success", 0);
		mob.add_abils.morale = GetInt(enhanced, "morale", 0);
		mob.add_abils.initiative_add = GetInt(enhanced, "initiative_add", 0);
		mob.add_abils.absorb = GetInt(enhanced, "absorb", 0);
		mob.add_abils.aresist = GetInt(enhanced, "aresist", 0);
		mob.add_abils.mresist = GetInt(enhanced, "mresist", 0);
		mob.add_abils.presist = GetInt(enhanced, "presist", 0);
		mob.mob_specials.attack_type = GetInt(enhanced, "bare_hand_attack", 0);
		mob.mob_specials.like_work = GetInt(enhanced, "like_work", 0);
		mob.mob_specials.MaxFactor = GetInt(enhanced, "max_factor", 0);
		mob.mob_specials.extra_attack = GetInt(enhanced, "extra_attack", 0);
		mob.set_remort(GetInt(enhanced, "mob_remort", 0));

		if (enhanced["special_bitvector"])
		{
			std::string special_bv = enhanced["special_bitvector"].as<std::string>();
			mob.mob_specials.npc_flags.from_string((char *)special_bv.c_str());
		}

		if (enhanced["role"])
		{
			std::string role_str = enhanced["role"].as<std::string>();
			CharData::role_t role(role_str);
			mob.set_role(role);
		}

		if (enhanced["resistances"] && enhanced["resistances"].IsSequence())
		{
			int idx = 0;
			for (const auto &val_node : enhanced["resistances"])
			{
				int value = val_node.as<int>();
				if (idx < static_cast<int>(mob.add_abils.apply_resistance.size()))
				{
					mob.add_abils.apply_resistance[idx] = value;
				}
				idx++;
			}
		}

		if (enhanced["saves"] && enhanced["saves"].IsSequence())
		{
			int idx = 0;
			for (const auto &val_node : enhanced["saves"])
			{
				int value = val_node.as<int>();
				if (idx < static_cast<int>(mob.add_abils.apply_saving_throw.size()))
				{
					mob.add_abils.apply_saving_throw[idx] = value;
				}
				idx++;
			}
		}

		if (enhanced["feats"] && enhanced["feats"].IsSequence())
		{
			for (const auto &feat_node : enhanced["feats"])
			{
				int feat_id = feat_node.as<int>();
				if (feat_id >= 0 && feat_id < static_cast<int>(mob.real_abils.Feats.size()))
				{
					mob.real_abils.Feats.set(feat_id);
				}
			}
		}

		if (enhanced["spells"] && enhanced["spells"].IsSequence())
		{
			for (const auto &spell_node : enhanced["spells"])
			{
				int spell_id = spell_node.as<int>();
				if (spell_id >= 0 && spell_id < static_cast<int>(mob.real_abils.SplKnw.size()))
				{
					mob.real_abils.SplKnw[spell_id] = 1;
				}
			}
		}

		if (enhanced["helpers"] && enhanced["helpers"].IsSequence())
		{
			for (const auto &helper_node : enhanced["helpers"])
			{
				int helper_vnum = helper_node.as<int>();
				mob.summon_helpers.push_back(helper_vnum);
			}
		}

		if (enhanced["destinations"] && enhanced["destinations"].IsSequence())
		{
			int idx = 0;
			for (const auto &dest_node : enhanced["destinations"])
			{
				int room_vnum = dest_node.as<int>();
				if (idx < static_cast<int>(mob.mob_specials.dest.size()))
				{
					mob.mob_specials.dest[idx] = room_vnum;
				}
				idx++;
			}
		}
	}

	// Initialize test data if needed
	if (mob.GetLevel() == 0)
		SetTestData(&mob);

	return mob;
}

// Parallel mob loading
void YamlWorldDataSource::LoadMobsParallel()
{
	std::vector<int> mob_vnums = GetMobList();
	if (mob_vnums.empty())
	{
		log("No mobs found in YAML index.");
		return;
	}

	int mob_count = mob_vnums.size();
	mob_proto = new CharData[mob_count];
	CREATE(mob_index, mob_count);
	log("   %d mobs, %zd bytes in index, %zd bytes in prototypes.",
		mob_count, sizeof(IndexData) * mob_count, sizeof(CharData) * mob_count);

	// Build zone vnum to rnum map
	std::map<int, int> zone_vnum_to_rnum;
	for (size_t i = 0; i < zone_table.size(); i++)
	{
		zone_vnum_to_rnum[zone_table[i].vnum] = i;
	}

	// Pre-allocate vnum to index mapping (sorted by vnum)
	std::map<int, size_t> vnum_to_idx;
	for (size_t i = 0; i < mob_vnums.size(); ++i)
	{
		vnum_to_idx[mob_vnums[i]] = i;
	}

	// Distribute mobs into batches
	auto batches = utils::DistributeBatches(mob_vnums, m_num_threads);
	std::atomic<int> error_count{0};

	// Launch parallel loading
	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &vnum_to_idx, &error_count]() {
			for (int vnum : batches[thread_id])
			{
				std::string filepath = m_world_dir + "/mobs/" + std::to_string(vnum) + ".yaml";
				try
				{
					size_t mob_idx = vnum_to_idx.at(vnum);
					mob_proto[mob_idx] = ParseMobFile(filepath);
					mob_proto[mob_idx].set_rnum(mob_idx);

					// Attach triggers (thread-safe read from trig_index)
					YAML::Node root = YAML::LoadFile(filepath);
					if (root["triggers"] && root["triggers"].IsSequence())
					{
						for (const auto &trig_node : root["triggers"])
						{
							int trigger_vnum = trig_node.as<int>();
							AttachTriggerToMob(mob_idx, trigger_vnum, vnum);
						}
					}
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load mob from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
				catch (const std::exception &e)
				{
					log("SYSERR: Failed to load mob from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
			}
		}));
	}

	// Wait for all tasks
	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		log("FATAL: %d mob(s) failed to load. Aborting.", error_count.load());
		exit(1);
	}

	// Sequential post-processing: setup mob_index and zone locations
	top_of_mobt = mob_count;

	// top_of_mobt should be last valid index, not count
	if (top_of_mobt > 0)
	{
		top_of_mobt--;
	}

	for (size_t i = 0; i < mob_vnums.size(); ++i)
	{
		int vnum = mob_vnums[i];

		mob_index[i].vnum = vnum;
		mob_index[i].total_online = 0;
		mob_index[i].stored = 0;
		mob_index[i].func = nullptr;
		mob_index[i].farg = nullptr;
		mob_index[i].proto = nullptr;
		mob_index[i].set_idx = -1;

		// Update zone RnumMobsLocation
		int zone_vnum = vnum / 100;
		auto zone_it = zone_vnum_to_rnum.find(zone_vnum);
		if (zone_it != zone_vnum_to_rnum.end())
		{
			if (zone_table[zone_it->second].RnumMobsLocation.first == -1)
			{
				zone_table[zone_it->second].RnumMobsLocation.first = i;
			}
			zone_table[zone_it->second].RnumMobsLocation.second = i;
		}
	}

	log("Loaded %d mobs from YAML (parallel).", top_of_mobt);
}

void YamlWorldDataSource::LoadMobs()
{
	log("Loading mobs from YAML files.");

	// Dictionaries already loaded in LoadZones, thread pool already created
	LoadMobsParallel();
}

// Parse single object file (thread-safe worker function)
CObjectPrototype* YamlWorldDataSource::ParseObjectFile(const std::string &file_path)
{
	YAML::Node root = YAML::LoadFile(file_path);
	
	// Extract vnum from filename
	size_t last_slash = file_path.find_last_of('/');
	size_t last_dot = file_path.find_last_of('.');
	std::string vnum_str = file_path.substr(last_slash + 1, last_dot - last_slash - 1);
	int vnum = std::atoi(vnum_str.c_str());
	
	// NOTE: This returns raw pointer - caller must wrap in shared_ptr
	auto obj_ptr = new CObjectPrototype(vnum);
	
			// Object created above

			// Names
			YAML::Node names = root["names"];
			if (names)
			{
				obj_ptr->set_aliases(GetText(names, "aliases"));
				obj_ptr->set_short_description(utils::colorLOW(GetText(names, "nominative")));
				obj_ptr->set_PName(ECase::kNom, utils::colorLOW(GetText(names, "nominative")));
				obj_ptr->set_PName(ECase::kGen, utils::colorLOW(GetText(names, "genitive")));
				obj_ptr->set_PName(ECase::kDat, utils::colorLOW(GetText(names, "dative")));
				obj_ptr->set_PName(ECase::kAcc, utils::colorLOW(GetText(names, "accusative")));
				obj_ptr->set_PName(ECase::kIns, utils::colorLOW(GetText(names, "instrumental")));
				obj_ptr->set_PName(ECase::kPre, utils::colorLOW(GetText(names, "prepositional")));
			}

			obj_ptr->set_description(utils::colorCAP(GetText(root, "short_desc")));
			obj_ptr->set_action_description(GetText(root, "action_desc"));

			// Type
			int obj_type_id = ParseEnum(root["type"], "obj_types", 0);
			obj_ptr->set_type(static_cast<EObjType>(obj_type_id));

			// Material
			obj_ptr->set_material(static_cast<EObjMaterial>(GetInt(root, "material", 0)));

			// Values
			YAML::Node values = root["values"];
			if (values && values.IsSequence() && values.size() >= 4)
			{
				obj_ptr->set_val(0, values[0].as<long>(0));
				obj_ptr->set_val(1, values[1].as<long>(0));
				obj_ptr->set_val(2, values[2].as<long>(0));
				obj_ptr->set_val(3, values[3].as<long>(0));
			}

			// Physical properties
			obj_ptr->set_weight(GetInt(root, "weight", 0));
			if (obj_ptr->get_type() == EObjType::kLiquidContainer || obj_ptr->get_type() == EObjType::kFountain)
			{
				if (obj_ptr->get_weight() < obj_ptr->get_val(1))
				{
					obj_ptr->set_weight(obj_ptr->get_val(1) + 5);
				}
			}
			obj_ptr->set_cost(GetInt(root, "cost", 0));
			obj_ptr->set_rent_off(GetInt(root, "rent_off", 0));
			obj_ptr->set_rent_on(GetInt(root, "rent_on", 0));
			obj_ptr->set_spec_param(GetInt(root, "spec_param", 0));

			int max_dur = GetInt(root, "max_durability", 100);
			int cur_dur = GetInt(root, "cur_durability", 100);
			obj_ptr->set_maximum_durability(max_dur);
			obj_ptr->set_current_durability(std::min(max_dur, cur_dur));

			int timer = GetInt(root, "timer", -1);
			if (timer <= 0) { timer = ObjData::SEVEN_DAYS; }
			if (timer > 99999) timer = 99999;
			obj_ptr->set_timer(timer);

			obj_ptr->set_spell(GetInt(root, "spell", -1));
			obj_ptr->set_level(GetInt(root, "level", 0));
			obj_ptr->set_sex(static_cast<EGender>(ParseGender(root["sex"])));

			if (root["max_in_world"])
			{
				obj_ptr->set_max_in_world(root["max_in_world"].as<int>());
			}
			else
			{
				obj_ptr->set_max_in_world(-1);
			}

			obj_ptr->set_minimum_remorts(GetInt(root, "minimum_remorts", 0));

			// Flags
			auto &dm = DictionaryManager::Instance();

			if (root["extra_flags"] && root["extra_flags"].IsSequence())
			{
				for (const auto &flag_node : root["extra_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("extra_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->set_extra_flag(static_cast<EObjFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_extra_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			if (root["wear_flags"] && root["wear_flags"].IsSequence())
			{
				int wear_flags = 0;
				for (const auto &flag_node : root["wear_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("wear_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						wear_flags |= (1 << flag_val);
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						if (bit >= 0 && bit < 32)
							wear_flags |= (1 << bit);
					}
				}
				obj_ptr->set_wear_flags(wear_flags);
			}

			if (root["no_flags"] && root["no_flags"].IsSequence())
			{
				for (const auto &flag_node : root["no_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("no_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->set_no_flag(static_cast<ENoFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_no_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			if (root["anti_flags"] && root["anti_flags"].IsSequence())
			{
				for (const auto &flag_node : root["anti_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("anti_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->set_anti_flag(static_cast<EAntiFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_anti_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			if (root["affect_flags"] && root["affect_flags"].IsSequence())
			{
				for (const auto &flag_node : root["affect_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("affect_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj_ptr->SetEWeaponAffectFlag(static_cast<EWeaponAffect>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj_ptr->toggle_affect_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			// Match Legacy: remove transformed and ticktimer flags after loading
			obj_ptr->unset_extraflag(EObjFlag::kTransformed);
			obj_ptr->unset_extraflag(EObjFlag::kTicktimer);

			// Match Legacy: override max_in_world for zonedecay/repop_decay objects
			if (obj_ptr->has_flag(EObjFlag::kZonedacay) || obj_ptr->has_flag(EObjFlag::kRepopDecay))
			{
				obj_ptr->set_max_in_world(-1);
			}

			// Applies
			int apply_idx = 0;
			if (root["applies"] && root["applies"].IsSequence())
			{
				for (const auto &apply_node : root["applies"])
				{
					if (apply_idx >= kMaxObjAffect) break;
					int location = GetInt(apply_node, "location", 0);
					int modifier = GetInt(apply_node, "modifier", 0);
					obj_ptr->set_affected(apply_idx++, static_cast<EApply>(location), modifier);
				}
			}

			// Extra descriptions
			if (root["extra_descriptions"] && root["extra_descriptions"].IsSequence())
			{
				for (const auto &ed_node : root["extra_descriptions"])
				{
					std::string keywords = GetText(ed_node, "keywords");
					std::string description = GetText(ed_node, "description");

					auto ex_desc = std::make_shared<ExtraDescription>();
					ex_desc->set_keyword(keywords);
					ex_desc->set_description(description);
					ex_desc->next = obj_ptr->get_ex_description();
					obj_ptr->set_ex_description(ex_desc);
				}
			}

			// Add to proto first to get rnum

	return obj_ptr;
}


// Parallel object loading
void YamlWorldDataSource::LoadObjectsParallel()
{
	std::vector<int> obj_vnums = GetObjectList();
	if (obj_vnums.empty())
	{
		log("No objects found in YAML index.");
		return;
	}

	int obj_count = obj_vnums.size();
	log("   %d objs.", obj_count);

	// Distribute objects into batches
	auto batches = utils::DistributeBatches(obj_vnums, m_num_threads);

	// Thread-local results (each thread collects its objects)
	std::vector<std::vector<std::pair<int, CObjectPrototype*>>> thread_results(batches.size());
	std::atomic<int> error_count{0};

	// Launch parallel loading
	std::vector<std::future<void>> futures;
	for (size_t thread_id = 0; thread_id < batches.size(); ++thread_id)
	{
		futures.push_back(m_thread_pool->Enqueue([this, thread_id, &batches, &thread_results, &error_count]() {
			for (int vnum : batches[thread_id])
			{
				std::string filepath = m_world_dir + "/objects/" + std::to_string(vnum) + ".yaml";
				try
				{
					CObjectPrototype* obj = ParseObjectFile(filepath);
					thread_results[thread_id].emplace_back(vnum, obj);
				}
				catch (const YAML::Exception &e)
				{
					log("SYSERR: Failed to load object from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
				catch (const std::exception &e)
				{
					log("SYSERR: Failed to load object from '%s': %s", filepath.c_str(), e.what());
					error_count++;
				}
			}
		}));
	}

	// Wait for all tasks
	for (auto &future : futures)
	{
		future.wait();
	}

	if (error_count > 0)
	{
		log("FATAL: %d object(s) failed to load. Aborting.", error_count.load());
		exit(1);
	}

	// Merge results into obj_proto (sequential)
	int loaded_count = 0;
	for (auto &results : thread_results)
	{
		for (auto &[vnum, obj_raw_ptr] : results)
		{
			// Wrap in shared_ptr and add to obj_proto
			auto obj = std::shared_ptr<CObjectPrototype>(obj_raw_ptr);
			obj_proto.add(obj, vnum);
			loaded_count++;
		}
	}

	log("Loaded %d objects from YAML (parallel).", loaded_count);
}


void YamlWorldDataSource::LoadObjects()
{
	log("Loading objects from YAML files.");

	// Dictionaries already loaded in LoadZones, thread pool already created
	LoadObjectsParallel();
}

// Helper methods for save operations

std::string YamlWorldDataSource::ConvertToUtf8(const std::string &koi8r_str) const
{
	if (koi8r_str.empty())
	{
		return "";
	}

	static char buffer[65536];
	char *input = const_cast<char*>(koi8r_str.c_str());
	char *output = buffer;
	koi_to_utf8(input, output);
	return buffer;
}

std::vector<std::string> YamlWorldDataSource::ConvertFlagsToNames(const FlagData &flags, const std::string &dict_name) const
{
	std::vector<std::string> names;
	auto &dm = DictionaryManager::Instance();
	const Dictionary *dict = dm.GetDictionary(dict_name);

	if (!dict)
	{
		return names;
	}

	const auto &entries = dict->GetEntries();

	for (size_t plane = 0; plane < FlagData::kPlanesNumber; ++plane)
	{
		Bitvector plane_bits = flags.get_plane(plane);
		if (plane_bits == 0) continue;

		for (int bit = 0; bit < 30; ++bit)
		{
			if (plane_bits & (1 << bit))
			{
				int bit_index = plane * 30 + bit;

				std::string flag_name;
				for (const auto &[name, value] : entries)
				{
					if (value == bit_index)
					{
						flag_name = name;
						break;
					}
				}

				if (!flag_name.empty())
				{
					names.push_back(flag_name);
				}
				else
				{
					names.push_back("UNUSED_" + std::to_string(bit_index));
				}
			}
		}
	}

	return names;
}

std::string YamlWorldDataSource::ReverseLookupEnum(const std::string &dict_name, int value) const
{
	auto &dm = DictionaryManager::Instance();
	const Dictionary *dict = dm.GetDictionary(dict_name);

	if (!dict)
	{
		return std::to_string(value);
	}

	const auto &entries = dict->GetEntries();
	for (const auto &[name, dict_value] : entries)
	{
		if (dict_value == value)
		{
			return name;
		}
	}

	return std::to_string(value);
}

bool YamlWorldDataSource::WriteYamlAtomic(const std::string &filepath, const YAML::Node &node) const
{
	std::string temp_filepath = filepath + ".tmp";

	try
	{
		YAML::Emitter emitter;
		emitter << node;

		std::ofstream out(temp_filepath);
		if (!out.is_open())
		{
			log("SYSERR: Failed to open temp file for writing: %s", temp_filepath.c_str());
			return false;
		}
		out << emitter.c_str();
		out.close();

		std::rename(temp_filepath.c_str(), filepath.c_str());
		return true;
	}
	catch (const std::exception &e)
	{
		log("SYSERR: Failed to write YAML file %s: %s", filepath.c_str(), e.what());
	
		return false;
	}
}
// ============================================================================

void YamlWorldDataSource::SaveZone(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveZone", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	std::string zone_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum);
	std::string zone_file = zone_dir + "/zone.yaml";

	namespace fs = std::filesystem;
	if (!fs::exists(zone_dir))
	{
		fs::create_directories(zone_dir);
	}

	YAML::Node root;
	root["vnum"] = zone.vnum;
	root["name"] = ConvertToUtf8(zone.name);
	root["zone_group"] = zone.group;

	YAML::Node metadata;
	if (!zone.comment.empty()) metadata["comment"] = ConvertToUtf8(zone.comment);
	if (!zone.location.empty()) metadata["location"] = ConvertToUtf8(zone.location);
	if (!zone.author.empty()) metadata["author"] = ConvertToUtf8(zone.author);
	if (!zone.description.empty()) metadata["description"] = ConvertToUtf8(zone.description);
	if (metadata.size() > 0) root["metadata"] = metadata;

	root["top_room"] = zone.top;
	root["lifespan"] = zone.lifespan;
	root["reset_mode"] = zone.reset_mode;
	root["reset_idle"] = zone.reset_idle ? 1 : 0;
	root["zone_type"] = zone.type;
	root["mode"] = zone.level;
	if (zone.entrance != 0) root["entrance"] = zone.entrance;
	root["under_construction"] = zone.under_construction;

	if (zone.typeA_count > 0)
	{
		YAML::Node typeA_zones;
		for (int i = 0; i < zone.typeA_count; ++i)
		{
			typeA_zones.push_back(zone.typeA_list[i]);
		}
		root["typeA_zones"] = typeA_zones;
	}

	if (zone.typeB_count > 0)
	{
		YAML::Node typeB_zones;
		for (int i = 0; i < zone.typeB_count; ++i)
		{
			typeB_zones.push_back(zone.typeB_list[i]);
		}
		root["typeB_zones"] = typeB_zones;
	}

	if (zone.cmd && zone.cmd[0].command != 'S')
	{
		YAML::Node commands;
		for (int i = 0; zone.cmd[i].command != 'S'; ++i)
		{
			const struct reset_com &cmd = zone.cmd[i];
			YAML::Node cmd_node;

			cmd_node["if_flag"] = cmd.if_flag;

			switch (cmd.command)
			{
			case 'M':
				cmd_node["type"] = "LOAD_MOB";
				cmd_node["mob_vnum"] = cmd.arg1;
				cmd_node["max_world"] = cmd.arg2;
				cmd_node["room_vnum"] = cmd.arg3;
				if (cmd.arg4 != -1) cmd_node["max_room"] = cmd.arg4;
				break;
			case 'O':
				cmd_node["type"] = "LOAD_OBJ";
				cmd_node["obj_vnum"] = cmd.arg1;
				cmd_node["max"] = cmd.arg2;
				cmd_node["room_vnum"] = cmd.arg3;
				if (cmd.arg4 != -1) cmd_node["load_prob"] = cmd.arg4;
				break;
			case 'G':
				cmd_node["type"] = "GIVE_OBJ";
				cmd_node["obj_vnum"] = cmd.arg1;
				cmd_node["max"] = cmd.arg2;
				if (cmd.arg4 != -1) cmd_node["load_prob"] = cmd.arg4;
				break;
			case 'E':
				cmd_node["type"] = "EQUIP_MOB";
				cmd_node["obj_vnum"] = cmd.arg1;
				cmd_node["max"] = cmd.arg2;
				cmd_node["wear_pos"] = cmd.arg3;
				if (cmd.arg4 != -1) cmd_node["load_prob"] = cmd.arg4;
				break;
			case 'P':
				cmd_node["type"] = "PUT_OBJ";
				cmd_node["obj_vnum"] = cmd.arg1;
				cmd_node["max"] = cmd.arg2;
				cmd_node["container_vnum"] = cmd.arg3;
				if (cmd.arg4 != -1) cmd_node["load_prob"] = cmd.arg4;
				break;
			case 'D':
				cmd_node["type"] = "DOOR";
				cmd_node["room_vnum"] = cmd.arg1;
				cmd_node["direction"] = cmd.arg2;
				cmd_node["state"] = cmd.arg3;
				break;
			case 'R':
				cmd_node["type"] = "REMOVE_OBJ";
				cmd_node["room_vnum"] = cmd.arg1;
				cmd_node["obj_vnum"] = cmd.arg2;
				break;
			case 'T':
				cmd_node["type"] = "TRIGGER";
				cmd_node["trigger_type"] = cmd.arg1;
				cmd_node["trigger_vnum"] = cmd.arg2;
				if (cmd.arg3 != -1) cmd_node["room_vnum"] = cmd.arg3;
				break;
			case 'V':
				cmd_node["type"] = "VARIABLE";
				cmd_node["trigger_type"] = cmd.arg1;
				cmd_node["context"] = cmd.arg2;
				cmd_node["room_vnum"] = cmd.arg3;
				if (cmd.sarg1) cmd_node["var_name"] = ConvertToUtf8(cmd.sarg1);
				if (cmd.sarg2) cmd_node["var_value"] = ConvertToUtf8(cmd.sarg2);
				break;
			case 'Q':
				cmd_node["type"] = "EXTRACT_MOB";
				cmd_node["mob_vnum"] = cmd.arg1;
				cmd_node["if_flag"] = 0;
				break;
			case 'F':
				cmd_node["type"] = "FOLLOW";
				cmd_node["room_vnum"] = cmd.arg1;
				cmd_node["leader_mob_vnum"] = cmd.arg2;
				cmd_node["follower_mob_vnum"] = cmd.arg3;
				break;
			default:
				continue;
			}

			commands.push_back(cmd_node);
		}
		root["commands"] = commands;
	}

	if (!WriteYamlAtomic(zone_file, root))
	{
		log("SYSERR: Failed to save zone %d", zone.vnum);
		return;
	}

	log("Saved zone %d to YAML file", zone.vnum);
}

void YamlWorldDataSource::SaveTriggers(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveTriggers", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	TrgRnum first_trig = zone.RnumTrigsLocation.first;
	TrgRnum last_trig = zone.RnumTrigsLocation.second;

	if (first_trig == -1 || last_trig == -1)
	{
		log("Zone %d has no triggers to save", zone.vnum);
		return;
	}

	std::string trig_dir = m_world_dir + "/triggers";
	namespace fs = std::filesystem;
	if (!fs::exists(trig_dir))
	{
		fs::create_directories(trig_dir);
	}

	int saved_count = 0;
	for (TrgRnum trig_rnum = first_trig; trig_rnum <= last_trig && trig_rnum <= top_of_trigt; ++trig_rnum)
	{
		if (!trig_index[trig_rnum])
		{
			continue;
		}

		int trig_vnum = trig_index[trig_rnum]->vnum;
		Trigger *trig = trig_index[trig_rnum]->proto;

		if (!trig)
		{
			continue;
		}

		YAML::Node root;
		root["name"] = ConvertToUtf8(GET_TRIG_NAME(trig));
		root["attach_type"] = ReverseLookupEnum("attach_types", trig->get_attach_type());
		root["narg"] = GET_TRIG_NARG(trig);
		if (!trig->arglist.empty())
		{
			root["arglist"] = ConvertToUtf8(trig->arglist);
		}

		YAML::Node trigger_types;
		for (int bit = 0; bit < 32; ++bit)
		{
			if (GET_TRIG_TYPE(trig) & (1L << bit))
			{
				std::string type_name = ReverseLookupEnum("trigger_types", bit);
				if (!type_name.empty() && type_name != std::to_string(bit))
				{
					trigger_types.push_back(type_name);
				}
			}
		}
		if (trigger_types.size() > 0)
		{
			root["trigger_types"] = trigger_types;
		}

		std::string script;
		for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next)
		{
			if (!cmd->cmd.empty())
			{
				script += cmd->cmd;
				if (cmd->next)
				{
					script += "\n";
				}
			}
		}
		if (!script.empty())
		{
			root["script"] = ConvertToUtf8(script);
		}

		std::string trig_file = trig_dir + "/" + std::to_string(trig_vnum) + ".yaml";
		if (!WriteYamlAtomic(trig_file, root))
		{
			log("SYSERR: Failed to save trigger %d", trig_vnum);
			continue;
		}
		++saved_count;
	}

	log("Saved %d triggers for zone %d", saved_count, zone.vnum);
}

void YamlWorldDataSource::SaveRooms(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveRooms", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	RoomRnum first_room = zone.RnumRoomsLocation.first;
	RoomRnum last_room = zone.RnumRoomsLocation.second;

	if (first_room == -1 || last_room == -1)
	{
		log("Zone %d has no rooms to save", zone.vnum);
		return;
	}

	std::string rooms_dir = m_world_dir + "/zones/" + std::to_string(zone.vnum) + "/rooms";
	namespace fs = std::filesystem;
	if (!fs::exists(rooms_dir))
	{
		fs::create_directories(rooms_dir);
	}

	int saved_count = 0;
	for (RoomRnum room_rnum = first_room; room_rnum <= last_room && room_rnum <= top_of_world; ++room_rnum)
	{
		RoomData *room = world[room_rnum];
		if (!room || room->vnum < zone.vnum * 100 || room->vnum > zone.top)
		{
			continue;
		}

		YAML::Node root;
		root["vnum"] = room->vnum;

		if (room->name)
		{
			root["name"] = ConvertToUtf8(room->name);
		}

		// description_num is size_t (unsigned), always >= 0
		std::string desc = GlobalObjects::descriptions().get(room->description_num);
		if (!desc.empty())
		{
			root["description"] = ConvertToUtf8(desc);
		}

		root["sector"] = ReverseLookupEnum("sectors", static_cast<int>(room->sector_type));

		FlagData room_flags = room->read_flags();
		auto flag_names = ConvertFlagsToNames(room_flags, "room_flags");
		if (!flag_names.empty())
		{
			YAML::Node flags;
			for (const auto &flag : flag_names)
			{
				flags.push_back(flag);
			}
			root["flags"] = flags;
		}

		YAML::Node exits;
		for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
		{
			if (!room->dir_option_proto[dir])
			{
				continue;
			}

			YAML::Node exit_node;
			exit_node["direction"] = ReverseLookupEnum("directions", dir);

			if (room->dir_option_proto[dir]->to_room() != kNowhere)
			{
				RoomRnum to_rnum = room->dir_option_proto[dir]->to_room();
				if (to_rnum >= 0 && to_rnum <= top_of_world && world[to_rnum])
				{
					exit_node["to_room"] = world[to_rnum]->vnum;
				}
				else
				{
					exit_node["to_room"] = -1;
				}
			}
			else
			{
				exit_node["to_room"] = -1;
			}

			if (!room->dir_option_proto[dir]->general_description.empty())
			{
				exit_node["description"] = ConvertToUtf8(room->dir_option_proto[dir]->general_description);
			}

			if (room->dir_option_proto[dir]->keyword)
			{
				exit_node["keywords"] = ConvertToUtf8(room->dir_option_proto[dir]->keyword);
			}

			if (room->dir_option_proto[dir]->exit_info != 0)
			{
				exit_node["exit_flags"] = room->dir_option_proto[dir]->exit_info;
			}

			if (room->dir_option_proto[dir]->key != -1)
			{
				exit_node["key"] = room->dir_option_proto[dir]->key;
			}

			if (room->dir_option_proto[dir]->lock_complexity != 0)
			{
				exit_node["lock_complexity"] = room->dir_option_proto[dir]->lock_complexity;
			}

			exits.push_back(exit_node);
		}
		if (exits.size() > 0)
		{
			root["exits"] = exits;
		}

		if (room->ex_description)
		{
			YAML::Node extra_descs;
			for (auto exdesc = room->ex_description; exdesc; exdesc = exdesc->next)
			{
				YAML::Node ed_node;
				if (exdesc->keyword)
				{
					ed_node["keywords"] = ConvertToUtf8(exdesc->keyword);
				}
				if (exdesc->description)
				{
					ed_node["description"] = ConvertToUtf8(exdesc->description);
				}
				extra_descs.push_back(ed_node);
			}
			if (extra_descs.size() > 0)
			{
				root["extra_descriptions"] = extra_descs;
			}
		}

		if (room->proto_script && !room->proto_script->empty())
		{
			YAML::Node triggers;
			for (auto trig_vnum : *room->proto_script)
			{
				triggers.push_back(trig_vnum);
			}
			root["triggers"] = triggers;
		}

		int rel_num = room->vnum % 100;
		std::string room_file = rooms_dir + "/" + std::to_string(rel_num) + ".yaml";
		if (!WriteYamlAtomic(room_file, root))
		{
			log("SYSERR: Failed to save room %d", room->vnum);
			continue;
		}
		++saved_count;
	}

	log("Saved %d rooms for zone %d", saved_count, zone.vnum);
}

void YamlWorldDataSource::SaveMobs(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveMobs", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	MobRnum first_mob = zone.RnumMobsLocation.first;
	MobRnum last_mob = zone.RnumMobsLocation.second;

	if (first_mob == -1 || last_mob == -1)
	{
		log("Zone %d has no mobs to save", zone.vnum);
		return;
	}

	std::string mobs_dir = m_world_dir + "/mobs";
	namespace fs = std::filesystem;
	if (!fs::exists(mobs_dir))
	{
		fs::create_directories(mobs_dir);
	}

	int saved_count = 0;
	for (MobRnum mob_rnum = first_mob; mob_rnum <= last_mob && mob_rnum <= top_of_mobt; ++mob_rnum)
	{
		if (!mob_index[mob_rnum].vnum)
		{
			continue;
		}

		int mob_vnum = mob_index[mob_rnum].vnum;
		CharData &mob = mob_proto[mob_rnum];

		YAML::Node root;

		YAML::Node names;
		const std::string &aliases = mob.get_npc_name();
		if (!aliases.empty())
		{
			names["aliases"] = ConvertToUtf8(aliases);
		}
		names["nominative"] = ConvertToUtf8(mob.player_data.PNames[ECase::kNom]);
		names["genitive"] = ConvertToUtf8(mob.player_data.PNames[ECase::kGen]);
		names["dative"] = ConvertToUtf8(mob.player_data.PNames[ECase::kDat]);
		names["accusative"] = ConvertToUtf8(mob.player_data.PNames[ECase::kAcc]);
		names["instrumental"] = ConvertToUtf8(mob.player_data.PNames[ECase::kIns]);
		names["prepositional"] = ConvertToUtf8(mob.player_data.PNames[ECase::kPre]);
		root["names"] = names;

		YAML::Node descs;
		descs["short_desc"] = ConvertToUtf8(mob.player_data.long_descr);
		descs["long_desc"] = ConvertToUtf8(mob.player_data.description);
		root["descriptions"] = descs;

		root["alignment"] = GET_ALIGNMENT(&mob);

		YAML::Node stats;
		stats["level"] = mob.GetLevel();
		stats["hitroll_penalty"] = GET_HR(&mob);
		stats["armor"] = GET_AC(&mob);

		YAML::Node hp;
		hp["dice_count"] = mob.mem_queue.total;
		hp["dice_size"] = mob.mem_queue.stored;
		hp["bonus"] = mob.get_hit();
		stats["hp"] = hp;

		YAML::Node dmg;
		dmg["dice_count"] = mob.mob_specials.damnodice;
		dmg["dice_size"] = mob.mob_specials.damsizedice;
		dmg["bonus"] = mob.real_abils.damroll;
		stats["damage"] = dmg;

		root["stats"] = stats;

		YAML::Node gold;
		gold["dice_count"] = mob.mob_specials.GoldNoDs;
		gold["dice_size"] = mob.mob_specials.GoldSiDs;
		gold["bonus"] = mob.get_gold();
		root["gold"] = gold;

		root["experience"] = mob.get_exp();

		YAML::Node pos;
		pos["default"] = ReverseLookupEnum("positions", static_cast<int>(mob.mob_specials.default_pos));
		pos["start"] = ReverseLookupEnum("positions", static_cast<int>(mob.GetPosition()));
		root["position"] = pos;

		root["sex"] = ReverseLookupEnum("genders", static_cast<int>(mob.get_sex()));

		root["size"] = GET_SIZE(&mob);
		root["height"] = GET_HEIGHT(&mob);
		root["weight"] = GET_WEIGHT(&mob);

		if (mob.get_str() > 0)
		{
			YAML::Node attrs;
			attrs["strength"] = mob.get_str();
			attrs["dexterity"] = mob.get_dex();
			attrs["intelligence"] = mob.get_int();
			attrs["wisdom"] = mob.get_wis();
			attrs["constitution"] = mob.get_con();
			attrs["charisma"] = mob.get_cha();
			root["attributes"] = attrs;
		}

		auto act_flags = ConvertFlagsToNames(mob.char_specials.saved.act, "action_flags");
		if (!act_flags.empty())
		{
			YAML::Node action_flags;
			for (const auto &flag : act_flags)
			{
				action_flags.push_back(flag);
			}
			root["action_flags"] = action_flags;
		}

		auto aff_flags = ConvertFlagsToNames(AFF_FLAGS(&mob), "affect_flags");
		if (!aff_flags.empty())
		{
			YAML::Node affect_flags;
			for (const auto &flag : aff_flags)
			{
				affect_flags.push_back(flag);
			}
			root["affect_flags"] = affect_flags;
		}

		YAML::Node skills;
		for (ESkill skill_id = ESkill::kFirst; skill_id <= ESkill::kLast; ++skill_id)
		{
			int skill_value = mob.GetSkill(skill_id);
			if (skill_value > 0)
			{
				YAML::Node skill_node;
				skill_node["skill_id"] = static_cast<int>(skill_id);
				skill_node["value"] = skill_value;
				skills.push_back(skill_node);
			}
		}
		if (skills.size() > 0)
		{
			root["skills"] = skills;
		}

		if (mob.proto_script && !mob.proto_script->empty())
		{
			YAML::Node triggers;
			for (auto trig_vnum : *mob.proto_script)
			{
				triggers.push_back(trig_vnum);
			}
			root["triggers"] = triggers;
		}

		if (mob.get_str() > 0)
		{
			YAML::Node enhanced;

			if (mob.get_str_add() != 0) enhanced["str_add"] = mob.get_str_add();
			if (mob.add_abils.hitreg != 0) enhanced["hp_regen"] = mob.add_abils.hitreg;
			if (mob.add_abils.armour != 0) enhanced["armour_bonus"] = mob.add_abils.armour;
			if (mob.add_abils.manareg != 0) enhanced["mana_regen"] = mob.add_abils.manareg;
			if (mob.add_abils.cast_success != 0) enhanced["cast_success"] = mob.add_abils.cast_success;
			if (mob.add_abils.morale != 0) enhanced["morale"] = mob.add_abils.morale;
			if (mob.add_abils.initiative_add != 0) enhanced["initiative_add"] = mob.add_abils.initiative_add;
			if (mob.add_abils.absorb != 0) enhanced["absorb"] = mob.add_abils.absorb;
			if (mob.add_abils.aresist != 0) enhanced["aresist"] = mob.add_abils.aresist;
			if (mob.add_abils.mresist != 0) enhanced["mresist"] = mob.add_abils.mresist;
			if (mob.add_abils.presist != 0) enhanced["presist"] = mob.add_abils.presist;
			if (mob.mob_specials.attack_type != 0) enhanced["bare_hand_attack"] = mob.mob_specials.attack_type;
			if (mob.mob_specials.like_work != 0) enhanced["like_work"] = mob.mob_specials.like_work;
			if (mob.mob_specials.MaxFactor != 0) enhanced["max_factor"] = mob.mob_specials.MaxFactor;
			if (mob.mob_specials.extra_attack != 0) enhanced["extra_attack"] = mob.mob_specials.extra_attack;
			if (mob.get_remort() != 0) enhanced["mob_remort"] = mob.get_remort();

			char special_buf[kMaxStringLength];
			mob.mob_specials.npc_flags.tascii(FlagData::kPlanesNumber, special_buf);
			if (special_buf[0] != '0' || special_buf[1] != 'a')
			{
				enhanced["special_bitvector"] = special_buf;
			}

			std::string role_str = mob.get_role().to_string();
			if (!role_str.empty() && role_str != "000000000")
			{
				enhanced["role"] = role_str;
			}

			bool has_resistances = false;
			for (const auto &val : mob.add_abils.apply_resistance)
			{
				if (val != 0) { has_resistances = true; break; }
			}
			if (has_resistances)
			{
				YAML::Node resistances;
				for (const auto &val : mob.add_abils.apply_resistance)
				{
					resistances.push_back(val);
				}
				enhanced["resistances"] = resistances;
			}

			bool has_saves = false;
			for (const auto &val : mob.add_abils.apply_saving_throw)
			{
				if (val != 0) { has_saves = true; break; }
			}
			if (has_saves)
			{
				YAML::Node saves;
				for (const auto &val : mob.add_abils.apply_saving_throw)
				{
					saves.push_back(val);
				}
				enhanced["saves"] = saves;
			}

			YAML::Node feats;
			for (size_t i = 0; i < mob.real_abils.Feats.size(); ++i)
			{
				if (mob.real_abils.Feats.test(i))
				{
					feats.push_back(static_cast<int>(i));
				}
			}
			if (feats.size() > 0)
			{
				enhanced["feats"] = feats;
			}

			YAML::Node spells;
			for (size_t i = 0; i < mob.real_abils.SplKnw.size(); ++i)
			{
				if (mob.real_abils.SplKnw[i] > 0)
				{
					spells.push_back(static_cast<int>(i));
				}
			}
			if (spells.size() > 0)
			{
				enhanced["spells"] = spells;
			}

			if (!mob.summon_helpers.empty())
			{
				YAML::Node helpers;
				for (int helper_vnum : mob.summon_helpers)
				{
					helpers.push_back(helper_vnum);
				}
				enhanced["helpers"] = helpers;
			}

			bool has_destinations = false;
			for (int dest : mob.mob_specials.dest)
			{
				if (dest != 0) { has_destinations = true; break; }
			}
			if (has_destinations)
			{
				YAML::Node destinations;
				for (int dest : mob.mob_specials.dest)
				{
					destinations.push_back(dest);
				}
				enhanced["destinations"] = destinations;
			}

			if (enhanced.size() > 0)
			{
				root["enhanced"] = enhanced;
			}
		}

		std::string mob_file = mobs_dir + "/" + std::to_string(mob_vnum) + ".yaml";
		if (!WriteYamlAtomic(mob_file, root))
		{
			log("SYSERR: Failed to save mob %d", mob_vnum);
			continue;
		}
		++saved_count;
	}

	log("Saved %d mobs for zone %d", saved_count, zone.vnum);
}

void YamlWorldDataSource::SaveObjects(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveObjects", zone_rnum);
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	std::string objs_dir = m_world_dir + "/objects";
	namespace fs = std::filesystem;
	if (!fs::exists(objs_dir))
	{
		fs::create_directories(objs_dir);
	}

	int saved_count = 0;
	int start_vnum = zone.vnum * 100;
	int end_vnum = zone.top;

	for (const auto &[obj_vnum, obj_rnum] : obj_proto.vnum2index())
	{
		if (obj_vnum < start_vnum || obj_vnum > end_vnum)
		{
			continue;
		}

		auto obj = obj_proto[obj_rnum];
		if (!obj)
		{
			continue;
		}
		YAML::Node names;
		names["aliases"] = ConvertToUtf8(obj->get_aliases());
		names["nominative"] = ConvertToUtf8(obj->get_PName(ECase::kNom));

		YAML::Node root;
		names["genitive"] = ConvertToUtf8(obj->get_PName(ECase::kGen));
		names["dative"] = ConvertToUtf8(obj->get_PName(ECase::kDat));
		names["accusative"] = ConvertToUtf8(obj->get_PName(ECase::kAcc));
		names["instrumental"] = ConvertToUtf8(obj->get_PName(ECase::kIns));
		names["prepositional"] = ConvertToUtf8(obj->get_PName(ECase::kPre));
		root["names"] = names;

		root["short_desc"] = ConvertToUtf8(obj->get_description());
		if (!obj->get_action_description().empty())
		{
			root["action_desc"] = ConvertToUtf8(obj->get_action_description());
		}

		root["type"] = ReverseLookupEnum("obj_types", static_cast<int>(obj->get_type()));
		root["material"] = static_cast<int>(obj->get_material());

		YAML::Node values;
		values.push_back(obj->get_val(0));
		values.push_back(obj->get_val(1));
		values.push_back(obj->get_val(2));
		values.push_back(obj->get_val(3));
		root["values"] = values;

		root["weight"] = obj->get_weight();
		root["cost"] = obj->get_cost();
		root["rent_off"] = obj->get_rent_off();
		root["rent_on"] = obj->get_rent_on();
		if (obj->get_spec_param() != 0) root["spec_param"] = obj->get_spec_param();
		root["max_durability"] = obj->get_maximum_durability();
		root["cur_durability"] = obj->get_current_durability();
		root["timer"] = obj->get_timer();
		if (to_underlying(obj->get_spell()) >= 0) root["spell"] = to_underlying(obj->get_spell());
		root["level"] = obj->get_level();
		root["sex"] = ReverseLookupEnum("genders", static_cast<int>(obj->get_sex()));
		if (obj->get_max_in_world() != -1) root["max_in_world"] = obj->get_max_in_world();
		if (obj->get_minimum_remorts() != 0) root["minimum_remorts"] = obj->get_minimum_remorts();

		auto extra_flags = ConvertFlagsToNames(obj->get_extra_flags(), "extra_flags");
		if (!extra_flags.empty())
		{
			YAML::Node flags;
			for (const auto &flag : extra_flags)
			{
				flags.push_back(flag);
			}
			root["extra_flags"] = flags;
		}

		int wear_flags = obj->get_wear_flags();
		if (wear_flags != 0)
		{
			YAML::Node wear;
			for (int bit = 0; bit < 32; ++bit)
			{
				if (wear_flags & (1 << bit))
				{
					std::string flag_name = ReverseLookupEnum("wear_flags", bit);
					if (!flag_name.empty() && flag_name != std::to_string(bit))
					{
						wear.push_back(flag_name);
					}
					else
					{
						wear.push_back("UNUSED_" + std::to_string(bit));
					}
				}
			}
			root["wear_flags"] = wear;
		}

		auto no_flags = ConvertFlagsToNames(obj->get_no_flags(), "no_flags");
		if (!no_flags.empty())
		{
			YAML::Node flags;
			for (const auto &flag : no_flags)
			{
				flags.push_back(flag);
			}
			root["no_flags"] = flags;
		}

		auto anti_flags = ConvertFlagsToNames(obj->get_anti_flags(), "anti_flags");
		if (!anti_flags.empty())
		{
			YAML::Node flags;
			for (const auto &flag : anti_flags)
			{
				flags.push_back(flag);
			}
			root["anti_flags"] = flags;
		}

		auto affect_flags = ConvertFlagsToNames(obj->get_affect_flags(), "affect_flags");
		if (!affect_flags.empty())
		{
			YAML::Node flags;
			for (const auto &flag : affect_flags)
			{
				flags.push_back(flag);
			}
			root["affect_flags"] = flags;
		}

		YAML::Node applies;
		for (int i = 0; i < kMaxObjAffect; ++i)
		{
			if (obj->get_affected(i).location != EApply::kNone)
			{
				YAML::Node apply_node;
				apply_node["location"] = static_cast<int>(obj->get_affected(i).location);
				apply_node["modifier"] = obj->get_affected(i).modifier;
				applies.push_back(apply_node);
			}
		}
		if (applies.size() > 0)
		{
			root["applies"] = applies;
		}

		if (obj->get_ex_description())
		{
			YAML::Node extra_descs;
			for (auto exdesc = obj->get_ex_description(); exdesc; exdesc = exdesc->next)
			{
				YAML::Node ed_node;
				if (exdesc->keyword)
				{
					ed_node["keywords"] = ConvertToUtf8(exdesc->keyword);
				}
				if (exdesc->description)
				{
					ed_node["description"] = ConvertToUtf8(exdesc->description);
				}
				extra_descs.push_back(ed_node);
			}
			if (extra_descs.size() > 0)
			{
				root["extra_descriptions"] = extra_descs;
			}
		}

		if (obj->get_proto_script_ptr() && !obj->get_proto_script().empty())
		{
			YAML::Node triggers;
			for (auto trig_vnum : obj->get_proto_script())
			{
				triggers.push_back(trig_vnum);
			}
			root["triggers"] = triggers;
		}

		std::string obj_file = objs_dir + "/" + std::to_string(obj_vnum) + ".yaml";
		if (!WriteYamlAtomic(obj_file, root))
		{
			log("SYSERR: Failed to save object %d", obj_vnum);
			continue;
		}
		++saved_count;
	}

	log("Saved %d objects for zone %d", saved_count, zone.vnum);
}

// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<IWorldDataSource> CreateYamlDataSource(const std::string &world_dir)
{
	return std::make_unique<YamlWorldDataSource>(world_dir);
}

} // namespace world_loader

#endif // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
