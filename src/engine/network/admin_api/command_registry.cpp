/**
 \file command_registry.cpp
 \brief Command dispatch registry implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "command_registry.h"
#include "crud_handlers.h"
#include "../../../engine/network/descriptor_data.h"
#include "../../../administration/accounts.h"
#include "../../../administration/password.h"

namespace admin_api {

using namespace admin_api::handlers;

// ============================================================================
// CommandRegistry Implementation
// ============================================================================

CommandRegistry& CommandRegistry::Instance()
{
	static CommandRegistry instance;
	return instance;
}

void CommandRegistry::Register(std::string_view command, CommandHandler handler)
{
	handlers_[std::string(command)] = handler;
}

bool CommandRegistry::Dispatch(std::string_view command, DescriptorData* d, const nlohmann::json& request)
{
	auto it = handlers_.find(std::string(command));
	if (it == handlers_.end())
	{
		return false;
	}

	// Call the registered handler
	it->second(d, request);
	return true;
}

bool CommandRegistry::IsRegistered(std::string_view command) const
{
	return handlers_.find(std::string(command)) != handlers_.end();
}

size_t CommandRegistry::Count() const
{
	return handlers_.size();
}

// ============================================================================
// Command Handlers (wrappers for CRUD handlers)
// ============================================================================

// Auth command (special case - not a CRUD operation)
static void HandleAuth(DescriptorData* d, const nlohmann::json& request)
{
	// Extract username and password from request
	std::string username = request.value("username", "");
	std::string password = request.value("password", "");

	if (username.empty() || password.empty())
	{
		SendErrorResponse(d, "Missing username or password");
		return;
	}

	// Authenticate using existing account system
	Account acc(username);

	// Verify password
	if (!acc.compare_password(password))
	{
		SendErrorResponse(d, "Authentication failed");
		return;
	}

	// Mark descriptor as authenticated
	d->admin_api_authenticated = true;

	// Send success response
	nlohmann::json response;
	response["status"] = "ok";
	response["message"] = "Authentication successful";
	SendJsonResponse(d, response);
}

// Zone commands
static void HandleListZonesCommand(DescriptorData* d, const nlohmann::json& request)
{
	(void)request;  // No parameters needed
	HandleListZones(d);
}

static void HandleGetZoneCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleGetZone(d, vnum);
}

static void HandleUpdateZoneCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	std::string data_str = request.value("data", "");
	HandleUpdateZone(d, vnum, data_str.c_str());
}

// Mob commands
static void HandleListMobsCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string zone = request.value("zone", "");
	HandleListMobs(d, zone.c_str());
}

static void HandleGetMobCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleGetMob(d, vnum);
}

static void HandleUpdateMobCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	std::string data_str = request.dump();  // Pass full request as JSON
	HandleUpdateMob(d, vnum, data_str.c_str());
}

static void HandleCreateMobCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string data_str = request.dump();
	HandleCreateMob(d, data_str.c_str());
}

static void HandleDeleteMobCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleDeleteMob(d, vnum);
}

// Object commands
static void HandleListObjectsCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string zone = request.value("zone", "");
	HandleListObjects(d, zone.c_str());
}

static void HandleGetObjectCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleGetObject(d, vnum);
}

static void HandleUpdateObjectCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	std::string data_str = request.dump();
	HandleUpdateObject(d, vnum, data_str.c_str());
}

static void HandleCreateObjectCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string data_str = request.dump();
	HandleCreateObject(d, data_str.c_str());
}

static void HandleDeleteObjectCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleDeleteObject(d, vnum);
}

// Room commands
static void HandleListRoomsCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string zone = request.value("zone", "");
	HandleListRooms(d, zone.c_str());
}

static void HandleGetRoomCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleGetRoom(d, vnum);
}

static void HandleUpdateRoomCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	std::string data_str = request.dump();
	HandleUpdateRoom(d, vnum, data_str.c_str());
}

static void HandleCreateRoomCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string data_str = request.dump();
	HandleCreateRoom(d, data_str.c_str());
}

static void HandleDeleteRoomCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleDeleteRoom(d, vnum);
}

// Trigger commands
static void HandleListTriggersCommand(DescriptorData* d, const nlohmann::json& request)
{
	std::string zone = request.value("zone", "");
	HandleListTriggers(d, zone.c_str());
}

static void HandleGetTriggerCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleGetTrigger(d, vnum);
}

static void HandleUpdateTriggerCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	std::string data_str = request.dump();
	HandleUpdateTrigger(d, vnum, data_str.c_str());
}

static void HandleCreateTriggerCommand(DescriptorData* d, const nlohmann::json& request)
{
	int zone = request.value("zone", -1);
	nlohmann::json data = request.value("data", nlohmann::json::object());
	HandleCreateTrigger(d, zone, data.dump().c_str());
}

static void HandleDeleteTriggerCommand(DescriptorData* d, const nlohmann::json& request)
{
	int vnum = request.value("vnum", -1);
	HandleDeleteTrigger(d, vnum);
}

// Statistics and players
static void HandleGetStatsCommand(DescriptorData* d, const nlohmann::json& request)
{
	(void)request;
	HandleGetStats(d);
}

static void HandleGetPlayersCommand(DescriptorData* d, const nlohmann::json& request)
{
	(void)request;
	HandleGetPlayers(d);
}

// ============================================================================
// Command Registry Initialization
// ============================================================================

void InitializeCommandRegistry()
{
	auto& registry = CommandRegistry::Instance();

	// Auth command
	registry.Register("auth", HandleAuth);

	// Zone commands
	registry.Register("list_zones", HandleListZonesCommand);
	registry.Register("get_zone", HandleGetZoneCommand);
	registry.Register("update_zone", HandleUpdateZoneCommand);

	// Mob commands
	registry.Register("list_mobs", HandleListMobsCommand);
	registry.Register("get_mob", HandleGetMobCommand);
	registry.Register("update_mob", HandleUpdateMobCommand);
	registry.Register("create_mob", HandleCreateMobCommand);
	registry.Register("delete_mob", HandleDeleteMobCommand);

	// Object commands
	registry.Register("list_objects", HandleListObjectsCommand);
	registry.Register("get_object", HandleGetObjectCommand);
	registry.Register("update_object", HandleUpdateObjectCommand);
	registry.Register("create_object", HandleCreateObjectCommand);
	registry.Register("delete_object", HandleDeleteObjectCommand);

	// Room commands
	registry.Register("list_rooms", HandleListRoomsCommand);
	registry.Register("get_room", HandleGetRoomCommand);
	registry.Register("update_room", HandleUpdateRoomCommand);
	registry.Register("create_room", HandleCreateRoomCommand);
	registry.Register("delete_room", HandleDeleteRoomCommand);

	// Trigger commands
	registry.Register("list_triggers", HandleListTriggersCommand);
	registry.Register("get_trigger", HandleGetTriggerCommand);
	registry.Register("update_trigger", HandleUpdateTriggerCommand);
	registry.Register("create_trigger", HandleCreateTriggerCommand);
	registry.Register("delete_trigger", HandleDeleteTriggerCommand);

	// Statistics and players
	registry.Register("get_stats", HandleGetStatsCommand);
	registry.Register("get_players", HandleGetPlayersCommand);
}

}  // namespace admin_api

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
