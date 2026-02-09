/**
 \file admin_api.cpp
 \brief Admin API implementation via Unix Domain Socket
*/

#ifdef ENABLE_ADMIN_API
#include <sys/socket.h>

#include "admin_api.h"
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
		log("Admin API: input buffer overflow");
		return -1;
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
		nanny(d, ptr);
		commands_processed++;

		if (d->state == EConState::kClose) {
			return -1;
		}
	}

	if (*ptr) {
		size_t remaining = strlen(ptr);
		memmove(d->inbuf, ptr, remaining + 1);
	} else {
		d->inbuf[0] = '\0';
	}

	return commands_processed;
}

// Check access rights (immortal or builder)
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

			// No normalization needed - player_table uses case-insensitive comparison

			// Authenticate with KOI8-R username
			if (admin_api_authenticate(d, username.c_str(), password.c_str())) {
				json response;
				response["status"] = "ok";
				response["message"] = "Authentication successful";
				admin_api_send_json(d, response.dump().c_str());

			// Notify immortals about admin connection
			mudlog(fmt::format("Admin API: {} connected and authenticated", username), BRF, kLvlImplementator, IMLOG, true);
			} else {
				admin_api_send_error(d, "Authentication failed");
			}
			mudlog(fmt::format("Admin API: authentication failed for user '{}'", username), BRF, kLvlGod, IMLOG, true);
			return;
		}

		// All commands require authentication
		if (!admin_api_is_authenticated(d)) {
			admin_api_send_error(d, "Not authenticated. Use 'auth' command first.");
			return;
		}


	// Log all API commands to immortals (except auth/ping)
	mudlog(fmt::format("Admin API: {} executed '{}'", d->admin_user_name, command), BRF, kLvlImplementator, IMLOG, true);
		// Commands requiring authentication
		if (command == "list_mobs") {
			std::string zone = request.value("zone", "");
			admin_api_list_mobs(d, zone.c_str());
		}
		else if (command == "get_mob") {
			int vnum = request.value("vnum", -1);
			admin_api_get_mob(d, vnum);
		}
		else if (command == "update_mob") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			admin_api_update_mob(d, vnum, data.dump().c_str());
		}
		else if (command == "create_mob") {
			int zone = request.value("zone", -1);
			json data = request.value("data", json::object());
			admin_api_create_mob(d, zone, data.dump().c_str());
		}
		else if (command == "delete_mob") {
			int vnum = request.value("vnum", -1);
			admin_api_delete_mob(d, vnum);
		}
		else if (command == "list_objects") {
			std::string zone = request.value("zone", "");
			admin_api_list_objects(d, zone.c_str());
		}
		else if (command == "get_object") {
			int vnum = request.value("vnum", -1);
			admin_api_get_object(d, vnum);
		}
		else if (command == "update_object") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			admin_api_update_object(d, vnum, data.dump().c_str());
		}
		else if (command == "create_object") {
			int zone = request.value("zone", -1);
			json data = request.value("data", json::object());
			admin_api_create_object(d, zone, data.dump().c_str());
		}
		else if (command == "delete_object") {
			int vnum = request.value("vnum", -1);
			admin_api_delete_object(d, vnum);
		}
		else if (command == "list_rooms") {
			std::string zone = request.value("zone", "");
			admin_api_list_rooms(d, zone.c_str());
		}
		else if (command == "get_room") {
			int vnum = request.value("vnum", -1);
			admin_api_get_room(d, vnum);
		}
		else if (command == "update_room") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			admin_api_update_room(d, vnum, data.dump().c_str());
		}
		else if (command == "create_room") {
			int zone = request.value("zone", -1);
			json data = request.value("data", json::object());
			admin_api_create_room(d, zone, data.dump().c_str());
		}
		else if (command == "delete_room") {
			int vnum = request.value("vnum", -1);
			admin_api_delete_room(d, vnum);
		}
		else if (command == "list_zones") {
			admin_api_list_zones(d);
		}
		else if (command == "get_zone") {
			int vnum = request.value("vnum", -1);
			admin_api_get_zone(d, vnum);
		}
		else if (command == "update_zone") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			admin_api_update_zone(d, vnum, data.dump().c_str());
		}
		else if (command == "list_triggers") {
			std::string zone = request.value("zone", "");
			admin_api_list_triggers(d, zone.c_str());
		}
		else if (command == "get_trigger") {
			int vnum = request.value("vnum", -1);
			admin_api_get_trigger(d, vnum);
		}
		else if (command == "update_trigger") {
			int vnum = request.value("vnum", -1);
			json data = request["data"];
			admin_api_update_trigger(d, vnum, data.dump().c_str());
		}
		else if (command == "create_trigger") {
			int zone = request.value("zone", -1);
			json data = request.value("data", json::object());
			admin_api_create_trigger(d, zone, data.dump().c_str());
		}
		else if (command == "delete_trigger") {
			int vnum = request.value("vnum", -1);
			admin_api_delete_trigger(d, vnum);
		}
		else if (command == "get_stats") {
			admin_api_get_stats(d);
		}
		else if (command == "get_players") {
			admin_api_get_players(d);
		}
		else {
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

void admin_api_list_mobs(DescriptorData *d, const char *zone_vnum_str) {
	json response;
	response["status"] = "ok";
	response["mobs"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);
	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

	if (zone_rnum < 0) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	MobRnum first_mob = zone.RnumMobsLocation.first;
	MobRnum last_mob = zone.RnumMobsLocation.second;

	// Validate bounds before iteration
	if (first_mob < 0 || first_mob > top_of_mobt) {
		first_mob = 0;
	}
	if (last_mob < 0 || last_mob > top_of_mobt) {
		last_mob = top_of_mobt;
	}

	for (MobRnum rnum = first_mob; rnum <= last_mob; ++rnum) {
		// Double-check bounds before accessing mob_index
		if (rnum < 0 || rnum > top_of_mobt) continue;
		if (!mob_index[rnum].vnum) continue;

		CharData &mob = mob_proto[rnum];

		json mob_obj;
		mob_obj["vnum"] = mob_index[rnum].vnum;
		mob_obj["name"] = koi8r_to_utf8(mob.get_npc_name());
		mob_obj["short_desc"] = koi8r_to_utf8(mob.player_data.long_descr);
		mob_obj["level"] = mob.GetLevel();

		response["mobs"].push_back(mob_obj);
	}

	admin_api_send_json(d, response.dump().c_str());
}

void admin_api_get_mob(DescriptorData *d, int mob_vnum) {
	MobRnum rnum = GetMobRnum(mob_vnum);

	if (rnum == kNobody) {
		admin_api_send_error(d, "Mob not found");
		return;
	}

	CharData &mob = mob_proto[rnum];

	json mob_obj;
	mob_obj["vnum"] = mob_vnum;

	// Names
	json names;
	names["aliases"] = koi8r_to_utf8(mob.get_npc_name());
	names["nominative"] = koi8r_to_utf8(mob.player_data.PNames[ECase::kNom]);
	names["genitive"] = koi8r_to_utf8(mob.player_data.PNames[ECase::kGen]);
	names["dative"] = koi8r_to_utf8(mob.player_data.PNames[ECase::kDat]);
	names["accusative"] = koi8r_to_utf8(mob.player_data.PNames[ECase::kAcc]);
	names["instrumental"] = koi8r_to_utf8(mob.player_data.PNames[ECase::kIns]);
	names["prepositional"] = koi8r_to_utf8(mob.player_data.PNames[ECase::kPre]);
	mob_obj["names"] = names;

	// Descriptions
	json descriptions;
	descriptions["short_desc"] = koi8r_to_utf8(mob.player_data.long_descr);
	descriptions["long_desc"] = koi8r_to_utf8(mob.player_data.description);
	mob_obj["descriptions"] = descriptions;

	// Stats
	json stats;
	stats["level"] = mob.GetLevel();
	stats["hitroll_penalty"] = GET_HR(&mob);
	stats["armor"] = GET_AC(&mob);

	json hp;
	hp["dice_count"] = static_cast<int>(mob.mem_queue.total);
	hp["dice_size"] = static_cast<int>(mob.mem_queue.stored);
	hp["bonus"] = mob.get_hit();
	stats["hp"] = hp;

	json damage;
	damage["dice_count"] = static_cast<int>(mob.mob_specials.damnodice);
	damage["dice_size"] = static_cast<int>(mob.mob_specials.damsizedice);
	damage["bonus"] = mob.real_abils.damroll;
	stats["damage"] = damage;

	// Add sex, race, alignment to stats
	stats["sex"] = static_cast<int>(mob.get_sex());
	stats["race"] = mob.get_race();
	stats["alignment"] = mob.char_specials.saved.alignment;

	mob_obj["stats"] = stats;

	// Abilities
	json abilities;
	abilities["strength"] = mob.get_str();
	abilities["dexterity"] = mob.get_dex();
	abilities["constitution"] = mob.get_con();
	abilities["intelligence"] = mob.get_int();
	abilities["wisdom"] = mob.get_wis();
	abilities["charisma"] = mob.get_cha();
	mob_obj["abilities"] = abilities;

	// Resistances
	json resistances;
	resistances["fire"] = mob.add_abils.apply_resistance[to_underlying(EResist::kFire)];
	resistances["air"] = mob.add_abils.apply_resistance[to_underlying(EResist::kAir)];
	resistances["water"] = mob.add_abils.apply_resistance[to_underlying(EResist::kWater)];
	resistances["earth"] = mob.add_abils.apply_resistance[to_underlying(EResist::kEarth)];
	resistances["vitality"] = mob.add_abils.apply_resistance[to_underlying(EResist::kVitality)];
	resistances["mind"] = mob.add_abils.apply_resistance[to_underlying(EResist::kMind)];
	resistances["immunity"] = mob.add_abils.apply_resistance[to_underlying(EResist::kImmunity)];
	mob_obj["resistances"] = resistances;

	// Savings
	json savings;
	savings["will"] = mob.add_abils.apply_saving_throw[to_underlying(ESaving::kWill)];
	savings["stability"] = mob.add_abils.apply_saving_throw[to_underlying(ESaving::kStability)];
	savings["reflex"] = mob.add_abils.apply_saving_throw[to_underlying(ESaving::kReflex)];
	mob_obj["savings"] = savings;

	// Position (nested object)
	json position;
	position["default_position"] = static_cast<int>(mob.mob_specials.default_pos);
	position["load_position"] = static_cast<int>(mob.GetPosition());
	mob_obj["position"] = position;

	// Behavior
	json behavior;
	behavior["class"] = static_cast<int>(mob.GetClass());
	behavior["attack_type"] = mob.mob_specials.attack_type;

	// Gold (dice format)
	json gold;
	gold["dice_count"] = mob.mob_specials.GoldNoDs;
	gold["dice_size"] = mob.mob_specials.GoldSiDs;
	gold["bonus"] = 0;  // No bonus field in mob_specials
	behavior["gold"] = gold;

	// Helpers (array of mob vnums) - empty for now
	behavior["helpers"] = json::array();

	mob_obj["behavior"] = behavior;

	// Flags
	json flags;
	flags["mob_flags"] = json::array();
	flags["affect_flags"] = json::array();
	flags["npc_flags"] = json::array();
	mob_obj["flags"] = flags;

	// Triggers
	if (mob.proto_script) {
		json triggers = json::array();
		for (const auto &trig_vnum : *mob.proto_script) {
			triggers.push_back(trig_vnum);
		}
		mob_obj["triggers"] = triggers;
	} else {
		mob_obj["triggers"] = json::array();
	}

	json result;
	result["status"] = "ok";
	result["mob"] = mob_obj;

	admin_api_send_json(d, result.dump().c_str());
}

void admin_api_update_mob(DescriptorData *d, int mob_vnum, const char *json_data) {
	// Use OLC medit_save_internally() for proper mob update logic
	extern void medit_save_internally(DescriptorData *d);

	MobRnum rnum = GetMobRnum(mob_vnum);
	if (rnum == kNobody) {
		admin_api_send_error(d, "Mob not found");
		return;
	}

	try {
		json data = json::parse(json_data);

		// DEBUG: Log incoming JSON to understand the error
		log("Admin API: update_mob %d, incoming JSON: %s", mob_vnum, json_data);

		// Get zone
		int zone_vnum = mob_vnum / 100;
		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
		if (zone_rnum < 0) {
			admin_api_send_error(d, "Zone not found for mob");
			return;
		}

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		temp_d->olc->number = mob_vnum;
		temp_d->olc->zone_num = zone_rnum;

		// Create a copy of the mob for editing
		temp_d->olc->mob = new CharData(mob_proto[rnum]);
		temp_d->olc->mob->set_rnum(rnum);  // Mark as existing mob

		// Apply JSON fields to the copy
		CharData *mob = temp_d->olc->mob;

		// Update names
		if (data.contains("names")) {
			if (data["names"].contains("aliases")) {
				mob->set_npc_name(utf8_to_koi8r(data["names"]["aliases"].get<std::string>()));
			}
			if (data["names"].contains("nominative")) {
				mob->player_data.PNames[ECase::kNom] = utf8_to_koi8r(data["names"]["nominative"].get<std::string>());
			}
			if (data["names"].contains("genitive")) {
				mob->player_data.PNames[ECase::kGen] = utf8_to_koi8r(data["names"]["genitive"].get<std::string>());
			}
			if (data["names"].contains("dative")) {
				mob->player_data.PNames[ECase::kDat] = utf8_to_koi8r(data["names"]["dative"].get<std::string>());
			}
			if (data["names"].contains("accusative")) {
				mob->player_data.PNames[ECase::kAcc] = utf8_to_koi8r(data["names"]["accusative"].get<std::string>());
			}
			if (data["names"].contains("instrumental")) {
				mob->player_data.PNames[ECase::kIns] = utf8_to_koi8r(data["names"]["instrumental"].get<std::string>());
			}
			if (data["names"].contains("prepositional")) {
				mob->player_data.PNames[ECase::kPre] = utf8_to_koi8r(data["names"]["prepositional"].get<std::string>());
			}
		}

		// Update descriptions
		if (data.contains("descriptions")) {
			if (data["descriptions"].contains("short_desc")) {
				mob->player_data.long_descr = utf8_to_koi8r(data["descriptions"]["short_desc"].get<std::string>());
			}
			if (data["descriptions"].contains("long_desc")) {
				mob->player_data.description = utf8_to_koi8r(data["descriptions"]["long_desc"].get<std::string>());
			}
		}

		// Update stats
		if (data.contains("stats")) {
			if (data["stats"].contains("level")) {
				mob->set_level(data["stats"]["level"].get<int>());
			}
			if (data["stats"].contains("armor")) {
				mob->real_abils.armor = data["stats"]["armor"].get<int>();
			}
			if (data["stats"].contains("hitroll_penalty")) {
				mob->real_abils.hitroll = data["stats"]["hitroll_penalty"].get<int>();
			}
			// Sex (in stats from form)
			if (data["stats"].contains("sex")) {
				mob->set_sex(static_cast<EGender>(data["stats"]["sex"].get<int>()));
			}
			// Race (in stats from form)
			if (data["stats"].contains("race")) {
				mob->set_race(data["stats"]["race"].get<int>());
			}
			// Alignment
			if (data["stats"].contains("alignment")) {
				mob->char_specials.saved.alignment = data["stats"]["alignment"].get<int>();
			}
			// HP (dice format)
			if (data["stats"].contains("hp")) {
				auto hp = data["stats"]["hp"];
				if (hp.contains("dice_count")) {
					mob->mem_queue.total = hp["dice_count"].get<int>();
				}
				if (hp.contains("dice_size")) {
					mob->mem_queue.stored = hp["dice_size"].get<int>();
				}
				if (hp.contains("bonus")) {
					mob->set_hit(hp["bonus"].get<int>());
				}
			}
			// Damage (dice format)
			if (data["stats"].contains("damage")) {
				auto dmg = data["stats"]["damage"];
				if (dmg.contains("dice_count")) {
					mob->mob_specials.damnodice = dmg["dice_count"].get<int>();
				}
				if (dmg.contains("dice_size")) {
					mob->mob_specials.damsizedice = dmg["dice_size"].get<int>();
				}
				if (dmg.contains("bonus")) {
					mob->real_abils.damroll = dmg["bonus"].get<int>();
				}
			}
		}

		// Update abilities (separate section with full names)
		if (data.contains("abilities")) {
			if (data["abilities"].contains("strength")) {
				mob->set_str(data["abilities"]["strength"].get<int>());
			}
			if (data["abilities"].contains("dexterity")) {
				mob->set_dex(data["abilities"]["dexterity"].get<int>());
			}
			if (data["abilities"].contains("constitution")) {
				mob->set_con(data["abilities"]["constitution"].get<int>());
			}
			if (data["abilities"].contains("intelligence")) {
				mob->set_int(data["abilities"]["intelligence"].get<int>());
			}
			if (data["abilities"].contains("wisdom")) {
				mob->set_wis(data["abilities"]["wisdom"].get<int>());
			}
			if (data["abilities"].contains("charisma")) {
				mob->set_cha(data["abilities"]["charisma"].get<int>());
			}
		}

		// === RESISTANCES ===
		if (data.contains("resistances")) {
			if (data["resistances"].contains("fire")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kFire)] = data["resistances"]["fire"].get<int>();
			}
			if (data["resistances"].contains("air")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kAir)] = data["resistances"]["air"].get<int>();
			}
			if (data["resistances"].contains("water")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kWater)] = data["resistances"]["water"].get<int>();
			}
			if (data["resistances"].contains("earth")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kEarth)] = data["resistances"]["earth"].get<int>();
			}
			if (data["resistances"].contains("vitality")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kVitality)] = data["resistances"]["vitality"].get<int>();
			}
			if (data["resistances"].contains("mind")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kMind)] = data["resistances"]["mind"].get<int>();
			}
			if (data["resistances"].contains("immunity")) {
				mob->add_abils.apply_resistance[to_underlying(EResist::kImmunity)] = data["resistances"]["immunity"].get<int>();
			}
		}

		// === SAVINGS ===
		if (data.contains("savings")) {
			if (data["savings"].contains("will")) {
				mob->add_abils.apply_saving_throw[to_underlying(ESaving::kWill)] = data["savings"]["will"].get<int>();
			}
			if (data["savings"].contains("stability")) {
				mob->add_abils.apply_saving_throw[to_underlying(ESaving::kStability)] = data["savings"]["stability"].get<int>();
			}
			if (data["savings"].contains("reflex")) {
				mob->add_abils.apply_saving_throw[to_underlying(ESaving::kReflex)] = data["savings"]["reflex"].get<int>();
			}
		}

		// === POSITION (nested object) ===
		if (data.contains("position") && data["position"].is_object()) {
			if (data["position"].contains("default_position")) {
				mob->mob_specials.default_pos = static_cast<EPosition>(data["position"]["default_position"].get<int>());
			}
			if (data["position"].contains("load_position")) {
				mob->SetPosition(static_cast<EPosition>(data["position"]["load_position"].get<int>()));
			}
		}

		// === BEHAVIOR ===
		if (data.contains("behavior")) {
			if (data["behavior"].contains("class")) {
				mob->set_class(static_cast<ECharClass>(data["behavior"]["class"].get<int>()));
			}
			if (data["behavior"].contains("attack_type")) {
				mob->mob_specials.attack_type = data["behavior"]["attack_type"].get<int>();
			}
			// Gold (dice format)
			if (data["behavior"].contains("gold") && data["behavior"]["gold"].is_object()) {
				auto gold = data["behavior"]["gold"];
				if (gold.contains("dice_count")) {
					mob->mob_specials.GoldNoDs = gold["dice_count"].get<int>();
				}
				if (gold.contains("dice_size")) {
					mob->mob_specials.GoldSiDs = gold["dice_size"].get<int>();
				}
			}
			// Helpers (array of mob vnums)
			if (data["behavior"].contains("helpers") && data["behavior"]["helpers"].is_array()) {
				// Helpers are complex - for now just skip, rarely used
				// Would need to clear existing helpers and add new ones
			}
		}

		// Update class (legacy flat field, kept for backward compatibility)
		if (data.contains("class")) {
			mob->set_class(static_cast<ECharClass>(data["class"].get<int>()));
		}

		// Update flags (handle new nested object format)
		if (data.contains("flags") && data["flags"].is_object()) {
			// mob_flags array
			if (data["flags"].contains("mob_flags") && data["flags"]["mob_flags"].is_array()) {
				// Note: not clearing existing flags - just adding new ones
				// Full flag management would require clearing all flags first
				for (const auto &flag : data["flags"]["mob_flags"]) {
					if (flag.is_number_integer()) {
						mob->SetFlag(static_cast<EMobFlag>(flag.get<int>()));
					}
				}
			}
			// npc_flags array
			if (data["flags"].contains("npc_flags") && data["flags"]["npc_flags"].is_array()) {
				for (const auto &flag : data["flags"]["npc_flags"]) {
					if (flag.is_number_integer()) {
						mob->SetFlag(static_cast<ENpcFlag>(flag.get<int>()));
					}
				}
			}
			// affect_flags - ignore for now, complex
		}
		// Legacy flat flags array (backward compatibility)
		else if (data.contains("flags") && data["flags"].is_array()) {
			for (const auto &flag : data["flags"]) {
				mob->SetFlag(static_cast<EMobFlag>(flag.get<int>()));
			}
		}

		// Update NPC flags (legacy flat array)
		if (data.contains("npc_flags") && data["npc_flags"].is_array()) {
			for (const auto &flag : data["npc_flags"]) {
				mob->SetFlag(static_cast<ENpcFlag>(flag.get<int>()));
			}
		}
		// Update triggers (array of vnum integers)
		if (data.contains("triggers")) {
			// Initialize proto_script if needed
			if (!mob->proto_script) {
				mob->proto_script = std::make_shared<std::list<int>>();
			}
			mob->proto_script->clear();

			// Add new triggers
			for (const auto &trig_vnum_json : data["triggers"]) {
				int trig_vnum = trig_vnum_json.get<int>();

				// Verify trigger exists
				int trig_rnum = find_trig_rnum(trig_vnum);
				if (trig_rnum < 0) {
					log("Admin API: warning - trigger vnum %d not found for mob %d", trig_vnum, mob_vnum);
					continue;
				}

				mob->proto_script->push_back(trig_vnum);
			}
		}

		// Save via OLC (updates mob_proto, live mobs, and saves to disk)
		medit_save_internally(temp_d);

		// Clean up temporary descriptor (mob is freed by medit_save_internally)
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Mob updated successfully via OLC";
		admin_api_send_json(d, response.dump().c_str());
	} catch (const json::exception &e) {
		std::string error_msg = std::string("Update failed: ") + e.what();
		admin_api_send_error(d, error_msg.c_str());
	} catch (const std::exception &e) {
		std::string error_msg = std::string("Update failed: ") + e.what();
		admin_api_send_error(d, error_msg.c_str());
	} catch (...) {
		admin_api_send_error(d, "Update failed: unknown error");
	}
}

void admin_api_create_mob(DescriptorData *d, int zone_vnum, const char *json_data) {
	// Use OLC medit_mobile_init() and medit_save_internally() for proper mob creation
	extern void medit_mobile_init(CharData *mob);
	extern void medit_save_internally(DescriptorData *d);


	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
	if (zone_rnum < 0) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	try {
		json data = json::parse(json_data);

		// Determine vnum - either from data or auto-assign
		int vnum = data.value("vnum", -1);
		if (vnum < 0) {
			// Auto-assign: find first available vnum in zone range
			vnum = zone_vnum * 100;
			while (vnum < (zone_vnum + 1) * 100 && GetMobRnum(vnum) != kNobody) {
				++vnum;
			}
			if (vnum >= (zone_vnum + 1) * 100) {
				admin_api_send_error(d, "No available vnums in zone range");
				return;
			}
		} else {
			// Check vnum is in correct zone
			if (vnum / 100 != zone_vnum) {
				admin_api_send_error(d, "Vnum must be in zone range");
				return;
			}
			// Check vnum is available
			if (GetMobRnum(vnum) != kNobody) {
				admin_api_send_error(d, "Vnum already in use");
				return;
			}
		}

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		// Initialize output buffer to prevent crashes
		temp_d->output = temp_d->small_outbuf;
		temp_d->bufspace = kSmallBufsize - 1;
		*temp_d->output = '\0';
		temp_d->bufptr = 0;
		temp_d->olc->number = vnum;
		temp_d->olc->zone_num = zone_rnum;
		// Create dummy character to prevent OLC crashes when sending menus
		temp_d->character = std::make_shared<CharData>();
		temp_d->character->desc = temp_d;  // Set descriptor so SendMsgToChar works

		// Create new mob with OLC defaults
		temp_d->olc->mob = new CharData();
		medit_mobile_init(temp_d->olc->mob);
		temp_d->olc->mob->set_rnum(kNobody);  // Mark as new mob

		// Apply JSON fields
		CharData *mob = temp_d->olc->mob;
		if (data.contains("name")) {
			mob->set_npc_name(utf8_to_koi8r(data["name"].get<std::string>()));
		}
		if (data.contains("level")) {
			mob->set_level(data["level"].get<int>());
		}
		if (data.contains("short_desc")) {
			mob->player_data.long_descr = utf8_to_koi8r(data["short_desc"].get<std::string>());
		}

		// Save via OLC (handles array expansion, indexing, disk save)
		medit_save_internally(temp_d);

		// Capture OLC output
		std::string olc_output;
		if (temp_d->output && temp_d->bufptr > 0) {
			olc_output = koi8r_to_utf8(std::string(temp_d->output, temp_d->bufptr));
		}

		// Clean up temporary descriptor (mob freed by medit_save_internally)
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Mob created successfully via OLC";
		response["vnum"] = vnum;
		if (!olc_output.empty()) {
			response["olc_output"] = olc_output;
		}
		admin_api_send_json(d, response.dump().c_str());
	} catch (const json::exception &e) {
		std::string error_msg = std::string("Create failed: ") + e.what();
		admin_api_send_error(d, error_msg.c_str());
	} catch (const std::exception &e) {
		std::string error_msg = std::string("Create failed: ") + e.what();
		admin_api_send_error(d, error_msg.c_str());
	} catch (...) {
		admin_api_send_error(d, "Create failed: unknown error");
	}
}

void admin_api_delete_mob(DescriptorData *d, int mob_vnum) {
	(void)mob_vnum;  // Suppress unused parameter warning

	// Delete is dangerous - requires:
	// 1. Checking for live instances
	// 2. Removing from global arrays
	// 3. Shifting all subsequent mobs
	// 4. Updating all references (zone commands, triggers, etc.)
	// 5. Updating live mob rnums
	// This is extremely error-prone and can corrupt memory if not done perfectly.
	// For safety, refuse deletion via API.

	admin_api_send_error(d, "Mob deletion via API disabled for safety. Use in-game OLC instead.");
}

void admin_api_list_objects(DescriptorData *d, const char *zone_vnum_str) {
	json response;
	response["status"] = "ok";
	response["objects"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);
	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

	if (zone_rnum < 0) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	// Iterate through all objects and find those in this zone
	for (size_t i = 0; i < obj_proto.size(); ++i) {
		if (obj_proto.zone(i) != zone_rnum) {
			continue;
		}

		const auto &obj = obj_proto[i];

		json obj_data;
		obj_data["vnum"] = obj->get_vnum();
		obj_data["aliases"] = koi8r_to_utf8(obj->get_aliases());
		obj_data["short_desc"] = koi8r_to_utf8(obj->get_short_description());
		obj_data["type"] = static_cast<int>(obj->get_type());

		response["objects"].push_back(obj_data);
	}

	admin_api_send_json(d, response.dump().c_str());
}

void admin_api_get_object(DescriptorData *d, int obj_vnum) {
	ObjRnum rnum = GetObjRnum(obj_vnum);

	if (rnum < 0) {
		admin_api_send_error(d, "Object not found");
		return;
	}

	const auto &obj = obj_proto[rnum];

	json obj_data;
	obj_data["vnum"] = obj_vnum;
	obj_data["aliases"] = koi8r_to_utf8(obj->get_aliases());
	obj_data["short_desc"] = koi8r_to_utf8(obj->get_short_description());
	obj_data["description"] = koi8r_to_utf8(obj->get_description());
	obj_data["action_desc"] = koi8r_to_utf8(obj->get_action_description());
	obj_data["type"] = static_cast<int>(obj->get_type());
	
	// Extra flags (4 planes)
	obj_data["extra_flags"] = json::array();
	for (size_t i = 0; i < 4; ++i) {
		obj_data["extra_flags"].push_back(obj->get_extra_flags().get_plane(i));
	}
	
	obj_data["wear_flags"] = obj->get_wear_flags();
	obj_data["weight"] = obj->get_weight();
	obj_data["cost"] = obj->get_cost();
	obj_data["rent_on"] = obj->get_rent_on();
	obj_data["rent_off"] = obj->get_rent_off();

	// Names (7 case forms)
	json names;
	for (int c = ECase::kFirstCase; c <= ECase::kLastCase; ++c) {
		ECase ecase = static_cast<ECase>(c);
		std::string case_name;
		switch (ecase) {
			case ECase::kNom: case_name = "nominative"; break;
			case ECase::kGen: case_name = "genitive"; break;
			case ECase::kDat: case_name = "dative"; break;
			case ECase::kAcc: case_name = "accusative"; break;
			case ECase::kIns: case_name = "instrumental"; break;
			case ECase::kPre: case_name = "prepositional"; break;
			default: continue;
		}
		names[case_name] = koi8r_to_utf8(obj->get_PName(ecase));
	}
	obj_data["names"] = names;

	// Additional stats
	obj_data["level"] = obj->get_minimum_remorts();
	obj_data["sex"] = static_cast<int>(obj->get_sex());
	obj_data["material"] = static_cast<int>(obj->get_material());
	obj_data["timer"] = obj->get_timer();

	// Durability
	json durability;
	durability["current"] = obj->get_current_durability();
	durability["maximum"] = obj->get_maximum_durability();
	obj_data["durability"] = durability;

	// Type-specific values (value0-3)
	json type_specific;
	type_specific["value0"] = obj->get_val(0);
	type_specific["value1"] = obj->get_val(1);
	type_specific["value2"] = obj->get_val(2);
	type_specific["value3"] = obj->get_val(3);
	obj_data["type_specific"] = type_specific;

	// Affects (stat modifiers)
	json affects = json::array();
	for (int i = 0; i < kMaxObjAffect; ++i) {
		if (obj->get_affected(i).location != EApply::kNone) {
			json affect;
			affect["location"] = static_cast<int>(obj->get_affected(i).location);
			affect["modifier"] = obj->get_affected(i).modifier;
			affects.push_back(affect);
		}
	}
	obj_data["affects"] = affects;

	// Extra descriptions (linked list)
	json extra_descs = json::array();
	for (auto ed = obj->get_ex_description(); ed; ed = ed->next) {
		json extra;
		extra["keywords"] = koi8r_to_utf8(ed->keyword ? ed->keyword : "");
		extra["description"] = koi8r_to_utf8(ed->description ? ed->description : "");
		extra_descs.push_back(extra);
	}
	obj_data["extra_descriptions"] = extra_descs;

	// Triggers
	json triggers = json::array();
	for (const auto &trig_vnum : obj->get_proto_script()) {
		triggers.push_back(trig_vnum);
	}
	obj_data["triggers"] = triggers;

	json result;
	result["status"] = "ok";
	result["object"] = obj_data;

	admin_api_send_json(d, result.dump().c_str());
}

void admin_api_update_object(DescriptorData *d, int obj_vnum, const char *json_data) {

	ObjRnum rnum = GetObjRnum(obj_vnum);
	if (rnum < 0) {
		admin_api_send_error(d, "Object not found");
		return;
	}

	try {
		json data = json::parse(json_data);

		// Get zone
		int zone_vnum = obj_vnum / 100;
		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		temp_d->olc->number = obj_vnum;
		temp_d->olc->zone_num = zone_rnum;

		// Create a copy of the object for editing
		temp_d->olc->obj = new ObjData(*obj_proto[rnum]);
		temp_d->olc->obj->set_rnum(rnum);  // Mark as existing

		// Apply JSON fields
		auto &obj = temp_d->olc->obj;

		if (data.contains("aliases")) {
			obj->set_aliases(utf8_to_koi8r(data["aliases"].get<std::string>()));
		}
		if (data.contains("short_desc")) {
			obj->set_short_description(utf8_to_koi8r(data["short_desc"].get<std::string>()));
		}
		if (data.contains("description")) {
			obj->set_description(utf8_to_koi8r(data["description"].get<std::string>()));
		}
		if (data.contains("action_desc")) {
			obj->set_action_description(utf8_to_koi8r(data["action_desc"].get<std::string>()));
		}

		// Support both "pnames" and "names" keys
		auto names_data = data.contains("names") ? data["names"] : (data.contains("pnames") ? data["pnames"] : json{});
		if (!names_data.is_null()) {
			if (names_data.contains("nominative")) {
				obj->set_PName(ECase::kNom, utf8_to_koi8r(names_data["nominative"].get<std::string>()));
			}
			if (names_data.contains("genitive")) {
				obj->set_PName(ECase::kGen, utf8_to_koi8r(names_data["genitive"].get<std::string>()));
			}
			if (names_data.contains("dative")) {
				obj->set_PName(ECase::kDat, utf8_to_koi8r(names_data["dative"].get<std::string>()));
			}
			if (names_data.contains("accusative")) {
				obj->set_PName(ECase::kAcc, utf8_to_koi8r(names_data["accusative"].get<std::string>()));
			}
			if (names_data.contains("instrumental")) {
				obj->set_PName(ECase::kIns, utf8_to_koi8r(names_data["instrumental"].get<std::string>()));
			}
			if (names_data.contains("prepositional")) {
				obj->set_PName(ECase::kPre, utf8_to_koi8r(names_data["prepositional"].get<std::string>()));
			}
		}

		if (data.contains("weight")) {
			obj->set_weight(data["weight"].get<int>());
		}
		if (data.contains("cost")) {
			obj->set_cost(data["cost"].get<int>());
		}
		if (data.contains("rent_on")) {
			obj->set_rent_on(data["rent_on"].get<int>());
		}
		if (data.contains("rent_off")) {
			obj->set_rent_off(data["rent_off"].get<int>());
		}

		// Type
		if (data.contains("type")) {
			obj->set_type(static_cast<EObjType>(data["type"].get<int>()));
		}

		// Material
		if (data.contains("material")) {
			obj->set_material(static_cast<EObjMaterial>(data["material"].get<int>()));
		}

		// Sex
		if (data.contains("sex")) {
			obj->set_sex(static_cast<EGender>(data["sex"].get<int>()));
		}

		// Level
		if (data.contains("level")) {
			obj->set_level(data["level"].get<int>());
		}

		// Timer
		if (data.contains("timer")) {
			obj->set_timer(data["timer"].get<int>());
		}

		// Values (support both "values" array and "type_specific" object)
		if (data.contains("type_specific") && data["type_specific"].is_object()) {
			auto &ts = data["type_specific"];
			if (ts.contains("value0")) obj->set_val(0, ts["value0"].get<int>());
			if (ts.contains("value1")) obj->set_val(1, ts["value1"].get<int>());
			if (ts.contains("value2")) obj->set_val(2, ts["value2"].get<int>());
			if (ts.contains("value3")) obj->set_val(3, ts["value3"].get<int>());
		} else if (data.contains("values")) {
			for (size_t i = 0; i < 4 && i < data["values"].size(); ++i) {
				obj->set_val(i, data["values"][i].get<int>());
			}
		}

		// Extra flags
		if (data.contains("extra_flags")) {
			for (const auto &flag : data["extra_flags"]) {
				obj->set_extra_flag(static_cast<EObjFlag>(flag.get<int>()));
			}
		}

		// Wear flags
		if (data.contains("wear_flags")) {
			for (const auto &flag : data["wear_flags"]) {
				obj->set_wear_flag(static_cast<EWearFlag>(flag.get<int>()));
			}
		}

		// Anti flags
		if (data.contains("anti_flags")) {
			for (const auto &flag : data["anti_flags"]) {
				obj->set_anti_flag(static_cast<EAntiFlag>(flag.get<int>()));
			}
		}

		// No flags
		if (data.contains("no_flags")) {
			for (const auto &flag : data["no_flags"]) {
				obj->set_no_flag(static_cast<ENoFlag>(flag.get<int>()));
			}
		}

		// Affects
		if (data.contains("affects")) {
			size_t idx = 0;
			for (const auto &affect : data["affects"]) {
				if (idx >= kMaxObjAffect) break;
				if (affect.contains("location") && affect.contains("modifier")) {
					obj->set_affected(idx,
						static_cast<EApply>(affect["location"].get<int>()),
						affect["modifier"].get<int>());
					++idx;
				}
			}
		}

		// Durability (support both object and separate fields)
		if (data.contains("durability") && data["durability"].is_object()) {
			auto &dur = data["durability"];
			if (dur.contains("current")) {
				obj->set_current_durability(dur["current"].get<int>());
			}
			if (dur.contains("maximum")) {
				obj->set_maximum_durability(dur["maximum"].get<int>());
			}
		} else {
			if (data.contains("current_durability")) {
				obj->set_current_durability(data["current_durability"].get<int>());
			}
			if (data.contains("maximum_durability")) {
				obj->set_maximum_durability(data["maximum_durability"].get<int>());
			}
		}

		// Extra descriptions
		if (data.contains("extra_descriptions") && data["extra_descriptions"].is_array()) {
			// Clear existing extra descriptions
			obj->set_ex_description(nullptr);

			// Build linked list from array (in reverse to maintain order)
			ExtraDescription::shared_ptr prev = nullptr;
			for (auto it = data["extra_descriptions"].rbegin(); it != data["extra_descriptions"].rend(); ++it) {
				const auto &ed_data = *it;
				if (!ed_data.contains("keywords") || !ed_data.contains("description")) {
					continue;
				}

				auto ed = std::make_shared<ExtraDescription>();
				ed->set_keyword(utf8_to_koi8r(ed_data["keywords"].get<std::string>()));
				ed->set_description(utf8_to_koi8r(ed_data["description"].get<std::string>()));
				ed->next = prev;
				prev = ed;
			}

			if (prev) {
				obj->set_ex_description(prev);
			}
		}

		// Triggers
		if (data.contains("triggers") && data["triggers"].is_array()) {
			// Clear existing triggers and set new ones
			std::list<int> trigger_list;
			for (const auto &trig_vnum : data["triggers"]) {
				if (trig_vnum.is_number_integer()) {
					trigger_list.push_back(trig_vnum.get<int>());
				}
			}
			obj->set_proto_script(trigger_list);
		}

		// Save via OLC (handles live object updates, disk save)
		extern void oedit_save_internally(DescriptorData *d);
		oedit_save_internally(temp_d);

		// Clean up temporary descriptor
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Object updated successfully via OLC";
		admin_api_send_json(d, response.dump().c_str());

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Update failed: unknown error");
	}
}


void admin_api_create_object(DescriptorData *d, int zone_vnum, const char *json_data) {

	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
	if (zone_rnum < 0) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	try {
		json data = json::parse(json_data);

		// Find available vnum
		int vnum = data.value("vnum", -1);
		if (vnum < 0) {
			vnum = zone_vnum * 100;
			while (vnum < (zone_vnum + 1) * 100 && GetObjRnum(vnum) >= 0) {
				++vnum;
			}
			if (vnum >= (zone_vnum + 1) * 100) {
				admin_api_send_error(d, "No available vnums in zone range");
				return;
			}
		} else {
			if (vnum / 100 != zone_vnum) {
				admin_api_send_error(d, "Vnum must be in zone range");
				return;
			}
			if (GetObjRnum(vnum) >= 0) {
				admin_api_send_error(d, "Vnum already in use");
				return;
			}
		}

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		// Initialize output buffer to prevent crashes
		temp_d->output = temp_d->small_outbuf;
		temp_d->bufspace = kSmallBufsize - 1;
		*temp_d->output = '\0';
		temp_d->bufptr = 0;
		temp_d->olc->number = vnum;
		temp_d->olc->zone_num = zone_rnum;
		// Create dummy character to prevent OLC crashes when sending menus
		temp_d->character = std::make_shared<CharData>();
		temp_d->character->desc = temp_d;  // Set descriptor so SendMsgToChar works

		// Create object with OLC defaults (matching oedit_setup for real_num == -1)
		temp_d->olc->obj = new ObjData(vnum);
		auto &obj = temp_d->olc->obj;
		obj->set_aliases("new object");
		obj->set_description("Someone dropped something here");
		obj->set_short_description("new object");
		obj->set_PName(ECase::kNom, "new obj");
		obj->set_PName(ECase::kGen, "new obj");
		obj->set_PName(ECase::kDat, "new obj");
		obj->set_PName(ECase::kAcc, "new obj");
		obj->set_PName(ECase::kIns, "new obj");
		obj->set_PName(ECase::kPre, "new obj");
		obj->set_wear_flags(to_underlying(EWearFlag::kTake));
		obj->set_rnum(-1);  // Mark as new

		// Apply JSON fields
		if (data.contains("aliases")) {
			obj->set_aliases(utf8_to_koi8r(data["aliases"].get<std::string>()));
		}
		if (data.contains("short_desc")) {
			obj->set_short_description(utf8_to_koi8r(data["short_desc"].get<std::string>()));
		}
		if (data.contains("type")) {
			obj->set_type(static_cast<EObjType>(data["type"].get<int>()));
		}
		if (data.contains("weight")) {
			obj->set_weight(data["weight"].get<int>());
		}

		// Save via OLC (handles array expansion, indexing, disk save)
		extern void oedit_save_internally(DescriptorData *d);
		oedit_save_internally(temp_d);

		// Capture OLC output
		std::string olc_output;
		if (temp_d->output && temp_d->bufptr > 0) {
			olc_output = koi8r_to_utf8(std::string(temp_d->output, temp_d->bufptr));
		}

		// Clean up temporary descriptor (object is freed by oedit_save_internally)
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Object created successfully via OLC";
		response["vnum"] = vnum;
		if (!olc_output.empty()) {
			response["olc_output"] = olc_output;
		}
		admin_api_send_json(d, response.dump().c_str());

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Create failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Create failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Create failed: unknown error");
	}
}

void admin_api_delete_object(DescriptorData *d, int obj_vnum) {
	(void)obj_vnum;  // Suppress unused parameter warning

	// Delete is dangerous - requires:
	// 1. Checking for live instances (in game, on ground, in inventories)
	// 2. Removing from obj_proto vector
	// 3. Updating all references (zone commands, containers, etc.)
	// This is error-prone and can cause crashes if objects are in use.
	// For safety, refuse deletion via API.

	admin_api_send_error(d, "Object deletion via API disabled for safety. Use in-game OLC instead.");
}

void admin_api_list_rooms(DescriptorData *d, const char *zone_vnum_str) {
	json response;
	response["status"] = "ok";
	response["rooms"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);
	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

	if (zone_rnum < 0) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	RoomRnum first_room = zone.RnumRoomsLocation.first;
	RoomRnum last_room = zone.RnumRoomsLocation.second;

	for (RoomRnum rnum = first_room; rnum <= last_room && rnum <= top_of_world; ++rnum) {
		if (world[rnum]->vnum < 0) continue;

		json room_obj;
		room_obj["vnum"] = world[rnum]->vnum;
		room_obj["name"] = koi8r_to_utf8(world[rnum]->name);

		response["rooms"].push_back(room_obj);
	}

	admin_api_send_json(d, response.dump().c_str());
}

void admin_api_get_room(DescriptorData *d, int room_vnum) {
	RoomRnum rnum = GetRoomRnum(room_vnum);

	if (rnum == kNowhere) {
		admin_api_send_error(d, "Room not found");
		return;
	}

	RoomData *room = world[rnum];

	json room_obj;
	room_obj["vnum"] = room_vnum;
	room_obj["name"] = koi8r_to_utf8(room->name);
	// Description stored in GlobalObjects::descriptions(), skip for now
	if (room->temp_description) {
		room_obj["description"] = koi8r_to_utf8(room->temp_description);
	}
	room_obj["sector_type"] = static_cast<int>(room->sector_type);

	// Exits
	json exits = json::array();
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
		if (room->dir_option[dir]) {
			json exit_obj;
			exit_obj["direction"] = dir;
			exit_obj["to_room"] = room->dir_option[dir]->to_room() != kNowhere ?
				world[room->dir_option[dir]->to_room()]->vnum : -1;
			if (!room->dir_option[dir]->general_description.empty()) {
				exit_obj["description"] = koi8r_to_utf8(room->dir_option[dir]->general_description);
			}
			if (room->dir_option[dir]->keyword) {
				exit_obj["keyword"] = koi8r_to_utf8(room->dir_option[dir]->keyword);
			}
			exit_obj["exit_info"] = room->dir_option[dir]->exit_info;
			exit_obj["key_vnum"] = room->dir_option[dir]->key;
			exits.push_back(exit_obj);
		}
	}
	room_obj["exits"] = exits;

	// Extra descriptions (linked list)
	json extra_descrs = json::array();
	for (auto ed = room->ex_description; ed; ed = ed->next) {
		json ed_obj;
		if (ed->keyword) {
			ed_obj["keyword"] = koi8r_to_utf8(ed->keyword);
		}
		if (ed->description) {
			ed_obj["description"] = koi8r_to_utf8(ed->description);
		}
		extra_descrs.push_back(ed_obj);
	}
	room_obj["extra_descriptions"] = extra_descrs;

	// Triggers (array of vnums from prototype)
	json triggers = json::array();
	if (room->proto_script && !room->proto_script->empty()) {
		for (const auto &trig_vnum : *room->proto_script) {
			triggers.push_back(trig_vnum);
		}
	}
	room_obj["triggers"] = triggers;

	json result;
	result["status"] = "ok";
	result["room"] = room_obj;

	admin_api_send_json(d, result.dump().c_str());
}

void admin_api_update_room(DescriptorData *d, int room_vnum, const char *json_data) {

	RoomRnum rnum = GetRoomRnum(room_vnum);
	if (rnum == kNowhere) {
		admin_api_send_error(d, "Room not found");
		return;
	}

	try {
		json data = json::parse(json_data);
		ZoneRnum zone_rnum = world[rnum]->zone_rn;

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		temp_d->olc->number = room_vnum;
		temp_d->olc->zone_num = zone_rnum;

		// Create room copy for editing (matching redit_setup for real_num != kNowhere)
		RoomData *room = new RoomData;
		extern void CopyRoom(RoomData *to, RoomData *from);
		CopyRoom(room, world[rnum]);
		room->temp_description = str_dup(GlobalObjects::descriptions().get(world[rnum]->description_num).c_str());
		temp_d->olc->room = room;

		// Apply JSON fields
		if (data.contains("name")) {
			room->set_name(utf8_to_koi8r(data["name"].get<std::string>()));
		}
		if (data.contains("description")) {
			if (room->temp_description) {
				free(room->temp_description);
			}
			room->temp_description = str_dup(utf8_to_koi8r(data["description"].get<std::string>()).c_str());
		}
		if (data.contains("sector_type")) {
			room->sector_type = data["sector_type"].get<int>();
		}

		// Room flags
		if (data.contains("flags")) {
			room->clear_flags();
			for (const auto &flag : data["flags"]) {
				room->set_flag(flag.get<int>());
			}
		}

		// Exits (array format: [{direction: 0, ...}, ...]) - internal format (numbers)
		if (data.contains("exits")) {
			for (const auto &exit_json : data["exits"]) {
				// Parse direction as number (0-5)
				if (!exit_json.contains("direction")) continue;
				int dir = exit_json["direction"].get<int>();
				if (dir < 0 || dir >= EDirection::kMaxDirNum) continue;

				// Create exit if doesn't exist
				if (!room->dir_option[dir]) {
					room->dir_option[dir] = std::make_shared<ExitData>();
				}

				auto &exit = room->dir_option[dir];

				// Update exit fields
				if (exit_json.contains("to_room")) {
					exit->to_room(exit_json["to_room"].get<int>());
				}
				if (exit_json.contains("general_description")) {
					exit->general_description = utf8_to_koi8r(exit_json["general_description"].get<std::string>());
				}
				if (exit_json.contains("keyword")) {
					exit->set_keyword(utf8_to_koi8r(exit_json["keyword"].get<std::string>()));
				}
				if (exit_json.contains("exit_info")) {
					exit->exit_info = exit_json["exit_info"].get<int>();
				}
				if (exit_json.contains("key")) {
					exit->key = exit_json["key"].get<int>();
				}
				if (exit_json.contains("lock_complexity")) {
					exit->lock_complexity = exit_json["lock_complexity"].get<int>();
				}
			}
		}

		// Update triggers (array of vnum integers)
		if (data.contains("triggers")) {
			// Clear existing triggers
			room->proto_script->clear();
			
			// Add new triggers
			for (const auto &trig_vnum_json : data["triggers"]) {
				int trig_vnum = trig_vnum_json.get<int>();
				
				// Verify trigger exists
				int trig_rnum = find_trig_rnum(trig_vnum);
				if (trig_rnum < 0) {
					log("Admin API: warning - trigger vnum %d not found, skipping", trig_vnum);
					continue;
				}
				
				room->proto_script->push_back(trig_vnum);
			}
		}

		// Save via OLC (handles world update, disk save)
		extern void redit_save_internally(DescriptorData *d);
		redit_save_internally(temp_d);

		// Clean up temporary descriptor (room is freed by redit_save_internally)
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Room updated successfully via OLC";
		admin_api_send_json(d, response.dump().c_str());

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Update failed: unknown error");
	}
}

void admin_api_create_room(DescriptorData *d, int zone_vnum, const char *json_data) {

	try {
		json data = json::parse(json_data);
		
		// Extract vnum from JSON
		if (!data.contains("vnum")) {
			admin_api_send_error(d, "Missing 'vnum' field in JSON");
			return;
		}
		int vnum = data["vnum"].get<int>();
		
		// Get zone_rnum
		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
		if (zone_rnum == -1) {
			admin_api_send_error(d, "Zone not found");
			return;
		}
		
		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		// Initialize output buffer to prevent crashes
		temp_d->output = temp_d->small_outbuf;
		temp_d->bufspace = kSmallBufsize - 1;
		*temp_d->output = '\0';
		temp_d->bufptr = 0;
		temp_d->olc->number = vnum;
		temp_d->olc->zone_num = zone_rnum;
		
		// Initialize new room with defaults (redit_setup with kNowhere)
		// Create dummy character to prevent OLC crashes when sending menus
		temp_d->character = std::make_shared<CharData>();
		temp_d->character->desc = temp_d;  // Set descriptor so SendMsgToChar works
		extern void redit_setup(DescriptorData *d, int real_num);
		redit_setup(temp_d, kNowhere);
		
		RoomData *room = temp_d->olc->room;
		
		// Apply JSON fields
		if (data.contains("name")) {
			room->set_name(utf8_to_koi8r(data["name"].get<std::string>()));
		}
		if (data.contains("description")) {
			if (room->temp_description) {
				free(room->temp_description);
			}
			room->temp_description = str_dup(utf8_to_koi8r(data["description"].get<std::string>()).c_str());
		}
		if (data.contains("sector_type")) {
			room->sector_type = data["sector_type"].get<int>();
		}
		
		// Save via OLC (handles world insertion, disk save)
		extern void redit_save_internally(DescriptorData *d);
		redit_save_internally(temp_d);

		// Capture OLC output
		std::string olc_output;
		if (temp_d->output && temp_d->bufptr > 0) {
			olc_output = koi8r_to_utf8(std::string(temp_d->output, temp_d->bufptr));
		}
		
		// Clean up temporary descriptor (room is freed by redit_save_internally)
		delete temp_d->olc;
		delete temp_d;
		
		json response;
		response["status"] = "ok";
		response["message"] = "Room created successfully via OLC";
		response["vnum"] = vnum;
		if (!olc_output.empty()) {
			response["olc_output"] = olc_output;
		}
		admin_api_send_json(d, response.dump().c_str());
		
	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Create failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Create failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Create failed: unknown error");
	}
}





void admin_api_delete_room(DescriptorData *d, int room_vnum) {
	(void)room_vnum;  // Suppress unused parameter warning

	// Room deletion is dangerous - requires:
	// 1. Checking if mobs/objects are in the room
	// 2. Checking if exits point to this room
	// 3. Updating zone commands
	// 4. Shrinking world array
	// This is extremely error-prone and can corrupt world state.

	admin_api_send_error(d, "Room deletion via API disabled for safety. Use in-game OLC instead.");
}



// ==================== STATISTICS FUNCTIONS ====================

void admin_api_get_stats(DescriptorData *d) {
	json response;
	response["status"] = "ok";

	// Count zones (excluding zone_table[0] which is kNowhere)
	response["zones"] = static_cast<int>(zone_table.size()) - 1;

	// Count mob prototypes
	response["mobs"] = top_of_mobt + 1;

	// Count object prototypes
	response["objects"] = obj_proto.size();

	// Count rooms
	response["rooms"] = world.size();

	// Count trigger prototypes
	response["triggers"] = top_of_trigt + 1;

	admin_api_send_json(d, response.dump().c_str());
}

void admin_api_get_players(DescriptorData *d) {
	extern DescriptorData *descriptor_list;

	json response;
	response["status"] = "ok";
	response["players"] = json::array();

	// Iterate through all descriptors
	for (DescriptorData *desc = descriptor_list; desc; desc = desc->next) {
		// Skip non-playing connections (login screen, etc.)
		if (desc->state != EConState::kPlaying) {
			continue;
		}

		CharData *ch = desc->character.get();
		if (!ch) {
			continue;
		}

		json player;
		player["name"] = koi8r_to_utf8(ch->get_name().c_str());
		player["level"] = ch->GetLevel();
		player["remort"] = ch->get_remort();
		player["class"] = koi8r_to_utf8(MUD::Class(ch->GetClass()).GetName().c_str());
		player["room"] = GET_ROOM_VNUM(ch->in_room);

		// Add immortal flag
		player["is_immortal"] = (ch->GetLevel() >= kLvlImmortal);

		response["players"].push_back(player);
	}

	response["count"] = response["players"].size();

	admin_api_send_json(d, response.dump().c_str());
}

// ==================== ZONE FUNCTIONS ====================

void admin_api_list_zones(DescriptorData *d) {
	json response;
	response["status"] = "ok";
	response["zones"] = json::array();

	// Skip zone_table[0] which is reserved as kNowhere
	for (ZoneRnum zrn = 1; zrn < static_cast<ZoneRnum>(zone_table.size()); ++zrn) {
		const ZoneData &zone = zone_table[zrn];

		json zone_obj;
		zone_obj["vnum"] = zone.vnum;
		zone_obj["name"] = koi8r_to_utf8(zone.name);
		zone_obj["level"] = zone.level;
		if (!zone.author.empty()) {
			zone_obj["author"] = koi8r_to_utf8(zone.author);
		}

		response["zones"].push_back(zone_obj);
	}

	admin_api_send_json(d, response.dump().c_str());
}

void admin_api_get_zone(DescriptorData *d, int zone_vnum) {
	ZoneRnum zrn = GetZoneRnum(zone_vnum);

	if (zrn == -1) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	const ZoneData &zone = zone_table[zrn];

	json zone_obj;
	zone_obj["vnum"] = zone.vnum;
	zone_obj["name"] = koi8r_to_utf8(zone.name);
	
	if (!zone.comment.empty()) {
		zone_obj["comment"] = koi8r_to_utf8(zone.comment);
	}
	if (!zone.author.empty()) {
		zone_obj["author"] = koi8r_to_utf8(zone.author);
	}
	if (!zone.location.empty()) {
		zone_obj["location"] = koi8r_to_utf8(zone.location);
	}
	if (!zone.description.empty()) {
		zone_obj["description"] = koi8r_to_utf8(zone.description);
	}

	zone_obj["level"] = zone.level;
	zone_obj["type"] = zone.type;
	zone_obj["lifespan"] = zone.lifespan;
	zone_obj["reset_mode"] = zone.reset_mode;
	zone_obj["reset_idle"] = zone.reset_idle;
	zone_obj["top"] = zone.top;
	zone_obj["under_construction"] = zone.under_construction;
	zone_obj["group"] = zone.group;
	zone_obj["is_town"] = zone.is_town;

	json result;
	result["status"] = "ok";
	result["zone"] = zone_obj;

	admin_api_send_json(d, result.dump().c_str());
}

void admin_api_update_zone(DescriptorData *d, int zone_vnum, const char *json_data) {

	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	if (zrn == -1) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	try {
		json data = json::parse(json_data);

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		temp_d->olc->number = zone_vnum;
		temp_d->olc->zone_num = zrn;

		// Initialize zone editor (zedit_setup copies from zone_table[zrn])
		extern void zedit_setup(DescriptorData *d, int room_num);
		zedit_setup(temp_d, 0);  // room_num is unused in zedit_setup

		ZoneData *zone = temp_d->olc->zone;

		// Apply JSON fields
		if (data.contains("name")) {
			zone->name = utf8_to_koi8r(data["name"].get<std::string>());
		}
		if (data.contains("comment")) {
			zone->comment = utf8_to_koi8r(data["comment"].get<std::string>());
		}
		if (data.contains("author")) {
			zone->author = utf8_to_koi8r(data["author"].get<std::string>());
		}
		if (data.contains("location")) {
			zone->location = utf8_to_koi8r(data["location"].get<std::string>());
		}
		if (data.contains("description")) {
			zone->description = utf8_to_koi8r(data["description"].get<std::string>());
		}
		if (data.contains("level")) {
			zone->level = data["level"].get<int>();
		}
		if (data.contains("type")) {
			zone->type = data["type"].get<int>();
		}
		if (data.contains("lifespan")) {
			zone->lifespan = data["lifespan"].get<int>();
		}
		if (data.contains("reset_mode")) {
			zone->reset_mode = data["reset_mode"].get<int>();
		}
		if (data.contains("reset_idle")) {
			zone->reset_idle = data["reset_idle"].get<bool>();
		}
		if (data.contains("top")) {
			zone->top = data["top"].get<int>();
		}
		if (data.contains("under_construction")) {
			zone->under_construction = data["under_construction"].get<int>();
		}
		if (data.contains("group")) {
			zone->group = data["group"].get<int>();
		}
		if (data.contains("is_town")) {
			zone->is_town = data["is_town"].get<bool>();
		}

		// Mark zone as modified (zedit checks if vnum is non-zero)
		zone->vnum = 1;

		// Save via OLC (handles zone_table update, disk save)
		extern void zedit_save_internally(DescriptorData *d);
		zedit_save_internally(temp_d);

		// Clean up temporary descriptor
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Zone updated successfully via OLC";
		admin_api_send_json(d, response.dump().c_str());

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Update failed: unknown error");
	}
}



void admin_api_list_triggers(DescriptorData *d, const char *zone_vnum_str) {
	json response;
	response["status"] = "ok";
	response["triggers"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);

	// Iterate through all triggers and filter by zone
	for (int rnum = 0; rnum < top_of_trigt; ++rnum) {
		if (!trig_index[rnum]) continue;

		int trig_vnum = trig_index[rnum]->vnum;
		// Check if trigger belongs to the zone (vnums are zone*100 + offset)
		int trig_zone = trig_vnum / 100;
		if (trig_zone != zone_vnum) continue;

		if (!trig_index[rnum]->proto) {
			continue;
		}

		Trigger *trig = trig_index[rnum]->proto;

		json trig_obj;
		trig_obj["vnum"] = trig_vnum;
		trig_obj["name"] = koi8r_to_utf8(trig->get_name());
		trig_obj["attach_type"] = static_cast<int>(trig->get_attach_type());

		response["triggers"].push_back(trig_obj);
	}

	admin_api_send_json(d, response.dump().c_str());
}

void admin_api_get_trigger(DescriptorData *d, int trig_vnum) {
	int rnum = find_trig_rnum(trig_vnum);

	if (rnum < 0 || !trig_index[rnum] || !trig_index[rnum]->proto) {
		admin_api_send_error(d, "Trigger not found");
		return;
	}

	Trigger *trig = trig_index[rnum]->proto;

	json trig_obj;
	trig_obj["vnum"] = trig_vnum;
	trig_obj["name"] = koi8r_to_utf8(trig->get_name());
	trig_obj["attach_type"] = static_cast<int>(trig->get_attach_type());
	trig_obj["trigger_type"] = trig->get_trigger_type();
	trig_obj["narg"] = trig->narg;

	// Argument
	if (!trig->arglist.empty()) {
		trig_obj["arglist"] = koi8r_to_utf8(trig->arglist);
	}

	// Command list (linked list through shared_ptr)
	json commands = json::array();
	if (trig->cmdlist) {
		auto cmd = *trig->cmdlist;
		while (cmd) {
			commands.push_back(koi8r_to_utf8(cmd->cmd));
			cmd = cmd->next;
		}
	}
	trig_obj["commands"] = commands;

	json result;
	result["status"] = "ok";
	result["trigger"] = trig_obj;

	admin_api_send_json(d, result.dump().c_str());
}

void admin_api_update_trigger(DescriptorData *d, int trig_vnum, const char *json_data) {

	int rnum = find_trig_rnum(trig_vnum);
	if (rnum < 0 || !trig_index[rnum] || !trig_index[rnum]->proto) {
		admin_api_send_error(d, "Trigger not found");
		return;
	}

	try {
		json data = json::parse(json_data);
		int zone_vnum = trig_vnum / 100;
		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		// Initialize output buffer to prevent crashes
		temp_d->output = temp_d->small_outbuf;
		temp_d->bufspace = kSmallBufsize - 1;
		*temp_d->output = '\0';
		temp_d->bufptr = 0;
		temp_d->olc->number = trig_vnum;
		temp_d->olc->zone_num = zone_rnum;

		// Create dummy character to prevent OLC crashes when sending menus
		temp_d->character = std::make_shared<CharData>();
		if (!temp_d->character) {
			throw std::runtime_error("Failed to create CharData for Admin API");
		}
		temp_d->character->desc = temp_d;  // Set descriptor so SendMsgToChar works
		temp_d->character->set_name("AdminAPI");  // Set name for olc_log
		log("Admin API: Created character for trigedit, character=%p", temp_d->character.get());

		// Create trigger copy for editing
		Trigger *trig = new Trigger(*trig_index[rnum]->proto);
		// Ensure cmdlist is initialized (may be nullptr after copy if proto was empty)
		if (!trig->cmdlist) {
			trig->cmdlist.reset(new cmdlist_element::shared_ptr());
		}
		temp_d->olc->trig = trig;

		// Apply JSON fields
		if (data.contains("name")) {
			trig->set_name(utf8_to_koi8r(data["name"].get<std::string>()));
		}
		if (data.contains("arglist")) {
			trig->arglist = utf8_to_koi8r(data["arglist"].get<std::string>());
		}
		if (data.contains("narg")) {
			trig->narg = data["narg"].get<int>();
		}
		if (data.contains("trigger_type")) {
			trig->set_trigger_type(data["trigger_type"].get<long>());
		}
		if (data.contains("attach_type")) {
			trig->set_attach_type(static_cast<byte>(data["attach_type"].get<int>()));
		}

		// Script (OLC format: single text block with \n separators)
		if (data.contains("script")) {
			std::string script = utf8_to_koi8r(data["script"].get<std::string>());
			temp_d->olc->storage = str_dup(script.c_str());
		} else {
			// Build script from existing cmdlist if not provided
			std::string script;
			if (trig->cmdlist) {
				for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next) {
					if (!script.empty()) script += "\n";
					script += cmd->cmd;
				}
			}
			temp_d->olc->storage = str_dup(script.c_str());
		}

		// Save via OLC (compiles script, updates proto, updates live triggers, saves to disk)
		log("Admin API: About to call trigedit_save, character=%p", temp_d->character.get());
		if (!temp_d->character) {
			throw std::runtime_error("Character is nullptr before trigedit_save");
		}
		extern void trigedit_save(DescriptorData *d);
		trigedit_save(temp_d);
		log("Admin API: trigedit_save completed successfully");

		// Clean up
		if (temp_d->olc->storage) {
			free(temp_d->olc->storage);
		}
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Trigger updated successfully via OLC";
		admin_api_send_json(d, response.dump().c_str());

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Update failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Update failed: unknown error");
	}
}

void admin_api_create_trigger(DescriptorData *d, int zone_vnum, const char *json_data) {

	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
	if (zone_rnum < 0) {
		admin_api_send_error(d, "Zone not found");
		return;
	}

	try {
		json data = json::parse(json_data);

		// Find available vnum
		int vnum = data.value("vnum", -1);
		if (vnum < 0) {
			vnum = zone_vnum * 100;
			while (vnum < (zone_vnum + 1) * 100 && find_trig_rnum(vnum) >= 0) {
				++vnum;
			}
			if (vnum >= (zone_vnum + 1) * 100) {
				admin_api_send_error(d, "No available vnums in zone range");
				return;
			}
		} else {
			if (vnum / 100 != zone_vnum) {
				admin_api_send_error(d, "Vnum must be in zone range");
				return;
			}
			if (find_trig_rnum(vnum) >= 0) {
				admin_api_send_error(d, "Vnum already in use");
				return;
			}
		}

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		// Initialize output buffer to prevent crashes
		temp_d->output = temp_d->small_outbuf;
		temp_d->bufspace = kSmallBufsize - 1;
		*temp_d->output = '\0';
		temp_d->bufptr = 0;
		temp_d->olc->number = vnum;
		temp_d->olc->zone_num = zone_rnum;
		temp_d->character = d->character;  // For logging

		// Create dummy character to prevent OLC crashes when sending menus
		temp_d->character = std::make_shared<CharData>();
		temp_d->character->desc = temp_d;  // Set descriptor so SendMsgToChar works
		// Create trigger with OLC defaults (matching trigedit_setup_new)
		Trigger *trig = new Trigger(-1, "new trigger", MTRIG_GREET);
		trig->narg = 100;
		temp_d->olc->trig = trig;

		// Apply JSON fields
		if (data.contains("name")) {
			trig->set_name(utf8_to_koi8r(data["name"].get<std::string>()));
		}
		if (data.contains("arglist")) {
			trig->arglist = utf8_to_koi8r(data["arglist"].get<std::string>());
		}
		if (data.contains("narg")) {
			trig->narg = data["narg"].get<int>();
		}
		if (data.contains("trigger_type")) {
			trig->set_trigger_type(data["trigger_type"].get<long>());
		}
		if (data.contains("attach_type")) {
			trig->set_attach_type(static_cast<byte>(data["attach_type"].get<int>()));
		}

		// Script
		std::string script;
		if (data.contains("script")) {
			script = utf8_to_koi8r(data["script"].get<std::string>());
		} else {
			script = "say My trigger commandlist is not complete!";
		}
		temp_d->olc->storage = str_dup(script.c_str());

		// Save via OLC (handles array expansion, indexing, disk save)
		extern void trigedit_save(DescriptorData *d);
		trigedit_save(temp_d);

		// Capture OLC output
		std::string olc_output;
		if (temp_d->output && temp_d->bufptr > 0) {
			olc_output = koi8r_to_utf8(std::string(temp_d->output, temp_d->bufptr));
		}

		// Clean up
		if (temp_d->olc->storage) {
			free(temp_d->olc->storage);
		}
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Trigger created successfully via OLC";
		response["vnum"] = vnum;
		if (!olc_output.empty()) {
			response["olc_output"] = olc_output;
		}
		admin_api_send_json(d, response.dump().c_str());

	} catch (const json::exception &e) {
		admin_api_send_error(d, (std::string("Create failed: ") + e.what()).c_str());
	} catch (const std::exception &e) {
		admin_api_send_error(d, (std::string("Create failed: ") + e.what()).c_str());
	} catch (...) {
		admin_api_send_error(d, "Create failed: unknown error");
	}
}



void admin_api_delete_trigger(DescriptorData *d, int trig_vnum) {
	(void)trig_vnum;  // Suppress unused parameter warning

	// Trigger deletion is dangerous - requires:
	// 1. Checking if trigger is attached to any mobs/objects/rooms
	// 2. Removing from trig_index array
	// 3. Updating all references
	// 4. Freeing command list memory
	// This is error-prone and can cause crashes if trigger is in use.

	admin_api_send_error(d, "Trigger deletion via API disabled for safety. Use in-game OLC instead.");
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
