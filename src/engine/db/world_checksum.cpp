// Part of Bylins http://www.mud.ru
// World checksum calculation for detecting changes during refactoring

#include "world_checksum.h"

#include "db.h"
#include "obj_prototypes.h"
#include "engine/entities/zone.h"
#include "engine/entities/room_data.h"
#include "engine/entities/char_data.h"
#include "utils/logger.h"

#include <sstream>
#include <vector>

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

	// Serialize room flags using safe dynamic buffer
	std::vector<char> flag_buf(8192);
	flag_buf[0] = '\0';
	if (room->flags_sprint(flag_buf.data(), ","))
	{
		oss << flag_buf.data();
	}
	oss << "|";

	// Serialize exits (proto only)
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
	{
		const auto &exit = room->dir_option_proto[dir];
		if (exit)
		{
			oss << dir << ":";
			oss << exit->to_room() << ":";
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
	log("=======================");
}

void SaveDetailedChecksums(const char *filename)
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

	fclose(f);
	log("Detailed checksums saved to %s", filename);
}

} // namespace WorldChecksum

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
