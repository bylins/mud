// Part of Bylins http://www.mud.ru
// YAML world data source implementation

#ifdef HAVE_YAML

#include "yaml_world_data_source.h"
#include "dictionary_loader.h"
#include "db.h"
#include "obj_prototypes.h"
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

void YamlWorldDataSource::LoadZones()
{
	log("Loading zones from YAML files.");

	if (!LoadDictionaries())
	{
		return;
	}

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

	int zone_idx = 0;
	int error_count = 0;
	for (int zone_vnum : zone_vnums)
	{
		std::string zone_path = m_world_dir + "/zones/" + std::to_string(zone_vnum) + "/zone.yaml";

		try
		{
			YAML::Node root = YAML::LoadFile(zone_path);
			ZoneData &zone = zone_table[zone_idx];

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

			zone_idx++;
		}
		catch (const YAML::Exception &e)
		{
			log("SYSERR: Failed to load zone %d from '%s': %s", zone_vnum, zone_path.c_str(), e.what());
			++error_count;
		}
	}

	if (error_count > 0)
	{
		log("FATAL: %d zone(s) failed to load. Aborting.", error_count);
		exit(1);
	}

	log("Loaded %d zones from YAML.", zone_idx);
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

void YamlWorldDataSource::LoadTriggers()
{
	log("Loading triggers from YAML files.");

	if (!LoadDictionaries())
	{
		return;
	}

	// Get list of triggers to load from index.yaml
	std::vector<int> trigger_vnums = GetTriggerList();
	if (trigger_vnums.empty())
	{
		log("No triggers found in YAML index.");
		return;
	}

	int trig_count = trigger_vnums.size();
	if (trig_count == 0)
	{
		log("No triggers found in YAML files.");
		return;
	}

	CREATE(trig_index, trig_count);
	log("   %d triggers.", trig_count);

	top_of_trigt = 0;
	int error_count = 0;
	for (int vnum : trigger_vnums)
	{
		std::string filepath = m_world_dir + "/triggers/" + std::to_string(vnum) + ".yaml";
		try
		{
			YAML::Node root = YAML::LoadFile(filepath);

			std::string name = GetText(root, "name", "");
			int attach_type = ParseEnum(root["attach_type"], "attach_types", 0);

			// Parse trigger types
			long trigger_type = 0;
			if (root["trigger_types"] && root["trigger_types"].IsSequence())
			{
				for (const auto &type_node : root["trigger_types"])
				{
					std::string type_str = type_node.as<std::string>();
					// Lookup in trigger_types dictionary
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

			// Create trigger
			auto trig = new Trigger(top_of_trigt, std::move(name), static_cast<byte>(attach_type), trigger_type);
			GET_TRIG_NARG(trig) = narg;
			trig->arglist = arglist;


		// Parse script into cmdlist (uses base class method)
		ParseTriggerScript(trig, script);


		// Create index entry (uses base class method)
		CreateTriggerIndex(vnum, trig);

		}
		catch (const YAML::Exception &e)
		{
			log("SYSERR: Failed to load trigger from '%s': %s", filepath.c_str(), e.what());
			++error_count;
		}
	}

	if (error_count > 0)
	{
		log("FATAL: %d trigger(s) failed to load. Aborting.", error_count);
		exit(1);
	}

	log("Loaded %d triggers from YAML.", top_of_trigt);
}

// ============================================================================
// Room Loading
// ============================================================================

void YamlWorldDataSource::LoadRooms()
{
	log("Loading rooms from YAML files.");

	if (!LoadDictionaries())
	{
		return;
	}

	// Get list of enabled zones from index.yaml
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

		// Skip non-numeric directories (like index.yaml)
		std::string zone_dir_name = zone_entry.path().filename().string();
		if (zone_dir_name.empty() || !std::isdigit(zone_dir_name[0])) continue;

		int zone_vnum = std::stoi(zone_dir_name);

		// Skip disabled zones (not in index.yaml)
		if (enabled_zones.find(zone_vnum) == enabled_zones.end()) continue;

		std::string rooms_dir = zone_entry.path().string() + "/rooms";
		if (!fs::exists(rooms_dir)) continue;

		for (const auto &room_entry : fs::directory_iterator(rooms_dir))
		{
			if (!room_entry.is_regular_file()) continue;
			std::string filename = room_entry.path().filename().string();
			if (filename.size() < 6 || filename.substr(filename.size() - 5) != ".yaml") continue;

			// Room vnum = zone_vnum * 100 + relative number (from filename)
			int rel_num = std::stoi(filename.substr(0, filename.size() - 5));
			int vnum = zone_vnum * 100 + rel_num;
			room_files.emplace_back(vnum, room_entry.path().string());
		}
	}

	// Sort by vnum
	std::sort(room_files.begin(), room_files.end());

	int room_count = room_files.size();
	if (room_count == 0)
	{
		log("No rooms found in YAML files.");
		return;
	}

	// Create kNowhere room first
	world.push_back(new RoomData);
	top_of_world = kNowhere;

	log("   %d rooms, %zd bytes.", room_count, sizeof(RoomData) * room_count);

	int error_count = 0;
	// Zone rnum
	ZoneRnum zone_rn = 0;
	std::map<int, int> room_vnum_to_rnum;

	for (const auto &[vnum, filepath] : room_files)
	{
		try
		{
			YAML::Node root = YAML::LoadFile(filepath);

			auto room = new RoomData;
			room->vnum = vnum;

			std::string name = GetText(root, "name", "Untitled Room");
			if (!name.empty()) { name[0] = UPPER(name[0]); }
			room->set_name(name);

			std::string description = GetText(root, "description", "");
			if (!description.empty())
			{
				room->description_num = RoomDescription::add_desc(description);
			}

			// Set zone_rn
			while (vnum > zone_table[zone_rn].top)
			{
				if (++zone_rn >= static_cast<ZoneRnum>(zone_table.size()))
				{
					log("SYSERR: Room %d is outside of any zone.", vnum);
					break;
				}
			}
			if (zone_rn < static_cast<ZoneRnum>(zone_table.size()))
			{
				room->zone_rn = zone_rn;
				if (zone_table[zone_rn].RnumRoomsLocation.first == -1)
				{
					zone_table[zone_rn].RnumRoomsLocation.first = top_of_world + 1;
				}
				zone_table[zone_rn].RnumRoomsLocation.second = top_of_world + 1;
			}

			// Sector type
			room->sector_type = static_cast<ESector>(ParseEnum(root["sector"], "sectors", 0));

			// Room flags
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

			world.push_back(room);
			room_vnum_to_rnum[vnum] = ++top_of_world;

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

			// Load triggers
			if (root["triggers"] && root["triggers"].IsSequence())
			{
				for (const auto &trig_node : root["triggers"])
				{
					int trigger_vnum = trig_node.as<int>();
					AttachTriggerToRoom(top_of_world, trigger_vnum, vnum);
				}
			}
		}
		catch (const YAML::Exception &e)
		{
			log("SYSERR: Failed to load room from '%s': %s", filepath.c_str(), e.what());
			++error_count;
		}
	}

	if (error_count > 0)
	{
		log("FATAL: %d room(s) failed to load. Aborting.", error_count);
		exit(1);
	}

	log("Loaded %d rooms from YAML.", top_of_world);
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

void YamlWorldDataSource::LoadMobs()
{
	log("Loading mobs from YAML files.");

	if (!LoadDictionaries())
	{
		return;
	}

	// Get list of mobs to load from index.yaml
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

	top_of_mobt = 0;
	int error_count = 0;
	for (int vnum : mob_vnums)
	{
		std::string filepath = m_world_dir + "/mobs/" + std::to_string(vnum) + ".yaml";
		try
		{
			YAML::Node root = YAML::LoadFile(filepath);
			CharData &mob = mob_proto[top_of_mobt];

			mob.player_specials = player_special_data::s_for_mobiles;
			mob.SetNpcAttribute(true);
			mob.player_specials->saved.NameGod = 1001; // Default for Russian name declension
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

			// E-spec attributes - set defaults, then override if attributes section exists
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

			// Triggers
			if (root["triggers"] && root["triggers"].IsSequence())
			{
				for (const auto &trig_node : root["triggers"])
				{
					int trigger_vnum = trig_node.as<int>();
					AttachTriggerToMob(top_of_mobt, trigger_vnum, vnum);
				}
			}

			// Enhanced E-spec fields
			if (root["enhanced"])
			{
				YAML::Node enhanced = root["enhanced"];
				
				// Scalar fields
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
				
				// special_bitvector (FlagData)
				if (enhanced["special_bitvector"])
				{
					std::string special_bv = enhanced["special_bitvector"].as<std::string>();
					mob.mob_specials.npc_flags.from_string((char *)special_bv.c_str());
				}
				
				// role (bitset<9>)
				if (enhanced["role"])
				{
					std::string role_str = enhanced["role"].as<std::string>();
					CharData::role_t role(role_str);
					mob.set_role(role);
				}
				
				// Resistances array
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
				
				// Saves array
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
				
				// Feats array
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
				
				// Spells array
				if (enhanced["spells"] && enhanced["spells"].IsSequence())
				{
					for (const auto &spell_node : enhanced["spells"])
					{
						int spell_id = spell_node.as<int>();
						if (spell_id >= 0 && spell_id < static_cast<int>(mob.real_abils.SplKnw.size()))
						{
							mob.real_abils.SplKnw[spell_id] = 1;  // Mark spell as known
						}
					}
				}
				
				// Helpers array
				if (enhanced["helpers"] && enhanced["helpers"].IsSequence())
				{
					for (const auto &helper_node : enhanced["helpers"])
					{
						int helper_vnum = helper_node.as<int>();
						mob.summon_helpers.push_back(helper_vnum);
					}
				}
				
				// Destinations array
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

			// Setup index
			int zone_vnum = vnum / 100;
			auto zone_it = zone_vnum_to_rnum.find(zone_vnum);
			if (zone_it != zone_vnum_to_rnum.end())
			{
				if (zone_table[zone_it->second].RnumMobsLocation.first == -1)
				{
					zone_table[zone_it->second].RnumMobsLocation.first = top_of_mobt;
				}
				zone_table[zone_it->second].RnumMobsLocation.second = top_of_mobt;
			}

			mob_index[top_of_mobt].vnum = vnum;
			mob_index[top_of_mobt].total_online = 0;
			mob_index[top_of_mobt].stored = 0;
			mob_index[top_of_mobt].func = nullptr;
			mob_index[top_of_mobt].farg = nullptr;
			mob_index[top_of_mobt].proto = nullptr;
			mob_index[top_of_mobt].set_idx = -1;

			// Initialize test data if needed
			if (mob.GetLevel() == 0)
				SetTestData(&mob);
			mob.set_rnum(top_of_mobt);
			top_of_mobt++;
		}
		catch (const YAML::Exception &e)
		{
			log("SYSERR: Failed to load mob from '%s': %s", filepath.c_str(), e.what());
			++error_count;
		}
	}

	if (error_count > 0)
	{
		log("FATAL: %d mob(s) failed to load. Aborting.", error_count);
		exit(1);
	}

	if (top_of_mobt > 0)
	{
		top_of_mobt--;
	}

	log("Loaded %d mobs from YAML.", top_of_mobt + 1);
}

// ============================================================================
// Object Loading
// ============================================================================

void YamlWorldDataSource::LoadObjects()
{
	log("Loading objects from YAML files.");

	if (!LoadDictionaries())
	{
		return;
	}

	// Get list of objects to load from index.yaml
	std::vector<int> obj_vnums = GetObjectList();
	if (obj_vnums.empty())
	{
		log("No objects found in YAML index.");
		return;
	}

	int obj_count = obj_vnums.size();

	log("   %d objs.", obj_count);

	int error_count = 0;
	for (int vnum : obj_vnums)
	{
		std::string filepath = m_world_dir + "/objects/" + std::to_string(vnum) + ".yaml";
		try
		{
			YAML::Node root = YAML::LoadFile(filepath);

			auto obj = std::make_shared<CObjectPrototype>(vnum);

			// Names
			YAML::Node names = root["names"];
			if (names)
			{
				obj->set_aliases(GetText(names, "aliases"));
				obj->set_short_description(utils::colorLOW(GetText(names, "nominative")));
				obj->set_PName(ECase::kNom, utils::colorLOW(GetText(names, "nominative")));
				obj->set_PName(ECase::kGen, utils::colorLOW(GetText(names, "genitive")));
				obj->set_PName(ECase::kDat, utils::colorLOW(GetText(names, "dative")));
				obj->set_PName(ECase::kAcc, utils::colorLOW(GetText(names, "accusative")));
				obj->set_PName(ECase::kIns, utils::colorLOW(GetText(names, "instrumental")));
				obj->set_PName(ECase::kPre, utils::colorLOW(GetText(names, "prepositional")));
			}

			obj->set_description(utils::colorCAP(GetText(root, "short_desc")));
			obj->set_action_description(GetText(root, "action_desc"));

			// Type
			int obj_type_id = ParseEnum(root["type"], "obj_types", 0);
			obj->set_type(static_cast<EObjType>(obj_type_id));

			// Material
			obj->set_material(static_cast<EObjMaterial>(GetInt(root, "material", 0)));

			// Values
			YAML::Node values = root["values"];
			if (values && values.IsSequence() && values.size() >= 4)
			{
				obj->set_val(0, values[0].as<long>(0));
				obj->set_val(1, values[1].as<long>(0));
				obj->set_val(2, values[2].as<long>(0));
				obj->set_val(3, values[3].as<long>(0));
			}

			// Physical properties
			obj->set_weight(GetInt(root, "weight", 0));
			if (obj->get_type() == EObjType::kLiquidContainer || obj->get_type() == EObjType::kFountain)
			{
				if (obj->get_weight() < obj->get_val(1))
				{
					obj->set_weight(obj->get_val(1) + 5);
				}
			}
			obj->set_cost(GetInt(root, "cost", 0));
			obj->set_rent_off(GetInt(root, "rent_off", 0));
			obj->set_rent_on(GetInt(root, "rent_on", 0));
			obj->set_spec_param(GetInt(root, "spec_param", 0));

			int max_dur = GetInt(root, "max_durability", 100);
			int cur_dur = GetInt(root, "cur_durability", 100);
			obj->set_maximum_durability(max_dur);
			obj->set_current_durability(std::min(max_dur, cur_dur));

			int timer = GetInt(root, "timer", -1);
			if (timer <= 0) { timer = ObjData::SEVEN_DAYS; }
			if (timer > 99999) timer = 99999;
			obj->set_timer(timer);

			obj->set_spell(GetInt(root, "spell", -1));
			obj->set_level(GetInt(root, "level", 0));
			obj->set_sex(static_cast<EGender>(ParseGender(root["sex"])));

			if (root["max_in_world"])
			{
				obj->set_max_in_world(root["max_in_world"].as<int>());
			}
			else
			{
				obj->set_max_in_world(-1);
			}

			obj->set_minimum_remorts(GetInt(root, "minimum_remorts", 0));

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
						obj->set_extra_flag(static_cast<EObjFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj->toggle_extra_flag(plane, 1 << bit_in_plane);
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
				obj->set_wear_flags(wear_flags);
			}

			if (root["no_flags"] && root["no_flags"].IsSequence())
			{
				for (const auto &flag_node : root["no_flags"])
				{
					std::string flag_name = flag_node.as<std::string>();
					long flag_val = dm.Lookup("no_flags", flag_name, -1);
					if (flag_val >= 0)
					{
						obj->set_no_flag(static_cast<ENoFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj->toggle_no_flag(plane, 1 << bit_in_plane);
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
						obj->set_anti_flag(static_cast<EAntiFlag>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj->toggle_anti_flag(plane, 1 << bit_in_plane);
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
						obj->SetEWeaponAffectFlag(static_cast<EWeaponAffect>(IndexToBitvector(flag_val)));
					}
					else if (flag_name.rfind("UNUSED_", 0) == 0)
					{
						int bit = std::stoi(flag_name.substr(7));
						size_t plane = bit / 30;
						int bit_in_plane = bit % 30;
						obj->toggle_affect_flag(plane, 1 << bit_in_plane);
					}
				}
			}

			// Match Legacy: remove transformed and ticktimer flags after loading
			obj->unset_extraflag(EObjFlag::kTransformed);
			obj->unset_extraflag(EObjFlag::kTicktimer);

			// Match Legacy: override max_in_world for zonedecay/repop_decay objects
			if (obj->has_flag(EObjFlag::kZonedacay) || obj->has_flag(EObjFlag::kRepopDecay))
			{
				obj->set_max_in_world(-1);
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
					obj->set_affected(apply_idx++, static_cast<EApply>(location), modifier);
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
					ex_desc->next = obj->get_ex_description();
					obj->set_ex_description(ex_desc);
				}
			}

			// Add to proto first to get rnum
			ObjRnum obj_rnum = obj_proto.add(obj, vnum);

			// Triggers (after adding to proto so we have rnum)
			if (root["triggers"] && root["triggers"].IsSequence())
			{
				for (const auto &trig_node : root["triggers"])
				{
					int trigger_vnum = trig_node.as<int>();
					AttachTriggerToObject(obj_rnum, trigger_vnum, vnum);
				}
			}
		}
		catch (const YAML::Exception &e)
		{
			log("SYSERR: Failed to load object from '%s': %s", filepath.c_str(), e.what());
			++error_count;
		}
	}

	if (error_count > 0)
	{
		log("FATAL: %d object(s) failed to load. Aborting.", error_count);
		exit(1);
	}

	log("Loaded %zu objects from YAML.", obj_proto.size());
}

// ============================================================================
// Save methods (not implemented - read-only)
// ============================================================================

// ============================================================================
// Save helper functions
// ============================================================================

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

void YamlWorldDataSource::WriteYamlAtomic(const std::string &filename, const YAML::Node &node) const
{
	std::string new_file = filename + ".new";
	std::string old_file = filename + ".old";

	// Step 1: Write to .new file
	std::ofstream out(new_file);
	if (!out)
	{
		log("SYSERR: Cannot open file for writing: %s", new_file.c_str());
		return;
	}

	YAML::Emitter emitter;
	emitter << node;
	out << emitter.c_str();
	out.close();

	if (!out.good())
	{
		log("SYSERR: Error writing to file: %s", new_file.c_str());
		std::filesystem::remove(new_file);
		return;
	}

	// Step 2: Rename old file to .old (if exists)
	if (std::filesystem::exists(filename))
	{
		std::error_code ec;
		std::filesystem::rename(filename, old_file, ec);
		if (ec)
		{
			log("SYSERR: Cannot rename %s to %s: %s", filename.c_str(), old_file.c_str(), ec.message().c_str());
			std::filesystem::remove(new_file);
			return;
		}
	}

	// Step 3: Rename .new to target
	{
		std::error_code ec;
		std::filesystem::rename(new_file, filename, ec);
		if (ec)
		{
			log("SYSERR: Cannot rename %s to %s: %s", new_file.c_str(), filename.c_str(), ec.message().c_str());
			// Try to restore .old
			if (std::filesystem::exists(old_file))
			{
				std::filesystem::rename(old_file, filename);
			}
			return;
		}
	}

	// Step 4: Remove .old backup
	if (std::filesystem::exists(old_file))
	{
		std::filesystem::remove(old_file);
	}
}

void YamlWorldDataSource::UpdateIndexYaml(const std::string &section, int vnum) const
{
	std::string index_file = m_world_dir + "/" + section + "/index.yaml";
	std::set<int> vnums;

	// Load existing index
	if (std::filesystem::exists(index_file))
	{
		try
		{
			YAML::Node index = YAML::LoadFile(index_file);
			if (index.IsSequence())
			{
				for (const auto &item : index)
				{
					vnums.insert(item.as<int>());
				}
			}
		}
		catch (const YAML::Exception &e)
		{
			log("SYSERR: Error loading %s: %s", index_file.c_str(), e.what());
		}
	}

	// Add new vnum
	vnums.insert(vnum);

	// Write back
	YAML::Node index_node;
	for (int v : vnums)
	{
		index_node.push_back(v);
	}

	WriteYamlAtomic(index_file, index_node);
}

void YamlWorldDataSource::SaveZone(int zone_rnum)
{
	log("SYSERR: YAML data source is read-only. Cannot save zone %d.", zone_rnum);
}

void YamlWorldDataSource::SaveTriggers(int zone_rnum)
{
	log("SYSERR: YAML data source is read-only. Cannot save triggers for zone %d.", zone_rnum);
}

void YamlWorldDataSource::SaveRooms(int zone_rnum)
{
	log("SYSERR: YAML data source is read-only. Cannot save rooms for zone %d.", zone_rnum);
}

void YamlWorldDataSource::SaveMobs(int zone_rnum)
{
	log("SYSERR: YAML data source is read-only. Cannot save mobs for zone %d.", zone_rnum);
}

void YamlWorldDataSource::SaveObjects(int zone_rnum)
{
	log("SYSERR: YAML data source is read-only. Cannot save objects for zone %d.", zone_rnum);
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
