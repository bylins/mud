// Part of Bylins http://www.mud.ru
// World checksum calculation for detecting changes during refactoring

#include "world_checksum.h"

#include "db.h"
#include "obj_prototypes.h"
#include "global_objects.h"
#include "engine/entities/zone.h"
#include "engine/entities/room_data.h"
#include "engine/entities/char_data.h"
#include "utils/logger.h"
#include "utils/utils_string.h"
#include "engine/structs/extra_description.h"

#include <sstream>
#include <vector>
#include <filesystem>

#include <iomanip>

namespace
{

// CRC32 calculation function (same as in file_crc.cpp)
uint32_t CRC32(const char *buf, size_t len)
{
	static uint32_t crc_table[256];
	static bool table_initialized = false;

	if (!table_initialized)
	{
		for (int i = 0; i < 256; i++)
		{
			uint32_t crc = i;
			for (int j = 0; j < 8; j++)
			{
				crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
			}
			crc_table[i] = crc;
		}
		table_initialized = true;
	}

	uint32_t crc = 0xFFFFFFFFUL;
	while (len--)
	{
		crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFFUL;
}

uint32_t CRC32String(const std::string &str)
{
	return CRC32(str.c_str(), str.size());
}

// Serialize zone data to string for checksum calculation
std::string SerializeZone(const ZoneData &zone)
{
	std::ostringstream oss;

	// Include only persistent data, exclude runtime state
	oss << zone.vnum << "|";
	oss << zone.name << "|";
	oss << zone.lifespan << "|";
	oss << zone.top << "|";
	oss << zone.reset_mode << "|";
	oss << zone.comment << "|";
	oss << zone.author << "|";
	oss << zone.traffic << "|";
	oss << zone.level << "|";
	oss << zone.type << "|";
	oss << zone.first_enter << "|";
	oss << zone.location << "|";
	oss << zone.description << "|";
	oss << zone.typeA_count << "|";
	oss << zone.typeB_count << "|";
	oss << zone.under_construction << "|";
	oss << zone.group << "|";
	oss << zone.mob_level << "|";
	oss << zone.is_town << "|";

	// Serialize zone commands
	if (zone.cmd)
	{
		for (int i = 0; zone.cmd[i].command != 'S'; ++i)
		{
			const auto &cmd = zone.cmd[i];
			oss << cmd.command << ":";
			oss << cmd.if_flag << ":";
			oss << cmd.arg1 << ":";
			oss << cmd.arg2 << ":";
			oss << cmd.arg3 << ":";
			oss << cmd.arg4 << ":";
			if (cmd.sarg1)
			{
				oss << cmd.sarg1;
			}
			oss << ":";
			if (cmd.sarg2)
			{
				oss << cmd.sarg2;
			}
			oss << ";";
		}
	}



	return oss.str();
}

// Serialize room data to string for checksum calculation
std::string SerializeRoom(const RoomData *room)
{
	if (!room)
	{
		return "";
	}

	std::ostringstream oss;

	oss << room->vnum << "|";
	// zone_rn excluded - runtime index
	oss << room->sector_type << "|";
	if (room->name)
	{
		oss << room->name;
	}
	oss << "|";

	// Serialize room description (actual text content)
	if (room->temp_description)
	{
		oss << room->temp_description;
	}
	else
	{
		oss << GlobalObjects::descriptions().get(room->description_num);
	}
	oss << "|";

	// Serialize room flags using safe dynamic buffer
	std::vector<char> flag_buf(8192);
	flag_buf[0] = '\0';
	if (room->flags_sprint(flag_buf.data(), ","))
	{
		// Filter out UNDEF flags for consistent checksums
		// (UNDEF flags are undefined bits that Legacy loads but SQLite does not preserve)
		std::string flags_str(flag_buf.data());
		std::string filtered;
		for (const auto& flag : utils::Split(flags_str, ',')) {
			if (flag != "UNDEF") {
				if (!filtered.empty()) filtered += ",";
				filtered += flag;
			}
		}
		oss << filtered;
	}
	oss << "|";

	// Serialize exits (proto only)
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
	{
		const auto &exit = room->dir_option_proto[dir];
		if (exit)
		{
			oss << dir << ":";
			// Use vnum instead of rnum to make checksum independent of loading order
			auto to_rnum = exit->to_room();
			int to_vnum = (to_rnum >= 0 && to_rnum <= top_of_world && world[to_rnum])
				? world[to_rnum]->vnum : -1;
			oss << to_vnum << ":";
			oss << exit->general_description << ":";
			if (exit->keyword)
			{
				oss << exit->keyword;
			}
			oss << ":";
			if (exit->vkeyword)
			{
				oss << exit->vkeyword;
			}
			oss << ":";
			oss << static_cast<int>(exit->exit_info) << ":";
			oss << static_cast<int>(exit->lock_complexity) << ":";
			oss << exit->key << ";";
		}
	}
	oss << "|";

	// Serialize proto_script (trigger vnums)
	if (room->proto_script)
	{
		for (const auto &trig_vnum : *room->proto_script)
		{
			oss << trig_vnum << ",";
		}
	}

	return oss.str();
}

// Serialize mob prototype data to string for checksum calculation
std::string SerializeMob(MobRnum rnum)
{
	if (rnum < 0 || rnum > top_of_mobt)
	{
		return "";
	}

	const CharData &mob = mob_proto[rnum];
	std::ostringstream oss;

	oss << mob_index[rnum].vnum << "|";
	oss << mob.GetLevel() << "|";
	oss << mob.get_str() << "|";
	oss << mob.get_dex() << "|";
	oss << mob.get_con() << "|";
	oss << mob.get_wis() << "|";
	oss << mob.get_int() << "|";
	oss << mob.get_cha() << "|";
	oss << mob.get_name() << "|";
	oss << mob.GetCharAliases() << "|";
	oss << mob.get_npc_name() << "|";

	// NPC pads
	for (int i = 0; i < 6; ++i)
	{
		const char *pad = mob.get_pad(i);
		if (pad)
		{
			oss << pad;
		}
		oss << ",";
	}
	oss << "|";

	// Long description
	const char *long_descr = mob.get_long_descr();
	if (long_descr)
	{
		oss << long_descr;
	}
	oss << "|";

	// Description
	const char *descr = mob.get_description();
	if (descr)
	{
		oss << descr;
	}
	oss << "|";

	// Mob specials
	oss << static_cast<int>(mob.IsNpc() ? mob.IsNpc() : 0) << "|";
	oss << mob.get_max_hit() << "|";
	oss << mob.get_hit() << "|";

	// Proto script
	if (mob.proto_script)
	{
		for (const auto &trig_vnum : *mob.proto_script)
		{
			oss << trig_vnum << ",";
		}
	}

	return oss.str();
}

// Serialize object prototype data to string for checksum calculation
std::string SerializeObject(const CObjectPrototype::shared_ptr &obj)
{
	if (!obj)
	{
		return "";
	}

	std::ostringstream oss;

	oss << obj->get_vnum() << "|";
	oss << static_cast<int>(obj->get_type()) << "|";
	oss << obj->get_weight() << "|";
	oss << obj->get_cost() << "|";
	oss << obj->get_rent_on() << "|";
	oss << obj->get_rent_off() << "|";
	oss << obj->get_level() << "|";
	oss << static_cast<int>(obj->get_material()) << "|";
	oss << obj->get_max_in_world() << "|";
	oss << obj->get_timer() << "|";
	oss << static_cast<int>(obj->get_sex()) << "|";
	oss << static_cast<int>(obj->get_spell()) << "|";
	oss << obj->get_maximum_durability() << "|";
	oss << obj->get_current_durability() << "|";
	oss << obj->get_minimum_remorts() << "|";
	oss << obj->get_wear_flags() << "|";

	// Extra flags (all 4 planes)
	const auto &extra = obj->get_extra_flags();
	oss << extra.get_plane(0) << "," << extra.get_plane(1) << ","
	    << extra.get_plane(2) << "," << extra.get_plane(3) << "|";

	// Anti flags (all 4 planes)
	const auto &anti = obj->get_anti_flags();
	oss << anti.get_plane(0) << "," << anti.get_plane(1) << ","
	    << anti.get_plane(2) << "," << anti.get_plane(3) << "|";

	// No flags (all 4 planes)
	const auto &no = obj->get_no_flags();
	oss << no.get_plane(0) << "," << no.get_plane(1) << ","
	    << no.get_plane(2) << "," << no.get_plane(3) << "|";

	// Affect flags (all 4 planes)
	const auto &waff = obj->get_affect_flags();
	oss << waff.get_plane(0) << "," << waff.get_plane(1) << ","
	    << waff.get_plane(2) << "," << waff.get_plane(3) << "|";

	// Values
	for (int i = 0; i < CObjectPrototype::VALS_COUNT; ++i)
	{
		oss << obj->get_val(i) << ",";
	}
	oss << "|";

	// Affected
	for (int i = 0; i < kMaxObjAffect; ++i)
	{
		const auto &aff = obj->get_affected(i);
		oss << static_cast<int>(aff.location) << ":" << aff.modifier << ",";
	}
	oss << "|";

	// Names
	oss << obj->get_short_description() << "|";
	oss << obj->get_description() << "|";
	oss << obj->get_aliases() << "|";
	for (int i = 0; i <= ECase::kLastCase; ++i)
	{
		oss << obj->get_PName(static_cast<ECase>(i)) << ",";
	}
	oss << "|";

	// Extra descriptions
	for (auto ed = obj->get_ex_description(); ed; ed = ed->next)
	{
		if (ed->keyword)
		{
			oss << ed->keyword << ":";
		}
		if (ed->description)
		{
			oss << ed->description;
		}
		oss << ";";
	}
	oss << "|";

	// Proto script
	for (const auto &trig_vnum : obj->get_proto_script())
	{
		oss << trig_vnum << ",";
	}


	return oss.str();
}

// Serialize trigger data to string for checksum calculation
std::string SerializeTrigger(int rnum)
{
	if (rnum < 0 || rnum >= top_of_trigt || !trig_index[rnum])
	{
		return "";
	}

	const auto *index = trig_index[rnum];
	const auto *trig = index->proto;

	if (!trig)
	{
		return "";
	}

	std::ostringstream oss;

	oss << index->vnum << "|";
	oss << trig->get_name() << "|";
	oss << static_cast<int>(trig->get_attach_type()) << "|";
	oss << trig->get_trigger_type() << "|";
	oss << trig->narg << "|";
	oss << trig->arglist << "|";

	// Serialize command list
	if (trig->cmdlist)
	{
		auto cmd = *trig->cmdlist;
		while (cmd)
		{
			oss << cmd->cmd << ";";
			cmd = cmd->next;
		}
	}


	return oss.str();
}

} // anonymous namespace

namespace WorldChecksum
{

ChecksumResult Calculate()
{
	ChecksumResult result = {};

	// Calculate zones checksum
	uint32_t zones_xor = 0;
	for (const auto &zone : zone_table)
	{
		std::string serialized = SerializeZone(zone);
		uint32_t crc = CRC32String(serialized);
		zones_xor ^= crc;
		++result.zones_count;
	}
	result.zones = zones_xor;

	// Calculate rooms checksum
	uint32_t rooms_xor = 0;
	for (RoomRnum i = 0; i <= top_of_world; ++i)
	{
		std::string serialized = SerializeRoom(world[i]);
		uint32_t crc = CRC32String(serialized);
		rooms_xor ^= crc;
		++result.rooms_count;
	}
	result.rooms = rooms_xor;

	// Calculate mobs checksum
	uint32_t mobs_xor = 0;
	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		std::string serialized = SerializeMob(i);
		uint32_t crc = CRC32String(serialized);
		mobs_xor ^= crc;
		++result.mobs_count;
	}
	result.mobs = mobs_xor;

	// Calculate objects checksum
	uint32_t objects_xor = 0;
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		std::string serialized = SerializeObject(obj_proto[i]);
		uint32_t crc = CRC32String(serialized);
		objects_xor ^= crc;
		++result.objects_count;
	}
	result.objects = objects_xor;

	// Calculate triggers checksum
	uint32_t triggers_xor = 0;
	for (int i = 0; i < top_of_trigt; ++i)
	{
		std::string serialized = SerializeTrigger(i);
		uint32_t crc = CRC32String(serialized);
		triggers_xor ^= crc;
		++result.triggers_count;
	}
	result.triggers = triggers_xor;

	// ===== RUNTIME CHECKSUMS (after initialization) =====

	// Calculate room scripts checksum (prototype triggers)
	uint32_t room_scripts_xor = 0;
	for (RoomRnum i = 0; i <= top_of_world; ++i)
	{
		if (world[i]->proto_script && !world[i]->proto_script->empty())
		{
			std::ostringstream oss;
			oss << world[i]->vnum << "|";
			for (const auto trig_vnum : *world[i]->proto_script)
			{
				oss << trig_vnum << ",";
			}
			uint32_t crc = CRC32String(oss.str());
			room_scripts_xor ^= crc;
			++result.rooms_with_scripts;
		}
	}
	result.room_scripts = room_scripts_xor;

	// Calculate mob scripts checksum (prototype triggers)
	uint32_t mob_scripts_xor = 0;
	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		if (mob_proto[i].proto_script && !mob_proto[i].proto_script->empty())
		{
			std::ostringstream oss;
			oss << mob_index[i].vnum << "|";
			for (const auto trig_vnum : *mob_proto[i].proto_script)
			{
				oss << trig_vnum << ",";
			}
			uint32_t crc = CRC32String(oss.str());
			mob_scripts_xor ^= crc;
			++result.mobs_with_scripts;
		}
	}
	result.mob_scripts = mob_scripts_xor;

	// Calculate object scripts checksum
	uint32_t obj_scripts_xor = 0;
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		const auto &obj = obj_proto[i];
		if (!obj_proto.proto_script(i).empty())
		{
			std::ostringstream oss;
			oss << obj->get_vnum() << "|";
			for (const auto trig_vnum : obj_proto.proto_script(i))
			{
				oss << trig_vnum << ",";
			}
			uint32_t crc = CRC32String(oss.str());
			obj_scripts_xor ^= crc;
			++result.objects_with_scripts;
		}
	}
	result.obj_scripts = obj_scripts_xor;

	// Calculate door rnums checksum
	uint32_t door_rnums_xor = 0;
	for (RoomRnum i = 0; i <= top_of_world; ++i)
	{
		for (const auto &exit : world[i]->dir_option_proto)
		{
			if (exit)
			{
				std::ostringstream oss;
				oss << world[i]->vnum << "|" << exit->to_room();
				door_rnums_xor ^= CRC32String(oss.str());
			}
		}
	}
	result.door_rnums = door_rnums_xor;

	// Calculate zone_rnum checksums for mobs
	uint32_t zone_rnums_mobs_xor = 0;
	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		std::ostringstream oss;
		oss << mob_index[i].vnum << "|" << mob_index[i].zone;
		zone_rnums_mobs_xor ^= CRC32String(oss.str());
	}
	result.zone_rnums_mobs = zone_rnums_mobs_xor;

	// Calculate zone_rnum checksums for objects
	uint32_t zone_rnums_objects_xor = 0;
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		std::ostringstream oss;
		oss << obj_proto[i]->get_vnum() << "|" << obj_proto.zone(i);
		zone_rnums_objects_xor ^= CRC32String(oss.str());
	}
	result.zone_rnums_objects = zone_rnums_objects_xor;

	// Calculate zone commands rnums checksum
	uint32_t zone_cmd_rnums_xor = 0;
	for (const auto &zone : zone_table)
	{
		if (zone.cmd)
		{
			for (int i = 0; zone.cmd[i].command != 'S'; ++i)
			{
				const auto &cmd = zone.cmd[i];
				std::ostringstream oss;
				oss << zone.vnum << "|" << cmd.command << "|";
				oss << cmd.arg1 << "|" << cmd.arg2 << "|" << cmd.arg3;
				zone_cmd_rnums_xor ^= CRC32String(oss.str());
			}
		}
	}
	result.zone_cmd_rnums = zone_cmd_rnums_xor;

	// Calculate combined runtime checksum
	std::ostringstream runtime_combined;
	runtime_combined << result.room_scripts << "|";
	runtime_combined << result.mob_scripts << "|";
	runtime_combined << result.obj_scripts << "|";
	runtime_combined << result.door_rnums << "|";
	runtime_combined << result.zone_rnums_mobs << "|";
	runtime_combined << result.zone_rnums_objects << "|";
	runtime_combined << result.zone_cmd_rnums;
	result.runtime_combined = CRC32String(runtime_combined.str());

	// Calculate combined checksum
	std::ostringstream combined;
	combined << result.zones << "|";
	combined << result.rooms << "|";
	combined << result.mobs << "|";
	combined << result.objects << "|";
	combined << result.triggers;
	result.combined = CRC32String(combined.str());

	return result;
}

void LogResult(const ChecksumResult &result)
{
	log("=== World Checksums ===");
	log("Zones:    %08X (%zu zones)", result.zones, result.zones_count);
	log("Rooms:    %08X (%zu rooms)", result.rooms, result.rooms_count);
	log("Mobs:     %08X (%zu mobs)", result.mobs, result.mobs_count);
	log("Objects:  %08X (%zu objects)", result.objects, result.objects_count);
	log("Triggers: %08X (%zu triggers)", result.triggers, result.triggers_count);
	log("Combined: %08X", result.combined);

	log("=== Runtime Checksums (after initialization) ===");
	log("Room Scripts:      %08X (%zu rooms with scripts)", result.room_scripts, result.rooms_with_scripts);
	log("Mob Scripts:       %08X (%zu mobs with scripts)", result.mob_scripts, result.mobs_with_scripts);
	log("Object Scripts:    %08X (%zu objects with scripts)", result.obj_scripts, result.objects_with_scripts);
	log("Door Rnums:        %08X", result.door_rnums);
	log("Zone Rnums (Mobs): %08X", result.zone_rnums_mobs);
	log("Zone Rnums (Objs): %08X", result.zone_rnums_objects);
	log("Zone Cmd Rnums:    %08X", result.zone_cmd_rnums);
	log("Runtime Combined:  %08X", result.runtime_combined);
	log("=======================");
}

void SaveDetailedChecksums(const char *filename, const ChecksumResult &checksums)
{
	FILE *f = fopen(filename, "w");
	if (!f)
	{
		log("SYSERR: Cannot open %s for writing detailed checksums", filename);
		return;
	}

	fprintf(f, "# World Detailed Checksums\n");
	fprintf(f, "# Format: TYPE VNUM CHECKSUM\n\n");

	// Zones
	fprintf(f, "# Zones\n");
	for (const auto &zone : zone_table)
	{
		std::string serialized = SerializeZone(zone);
		uint32_t crc = CRC32String(serialized);
		fprintf(f, "ZONE %d %08X\n", zone.vnum, crc);
	}

	// Rooms
	fprintf(f, "\n# Rooms\n");
	for (RoomRnum i = 0; i <= top_of_world; ++i)
	{
		if (world[i])
		{
			std::string serialized = SerializeRoom(world[i]);
			uint32_t crc = CRC32String(serialized);
			fprintf(f, "ROOM %d %08X\n", world[i]->vnum, crc);
		}
	}

	// Mobs
	fprintf(f, "\n# Mobs\n");
	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		std::string serialized = SerializeMob(i);
		uint32_t crc = CRC32String(serialized);
		fprintf(f, "MOB %d %08X\n", mob_index[i].vnum, crc);
	}

	// Objects
	fprintf(f, "\n# Objects\n");
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		if (obj_proto[i])
		{
			std::string serialized = SerializeObject(obj_proto[i]);
			uint32_t crc = CRC32String(serialized);
			fprintf(f, "OBJ %d %08X\n", obj_proto[i]->get_vnum(), crc);
		}
	}

	// Triggers
	fprintf(f, "\n# Triggers\n");
	for (int i = 0; i < top_of_trigt; ++i)
	{
		std::string serialized = SerializeTrigger(i);
		uint32_t crc = CRC32String(serialized);
		if (trig_index[i])
		{
			fprintf(f, "TRIG %d %08X\n", trig_index[i]->vnum, crc);
		}
	}

	// Runtime Checksums
	fprintf(f, "\n# Runtime Checksums\n");
	fprintf(f, "ROOM_SCRIPTS %08X\n", checksums.room_scripts);
	fprintf(f, "MOB_SCRIPTS %08X\n", checksums.mob_scripts);
	fprintf(f, "OBJ_SCRIPTS %08X\n", checksums.obj_scripts);
	fprintf(f, "DOOR_RNUMS %08X\n", checksums.door_rnums);

	fclose(f);
	log("Detailed checksums saved to %s", filename);
}

bool SaveDetailedBuffers(const char *dir)
{
	try {
		std::filesystem::create_directories(dir);

	auto save_buffer = [](const char *filepath, const std::string &buffer, uint32_t crc) {
		FILE *f = fopen(filepath, "w");
		if (!f)
		{
			log("SYSERR: Failed to open file for writing: %s", filepath);
			return;
		}
		fprintf(f, "CRC32: %08X\n", crc);
		fprintf(f, "Length: %zu\n", buffer.size());
		fprintf(f, "---RAW---\n%s\n", buffer.c_str());
		fprintf(f, "---HEX---\n");
		for (size_t i = 0; i < buffer.size(); ++i)
		{
			fprintf(f, "%02X ", static_cast<unsigned char>(buffer[i]));
			if ((i + 1) % 32 == 0) fprintf(f, "\n");
		}
		fprintf(f, "\n");
		fclose(f);
	};

	std::string zones_dir = std::string(dir) + "/zones";
	std::filesystem::create_directories(zones_dir);
	for (const auto &zone : zone_table)
	{
		std::string serialized = SerializeZone(zone);
		uint32_t crc = CRC32String(serialized);
		std::string filepath = zones_dir + "/" + std::to_string(zone.vnum) + ".txt";
		save_buffer(filepath.c_str(), serialized, crc);
	}

	std::string rooms_dir = std::string(dir) + "/rooms";
	std::filesystem::create_directories(rooms_dir);
	for (RoomRnum i = 0; i <= top_of_world; ++i)
	{
		if (world[i])
		{
			std::string serialized = SerializeRoom(world[i]);
			uint32_t crc = CRC32String(serialized);
			std::string filepath = rooms_dir + "/" + std::to_string(world[i]->vnum) + ".txt";
			save_buffer(filepath.c_str(), serialized, crc);
		}
	}

	std::string mobs_dir = std::string(dir) + "/mobs";
	std::filesystem::create_directories(mobs_dir);
	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		std::string serialized = SerializeMob(i);
		uint32_t crc = CRC32String(serialized);
		std::string filepath = mobs_dir + "/" + std::to_string(mob_index[i].vnum) + ".txt";
		save_buffer(filepath.c_str(), serialized, crc);
	}

	std::string objs_dir = std::string(dir) + "/objects";
	std::filesystem::create_directories(objs_dir);
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		if (obj_proto[i])
		{
			std::string serialized = SerializeObject(obj_proto[i]);
			uint32_t crc = CRC32String(serialized);
			std::string filepath = objs_dir + "/" + std::to_string(obj_proto[i]->get_vnum()) + ".txt";
			save_buffer(filepath.c_str(), serialized, crc);
		}
	}

	std::string trigs_dir = std::string(dir) + "/triggers";
	std::filesystem::create_directories(trigs_dir);
	for (int i = 0; i < top_of_trigt; ++i)
	{
		if (trig_index[i])
		{
			std::string serialized = SerializeTrigger(i);
			uint32_t crc = CRC32String(serialized);
			std::string filepath = trigs_dir + "/" + std::to_string(trig_index[i]->vnum) + ".txt";
			save_buffer(filepath.c_str(), serialized, crc);
		}
	}

	log("Detailed buffers saved to %s", dir);
	return true;

	} catch (const std::filesystem::filesystem_error &e) {
		log("SYSERR: Failed to create directories in '%s': %s", dir, e.what());
		return false;
	}
}

std::map<std::string, uint32_t> LoadBaselineChecksums(const char *filename)
{
	std::map<std::string, uint32_t> result;
	std::ifstream f(filename);
	if (!f.is_open())
	{
		log("SYSERR: Cannot open baseline checksums file: %s", filename);
		return result;
	}

	std::string line;
	while (std::getline(f, line))
	{
		if (line.empty() || line[0] == '#') continue;
		std::istringstream iss(line);
		std::string type;
		int vnum;
		std::string crc_str;
		if (iss >> type >> vnum >> crc_str)
		{
			uint32_t crc = std::stoul(crc_str, nullptr, 16);
			std::string key = type + " " + std::to_string(vnum);
			result[key] = crc;
		}
	}

	log("Loaded %zu baseline checksums from %s", result.size(), filename);
	return result;
}

void CompareWithBaseline(const char *baseline_dir, int max_mismatches_per_type)
{
	std::string checksums_file = std::string(baseline_dir) + "/checksums_detailed.txt";
	auto baseline = LoadBaselineChecksums(checksums_file.c_str());
	if (baseline.empty())
	{
		log("No baseline checksums loaded, skipping comparison");
		return;
	}

	log("=== Comparing with baseline from %s ===", baseline_dir);

	int zone_mismatches = 0;
	int room_mismatches = 0;
	int mob_mismatches = 0;
	int obj_mismatches = 0;
	int trig_mismatches = 0;

	for (const auto &zone : zone_table)
	{
		std::string key = "ZONE " + std::to_string(zone.vnum);
		std::string serialized = SerializeZone(zone);
		uint32_t crc = CRC32String(serialized);
		auto it = baseline.find(key);
		if (it != baseline.end() && it->second != crc)
		{
			++zone_mismatches;
			if (zone_mismatches <= max_mismatches_per_type)
			{
				log("MISMATCH ZONE %d: baseline=%08X current=%08X", zone.vnum, it->second, crc);
				std::string baseline_file = std::string(baseline_dir) + "/zones/" + std::to_string(zone.vnum) + ".txt";
				log("  Baseline: %s", baseline_file.c_str());
				log("  Current buffer: %s", serialized.c_str());
			}
		}
	}

	for (RoomRnum i = 0; i <= top_of_world; ++i)
	{
		if (!world[i]) continue;
		std::string key = "ROOM " + std::to_string(world[i]->vnum);
		std::string serialized = SerializeRoom(world[i]);
		uint32_t crc = CRC32String(serialized);
		auto it = baseline.find(key);
		if (it != baseline.end() && it->second != crc)
		{
			++room_mismatches;
			if (room_mismatches <= max_mismatches_per_type)
			{
				log("MISMATCH ROOM %d: baseline=%08X current=%08X", world[i]->vnum, it->second, crc);
				std::string baseline_file = std::string(baseline_dir) + "/rooms/" + std::to_string(world[i]->vnum) + ".txt";
				log("  Baseline: %s", baseline_file.c_str());
				log("  Current buffer: %s", serialized.c_str());
			}
		}
	}

	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		std::string key = "MOB " + std::to_string(mob_index[i].vnum);
		std::string serialized = SerializeMob(i);
		uint32_t crc = CRC32String(serialized);
		auto it = baseline.find(key);
		if (it != baseline.end() && it->second != crc)
		{
			++mob_mismatches;
			if (mob_mismatches <= max_mismatches_per_type)
			{
				log("MISMATCH MOB %d: baseline=%08X current=%08X", mob_index[i].vnum, it->second, crc);
				std::string baseline_file = std::string(baseline_dir) + "/mobs/" + std::to_string(mob_index[i].vnum) + ".txt";
				log("  Baseline: %s", baseline_file.c_str());
				log("  Current buffer: %s", serialized.c_str());
			}
		}
	}

	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		if (!obj_proto[i]) continue;
		std::string key = "OBJ " + std::to_string(obj_proto[i]->get_vnum());
		std::string serialized = SerializeObject(obj_proto[i]);
		uint32_t crc = CRC32String(serialized);
		auto it = baseline.find(key);
		if (it != baseline.end() && it->second != crc)
		{
			++obj_mismatches;
			if (obj_mismatches <= max_mismatches_per_type)
			{
				log("MISMATCH OBJ %d: baseline=%08X current=%08X", obj_proto[i]->get_vnum(), it->second, crc);
				std::string baseline_file = std::string(baseline_dir) + "/objects/" + std::to_string(obj_proto[i]->get_vnum()) + ".txt";
				log("  Baseline: %s", baseline_file.c_str());
				log("  Current buffer: %s", serialized.c_str());
			}
		}
	}

	for (int i = 0; i < top_of_trigt; ++i)
	{
		if (!trig_index[i]) continue;
		std::string key = "TRIG " + std::to_string(trig_index[i]->vnum);
		std::string serialized = SerializeTrigger(i);
		uint32_t crc = CRC32String(serialized);
		auto it = baseline.find(key);
		if (it != baseline.end() && it->second != crc)
		{
			++trig_mismatches;
			if (trig_mismatches <= max_mismatches_per_type)
			{
				log("MISMATCH TRIG %d: baseline=%08X current=%08X", trig_index[i]->vnum, it->second, crc);
				std::string baseline_file = std::string(baseline_dir) + "/triggers/" + std::to_string(trig_index[i]->vnum) + ".txt";
				log("  Baseline: %s", baseline_file.c_str());
				log("  Current buffer: %s", serialized.c_str());
			}
		}
	}

	log("=== Comparison Summary ===");
	log("Zone mismatches: %d", zone_mismatches);
	log("Room mismatches: %d", room_mismatches);
	log("Mob mismatches:  %d", mob_mismatches);
	log("Obj mismatches:  %d", obj_mismatches);
	log("Trig mismatches: %d", trig_mismatches);
	log("Total mismatches: %d", zone_mismatches + room_mismatches + mob_mismatches + obj_mismatches + trig_mismatches);
}

} // namespace WorldChecksum

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
