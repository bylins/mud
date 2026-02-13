/**
 \file admin_api.cpp
 \brief Admin API implementation via Unix Domain Socket
*/

#ifdef ENABLE_ADMIN_API
#include <sys/socket.h>

#include "admin_api.h"
#include "admin_api/crud_handlers.h"
#include "engine/db/db.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/world_data_source_manager.h"
#include "utils/id_converter.h"
#include "engine/db/world_objects.h"
#include "engine/db/world_characters.h"
#include "engine/db/global_objects.h"
#include "engine/entities/zone.h"
#include "engine/entities/char_player.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/olc/olc.h"
#include "administration/accounts.h"
#include "administration/password.h"
#include "utils/utils.h"
#include "gameplay/core/constants.h"
#include "third_party_libs/nlohmann/json.hpp"

#include <string>
#include <sstream>

using json = nlohmann::json;
using namespace admin_api::handlers;

// ============================================================================
// Helper functions for encoding conversion
// ============================================================================

// Convert KOI8-R string to UTF-8 for JSON
std::string koi8r_to_utf8(const std::string &koi8r) {
	char utf8_buf[kMaxSockBuf * 6];
	char koi8r_buf[kMaxSockBuf * 6];

	utf8_buf[0] = '\0';

	strncpy(koi8r_buf, koi8r.c_str(), sizeof(koi8r_buf) - 1);
	koi8r_buf[sizeof(koi8r_buf) - 1] = 0;

	koi_to_utf8(koi8r_buf, utf8_buf);

	return std::string(utf8_buf);
}

// Convert UTF-8 string to KOI8-R (for incoming JSON data)
std::string utf8_to_koi8r(const std::string &utf8) {
	char koi8r_buf[kMaxSockBuf * 6];
	char utf8_buf[kMaxSockBuf * 6];
	
	strncpy(utf8_buf, utf8.c_str(), sizeof(utf8_buf) - 1);
	utf8_buf[sizeof(utf8_buf) - 1] = '\0';

	utf8_to_koi(utf8_buf, koi8r_buf);

	return std::string(koi8r_buf);
}

// ============================================================================
// Admin API socket I/O and chunking
// ============================================================================

#define MAX_CHUNKS 64  // Max 64KB responses (64 * 1KB chunks)

// Admin API input processing (separate from game process_input)
int admin_api_process_input(DescriptorData *d) {
	char *ptr, *nl_pos;
	ssize_t bytes_read;

	size_t buf_length = strlen(d->inbuf);
	char *read_point = d->inbuf + buf_length;
	size_t space_left = kMaxRawInputLength - buf_length - 1;

	do {
		if (space_left <= 0) {
			log("SYSERR: Admin API input buffer overflow! Disconnecting.");
			return -1;
		}

		bytes_read = recv(d->descriptor, read_point, space_left, 0);

		if (bytes_read < 0) {
			if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
				return 0;
			}

			log("SYSERR: Admin API recv error: %s", strerror(errno));
			return -1;
		} else if (bytes_read == 0) {
			log("Admin API: client disconnected (EOF)");
			return -1;
		}

		*(read_point + bytes_read) = '\0';

		// Process complete lines
		for (ptr = d->inbuf; (nl_pos = strchr(ptr, '\n')); ptr = nl_pos + 1) {
			while (*nl_pos == '\n' || *nl_pos == '\r') {
				*nl_pos = '\0';
				nl_pos++;
			}

			if (*ptr) {
				admin_api_parse(d, ptr);
			}
		}

		// Move remaining data to start of buffer
		if (ptr > d->inbuf) {
			memmove(d->inbuf, ptr, strlen(ptr) + 1);
		}

		buf_length = strlen(d->inbuf);
		read_point = d->inbuf + buf_length;
		space_left = kMaxRawInputLength - buf_length - 1;

	} while (bytes_read > 0);

	return 0;
}

// ============================================================================
// Authentication
// ============================================================================

bool admin_api_authenticate(DescriptorData *d, const char *username, const char *password) {
	if (!username || !password) {
		return false;
	}

	// Check if authentication is required
	extern RuntimeConfiguration runtime_config;
	if (!runtime_config.admin_require_auth()) {
		d->admin_api_authenticated = true;
		d->admin_user_id = 1;
		log("Admin API: authentication bypassed (require_auth=false) for user '%s'", username);
		return true;
	}

	// Create temporary Player object for loading character
	auto temp_ch = std::make_shared<Player>();

	// Load player character by name
	int player_id = LoadPlayerCharacter(username, temp_ch.get(), ELoadCharFlags::kFindId);

	if (player_id < 0) {
		log("Admin API: user '%s' not found", username);
		return false;
	}

	// Check level (immortal or builder)
	if (temp_ch->GetLevel() < kLvlBuilder) {
		log("Admin API: user '%s' (level %d) - access denied (requires Builder+)", username, temp_ch->GetLevel());
		return false;
	}

	// Check password
	if (!Password::compare_password(temp_ch.get(), password)) {
		log("Admin API: user '%s' - incorrect password", username);
		return false;
	}

	// Authentication successful
	d->admin_api_authenticated = true;
	d->admin_user_id = temp_ch->get_uid();
	log("Admin API: user '%s' (ID %ld, level %d) authenticated successfully", username, temp_ch->get_uid(), temp_ch->GetLevel());
	return true;
}

bool admin_api_is_authenticated(DescriptorData *d) {
	return d->admin_api_authenticated;
}

// ============================================================================
// JSON response helpers
// ============================================================================

void admin_api_send_json(DescriptorData *d, const char *json_str) {
	if (!json_str) return;

	size_t len = strlen(json_str);
	const size_t chunk_size = 1024;
	size_t offset = 0;
	int chunk_num = 0;
	int total_chunks = (len + chunk_size - 1) / chunk_size;

	if (total_chunks > MAX_CHUNKS) {
		log("SYSERR: Admin API response too large (%zu bytes, %d chunks, max %d)", 
			len, total_chunks, MAX_CHUNKS);
		admin_api_send_error(d, "Response too large");
		return;
	}

	while (offset < len) {
		size_t this_chunk = std::min(chunk_size, len - offset);
		std::string chunk(json_str + offset, this_chunk);

		std::string header;
		if (total_chunks > 1) {
			header = fmt::format("CHUNK:{}/{}:", chunk_num, total_chunks);
		}

		std::string message = header + chunk + "\n";
		
		ssize_t written = send(d->descriptor, message.c_str(), message.length(), 0);
		if (written < 0) {
			log("SYSERR: Admin API send error: %s", strerror(errno));
			return;
		}

		offset += this_chunk;
		chunk_num++;
	}
}

void admin_api_send_error(DescriptorData *d, const char *error_msg) {
	json response;
	response["status"] = "error";
	response["error"] = error_msg;
	admin_api_send_json(d, response.dump().c_str());
}

// ============================================================================
// Command dispatcher
// ============================================================================

void admin_api_parse(DescriptorData *d, char *argument) {

	try {
		// Parse JSON command
		json request = json::parse(argument);

		std::string command = request.value("command", "");

		if (command == "auth") {
			std::string username_utf8 = request.value("username", "");
			std::string password = request.value("password", "");

			// Convert username from UTF-8 to KOI8-R (server's internal encoding)
			std::string username = utf8_to_koi8r(username_utf8);

			// Authenticate with KOI8-R username
			if (admin_api_authenticate(d, username.c_str(), password.c_str())) {
				json response;
				response["status"] = "ok";
				response["message"] = "Authentication successful";
				admin_api_send_json(d, response.dump().c_str());

				mudlog(fmt::format("Admin API: {} connected and authenticated", username), BRF, kLvlImplementator, IMLOG, true);
			} else {
				admin_api_send_error(d, "Authentication failed");
				mudlog(fmt::format("Admin API: authentication failed for user '{}'", username), BRF, kLvlGod, IMLOG, true);
			}
			return;
		}

		// All commands require authentication
		if (!admin_api_is_authenticated(d)) {
			admin_api_send_error(d, "Not authenticated. Use 'auth' command first.");
			return;
		}

		// Log all API commands to immortals (except auth/ping)
		mudlog(fmt::format("Admin API: {} executed '{}'", d->admin_user_name, command), BRF, kLvlImplementator, IMLOG, true);

		// ====================================================================
		// Dispatch to new modular handlers
		// ====================================================================

		// Zone commands
		if (command == "list_zones") {
			HandleListZones(d);
		}
		else if (command == "get_zone") {
			int vnum = request.value("vnum", -1);
			HandleGetZone(d, vnum);
		}
		else if (command == "update_zone") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			HandleUpdateZone(d, vnum, data.dump().c_str());
		}
		// Mob commands
		else if (command == "list_mobs") {
			std::string zone = request.value("zone", "");
			HandleListMobs(d, zone.c_str());
		}
		else if (command == "get_mob") {
			int vnum = request.value("vnum", -1);
			HandleGetMob(d, vnum);
		}
		else if (command == "update_mob") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			HandleUpdateMob(d, vnum, data.dump().c_str());
		}
		else if (command == "create_mob") {
			json data = request.value("data", json::object());
			HandleCreateMob(d, data.dump().c_str());
		}
		else if (command == "delete_mob") {
			int vnum = request.value("vnum", -1);
			HandleDeleteMob(d, vnum);
		}
		// Object commands
		else if (command == "list_objects") {
			std::string zone = request.value("zone", "");
			HandleListObjects(d, zone.c_str());
		}
		else if (command == "get_object") {
			int vnum = request.value("vnum", -1);
			HandleGetObject(d, vnum);
		}
		else if (command == "update_object") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			HandleUpdateObject(d, vnum, data.dump().c_str());
		}
		else if (command == "create_object") {
			json data = request.value("data", json::object());
			HandleCreateObject(d, data.dump().c_str());
		}
		else if (command == "delete_object") {
			int vnum = request.value("vnum", -1);
			HandleDeleteObject(d, vnum);
		}
		// Room commands
		else if (command == "list_rooms") {
			std::string zone = request.value("zone", "");
			HandleListRooms(d, zone.c_str());
		}
		else if (command == "get_room") {
			int vnum = request.value("vnum", -1);
			HandleGetRoom(d, vnum);
		}
		else if (command == "update_room") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			HandleUpdateRoom(d, vnum, data.dump().c_str());
		}
		else if (command == "create_room") {
			json data = request.value("data", json::object());
			HandleCreateRoom(d, data.dump().c_str());
		}
		else if (command == "delete_room") {
			int vnum = request.value("vnum", -1);
			HandleDeleteRoom(d, vnum);
		}
		// Trigger commands
		else if (command == "list_triggers") {
			std::string zone = request.value("zone", "");
			HandleListTriggers(d, zone.c_str());
		}
		else if (command == "get_trigger") {
			int vnum = request.value("vnum", -1);
			HandleGetTrigger(d, vnum);
		}
		else if (command == "update_trigger") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			HandleUpdateTrigger(d, vnum, data.dump().c_str());
		}
		else if (command == "create_trigger") {
			json data = request.value("data", json::object());
			HandleCreateTrigger(d, data.dump().c_str());
		}
		else if (command == "delete_trigger") {
			int vnum = request.value("vnum", -1);
			HandleDeleteTrigger(d, vnum);
		}
		// Stats and players
		else if (command == "get_stats") {
			HandleGetStats(d);
		}
		else if (command == "get_players") {
			HandleGetPlayers(d);
		}
		else {
			admin_api_send_error(d, "Unknown command");
		}

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("JSON error: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Error: ") + e.what()).c_str());
	}
}

#endif // ENABLE_ADMIN_API

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
