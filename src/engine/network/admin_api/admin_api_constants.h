/**
 \file admin_api_constants.h
 \brief Constants and enums for Admin API
 \authors Bylins team
 \date 2026-02-13

 This file contains all constants, limits, and enum classes used by the Admin API.
 Part of the Admin API refactoring to eliminate magic numbers and improve maintainability.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_ADMIN_API_CONSTANTS_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_ADMIN_API_CONSTANTS_H_

#include <cstddef>
#include <string_view>

namespace admin_api {

// ============================================================================
// Buffer Size Limits
// ============================================================================

/// Maximum size for large buffer accumulation (1 MB)
constexpr size_t kMaxLargeBufferSize = 1048576;

/// Maximum chunks allowed for chunked messages
constexpr int kMaxChunks = 4;

// ============================================================================
// Admin API Commands
// ============================================================================

/// All supported Admin API commands
enum class Command {
	kAuth,              // Authentication command

	// Mob operations
	kListMobs,
	kGetMob,
	kUpdateMob,
	kCreateMob,
	kDeleteMob,

	// Object operations
	kListObjects,
	kGetObject,
	kUpdateObject,
	kCreateObject,
	kDeleteObject,

	// Room operations
	kListRooms,
	kGetRoom,
	kUpdateRoom,
	kCreateRoom,
	kDeleteRoom,

	// Zone operations
	kListZones,
	kGetZone,
	kUpdateZone,

	// Trigger operations
	kListTriggers,
	kGetTrigger,
	kUpdateTrigger,
	kCreateTrigger,
	kDeleteTrigger,

	// Statistics and player info
	kGetStats,
	kGetPlayers,

	// Unknown/invalid command
	kUnknown
};

/// Convert command string to enum
constexpr Command StringToCommand(std::string_view cmd) {
	if (cmd == "auth") return Command::kAuth;

	// Mob commands
	if (cmd == "list_mobs") return Command::kListMobs;
	if (cmd == "get_mob") return Command::kGetMob;
	if (cmd == "update_mob") return Command::kUpdateMob;
	if (cmd == "create_mob") return Command::kCreateMob;
	if (cmd == "delete_mob") return Command::kDeleteMob;

	// Object commands
	if (cmd == "list_objects") return Command::kListObjects;
	if (cmd == "get_object") return Command::kGetObject;
	if (cmd == "update_object") return Command::kUpdateObject;
	if (cmd == "create_object") return Command::kCreateObject;
	if (cmd == "delete_object") return Command::kDeleteObject;

	// Room commands
	if (cmd == "list_rooms") return Command::kListRooms;
	if (cmd == "get_room") return Command::kGetRoom;
	if (cmd == "update_room") return Command::kUpdateRoom;
	if (cmd == "create_room") return Command::kCreateRoom;
	if (cmd == "delete_room") return Command::kDeleteRoom;

	// Zone commands
	if (cmd == "list_zones") return Command::kListZones;
	if (cmd == "get_zone") return Command::kGetZone;
	if (cmd == "update_zone") return Command::kUpdateZone;

	// Trigger commands
	if (cmd == "list_triggers") return Command::kListTriggers;
	if (cmd == "get_trigger") return Command::kGetTrigger;
	if (cmd == "update_trigger") return Command::kUpdateTrigger;
	if (cmd == "create_trigger") return Command::kCreateTrigger;
	if (cmd == "delete_trigger") return Command::kDeleteTrigger;

	// Stats and players
	if (cmd == "get_stats") return Command::kGetStats;
	if (cmd == "get_players") return Command::kGetPlayers;

	return Command::kUnknown;
}

/// Convert command enum to string
constexpr std::string_view CommandToString(Command cmd) {
	switch (cmd) {
		case Command::kAuth: return "auth";

		case Command::kListMobs: return "list_mobs";
		case Command::kGetMob: return "get_mob";
		case Command::kUpdateMob: return "update_mob";
		case Command::kCreateMob: return "create_mob";
		case Command::kDeleteMob: return "delete_mob";

		case Command::kListObjects: return "list_objects";
		case Command::kGetObject: return "get_object";
		case Command::kUpdateObject: return "update_object";
		case Command::kCreateObject: return "create_object";
		case Command::kDeleteObject: return "delete_object";

		case Command::kListRooms: return "list_rooms";
		case Command::kGetRoom: return "get_room";
		case Command::kUpdateRoom: return "update_room";
		case Command::kCreateRoom: return "create_room";
		case Command::kDeleteRoom: return "delete_room";

		case Command::kListZones: return "list_zones";
		case Command::kGetZone: return "get_zone";
		case Command::kUpdateZone: return "update_zone";

		case Command::kListTriggers: return "list_triggers";
		case Command::kGetTrigger: return "get_trigger";
		case Command::kUpdateTrigger: return "update_trigger";
		case Command::kCreateTrigger: return "create_trigger";
		case Command::kDeleteTrigger: return "delete_trigger";

		case Command::kGetStats: return "get_stats";
		case Command::kGetPlayers: return "get_players";

		case Command::kUnknown: return "unknown";
	}
	return "unknown";
}

}  // namespace admin_api

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_ADMIN_API_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
