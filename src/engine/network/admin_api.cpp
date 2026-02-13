/**
 \file admin_api.cpp
 \brief Admin API implementation via Unix Domain Socket
*/

#ifdef ENABLE_ADMIN_API
#include <sys/socket.h>

#include "admin_api.h"
#include "admin_api/command_registry.h"
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
#include "gameplay/affects/affect_contants.h"
#include "gameplay/mechanics/dead_load.h"
#include "third_party_libs/nlohmann/json.hpp"

#include <string>
#include <sstream>

using json = nlohmann::json;

// Helper function to convert KOI8-R to UTF-8
static std::string koi8r_to_utf8(const char *koi_str) {
	if (!koi_str) return "";

	char utf8_buf[kMaxStringLength];
	koi_to_utf8(const_cast<char*>(koi_str), utf8_buf);
	return std::string(utf8_buf);
}

// Convert KOI8-R string to UTF-8 for JSON
// Correct KOI8-R to UTF-8 conversion
// Use existing koi_to_utf8 function from utils
std::string koi8r_to_utf8(const std::string &koi8r) {
	char utf8_buf[kMaxSockBuf * 6];
	char koi8r_buf[kMaxSockBuf * 6];

	// Initialize buffers to prevent returning garbage if conversion fails
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



extern MobRnum top_of_mobt;
extern IndexData *mob_index;
extern CharData *mob_proto;

extern RoomRnum top_of_world;
class Rooms;
extern Rooms &world;

// Helper: find trigger rnum by vnum (no GetTrigRnum exists)
static int find_trig_rnum(int vnum) {
	for (int rnum = 0; rnum < top_of_trigt; ++rnum) {
		if (trig_index[rnum] && trig_index[rnum]->vnum == vnum) {
			return rnum;
		}
	}
	return -1;
}

// Admin API input processing (separate from game process_input)
int admin_api_process_input(DescriptorData *d) {
	char *ptr, *nl_pos;
	ssize_t bytes_read;

	size_t buf_length = strlen(d->inbuf);
	char *read_point = d->inbuf + buf_length;
	size_t space_left = kMaxRawInputLength - buf_length - 1;

	if (space_left <= 0) {
		// Buffer full but no newline yet - use large buffer for chunking
		if (d->admin_api_large_buffer.size() + buf_length > 1048576) { // 1MB limit
			log("Admin API: message too large (>1MB)");
			return -1;
		}
		d->admin_api_large_buffer.append(d->inbuf, buf_length);
		d->inbuf[0] = '\0';
		return 0; // Wait for more data
	}

	bytes_read = read(d->descriptor, read_point, space_left);

	if (bytes_read < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;  // No data available yet, not an error
		}
		// Real error
		log("Admin API: read error: %s", strerror(errno));
		return -1;
	} else if (bytes_read == 0) {
		// EOF - client disconnected
		log("Admin API: client disconnected (EOF)");
		return -1;
	}

	read_point[bytes_read] = '\0';

	int commands_processed = 0;
	for (ptr = d->inbuf; (nl_pos = strchr(ptr, '\n')); ptr = nl_pos + 1) {
		*nl_pos = '\0';

		// If we have accumulated data in large buffer, prepend it
		if (!d->admin_api_large_buffer.empty()) {
			d->admin_api_large_buffer.append(ptr);
			nanny(d, const_cast<char*>(d->admin_api_large_buffer.c_str()));
			d->admin_api_large_buffer.clear();
		} else {
			nanny(d, ptr);
		}
		commands_processed++;

		if (d->state == EConState::kClose) {
			return -1;
		}
	}

	if (*ptr) {
		size_t remaining = strlen(ptr);
		// If there's no newline, accumulate in large buffer
		if (!strchr(ptr, '\n')) {
			d->admin_api_large_buffer.append(ptr, remaining);
			d->inbuf[0] = '\0';
		} else {
			memmove(d->inbuf, ptr, remaining + 1);
		}
	} else {
		d->inbuf[0] = '\0';
	}

	return commands_processed;
}

// Check access rights (immortal or builder)
bool admin_api_is_authenticated(DescriptorData *d) {
	return d->admin_api_authenticated;
}

void admin_api_parse(DescriptorData *d, char *argument) {
	try {
		// Parse JSON command
		json request = json::parse(argument);
		std::string command = request.value("command", "");

		if (command.empty()) {
			admin_api_send_error(d, "Missing command field");
			return;
		}

		// Get command registry instance
		auto& registry = admin_api::CommandRegistry::Instance();

		// Special case: auth command doesn't require authentication
		if (command == "auth") {
			if (!registry.Dispatch(command, d, request)) {
				admin_api_send_error(d, "Auth command not registered");
			}
			return;
		}

		// All other commands require authentication
		if (!admin_api_is_authenticated(d)) {
			admin_api_send_error(d, "Not authenticated. Use 'auth' command first.");
			return;
		}

		// Log all authenticated API commands to immortals
		mudlog(fmt::format("Admin API: {} executed '{}'", d->admin_user_name, command), BRF, kLvlImplementator, IMLOG, true);

		// Dispatch command to registry
		if (!registry.Dispatch(command, d, request)) {
			admin_api_send_error(d, "Unknown command");
		}

	} catch (const json::exception &e) {
		std::string error_msg = std::string("JSON parse error: ") + e.what();
		admin_api_send_error(d, error_msg.c_str());
	} catch (const std::exception &e) {
		std::string error_msg = std::string("Error: ") + e.what();
		admin_api_send_error(d, error_msg.c_str());
	}
}

void admin_api_send_json(DescriptorData *d, const char *json) {
	std::string msg = std::string(json) + "\n";
	iosystem::write_to_descriptor(d->descriptor, msg.c_str(), msg.length());
}

void admin_api_send_error(DescriptorData *d, const char *error_msg) {
	json error;
	error["status"] = "error";
	error["error"] = error_msg;
	admin_api_send_json(d, error.dump().c_str());
}

#endif // ENABLE_ADMIN_API

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
