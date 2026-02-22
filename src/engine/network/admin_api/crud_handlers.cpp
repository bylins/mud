/**
 \file crud_handlers.cpp
 \brief CRUD operation handlers implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "crud_handlers.h"
#include "serializers.h"
#include "parsers.h"
#include "json_helpers.h"
#include "../../../engine/network/descriptor_data.h"
#include "../../../engine/db/db.h"
#include "../../../engine/db/obj_prototypes.h"
#include "../../../engine/db/world_data_source_manager.h"
#include "../../../engine/db/global_objects.h"
#include "../../../engine/entities/zone.h"
#include "../../../engine/entities/room_data.h"
#include "../../../engine/scripting/dg_scripts.h"
#include "../../../engine/olc/olc.h"
#include "../../../utils/utils.h"
#include "../../../gameplay/core/constants.h"

// External declarations
extern MobRnum top_of_mobt;
extern IndexData *mob_index;
extern CharData *mob_proto;
extern RoomRnum top_of_world;
extern TrgRnum top_of_trigt;
extern IndexData **trig_index;
extern DescriptorData *descriptor_list;

// External OLC functions
extern void medit_save_internally(DescriptorData *d);
extern void oedit_save_internally(DescriptorData *d);
extern void redit_save_internally(DescriptorData *d);
extern void trigedit_save(DescriptorData *d);
extern void CopyRoom(RoomData *to, RoomData *from);

// External admin_api helpers (from admin_api.cpp)
extern void admin_api_send_json(DescriptorData *d, const char *json_str);
extern void admin_api_send_error(DescriptorData *d, const char *error_message);

namespace admin_api::handlers {

using namespace admin_api::serializers;
using namespace admin_api::parsers;

// ============================================================================
// Helper Functions
// ============================================================================

static TrgRnum find_trig_rnum(TrgVnum vnum)
{
	for (int rnum = 0; rnum < top_of_trigt; ++rnum)
	{
		if (trig_index[rnum] && trig_index[rnum]->vnum == vnum)
		{
			return rnum;
		}
	}
	return -1;
}

// ============================================================================
// Response Helpers
// ============================================================================

void SendJsonResponse(DescriptorData* d, const json& response)
{
	admin_api_send_json(d, response.dump().c_str());
}

void SendErrorResponse(DescriptorData* d, const char* error_message)
{
	admin_api_send_error(d, error_message);
}

// ============================================================================
// Mob Handlers
// ============================================================================

void HandleGetMob(DescriptorData* d, int mob_vnum)
{
	MobRnum rnum = GetMobRnum(mob_vnum);

	if (rnum == kNobody)
	{
		SendErrorResponse(d, "Mob not found");
		return;
	}

	// Use serializer to convert mob to JSON
	json mob_data = SerializeMob(mob_proto[rnum], mob_vnum);

	json response;
	response["status"] = "ok";
	response["mob"] = mob_data;

	SendJsonResponse(d, response);
}

void HandleUpdateMob(DescriptorData* d, int mob_vnum, const char* json_data)
{
	MobRnum rnum = GetMobRnum(mob_vnum);

	if (rnum == kNobody)
	{
		SendErrorResponse(d, "Mob not found");
		return;
	}

	try
	{
		json data = json::parse(json_data);

		// Get zone
		int zone_vnum = mob_vnum / 100;
		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
		if (zone_rnum < 0)
		{
			SendErrorResponse(d, "Zone not found for mob");
			return;
		}

		// Create temporary OLC descriptor
		auto* temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		temp_d->olc->number = mob_vnum;
		temp_d->olc->zone_num = zone_rnum;

		// Create a copy of the mob for editing
		temp_d->olc->mob = new CharData(mob_proto[rnum]);
		temp_d->olc->mob->set_rnum(rnum);

		// Use parser to apply JSON updates
		ParseMobUpdate(temp_d->olc->mob, data);

		// Save changes using OLC
		medit_save_internally(temp_d);

		// Cleanup
		delete temp_d->olc->mob;
		delete temp_d->olc;
		delete temp_d;

		// Return updated mob data
		json response;
		response["status"] = "ok";
		response["message"] = "Mob updated successfully";
		response["mob"] = SerializeMob(mob_proto[rnum], mob_vnum);

		SendJsonResponse(d, response);
	}
	catch (const json::parse_error&)
	{
		SendErrorResponse(d, "Invalid JSON format");
	}
	catch (const std::exception& e)
	{
		SendErrorResponse(d, e.what());
	}
}

// ============================================================================
// Stub Handlers (for compilation - will be implemented in Stage 7)
// ============================================================================

void HandleListZones(DescriptorData* d)
{
	json response;
	response["status"] = "ok";
	response["zones"] = json::array();

	for (ZoneRnum zrn = 1; zrn < static_cast<ZoneRnum>(zone_table.size()); ++zrn)
	{
		const ZoneData &zone = zone_table[zrn];
		json zone_obj;
		zone_obj["vnum"] = zone.vnum;
		zone_obj["name"] = admin_api::json::Koi8rToUtf8(zone.name);
		zone_obj["level"] = zone.level;
		if (!zone.author.empty())
		{
			zone_obj["author"] = admin_api::json::Koi8rToUtf8(zone.author);
		}
		response["zones"].push_back(zone_obj);
	}

	SendJsonResponse(d, response);
}

void HandleGetZone(DescriptorData* d, int zone_vnum)
{
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	if (zrn == -1)
	{
		SendErrorResponse(d, "Zone not found");
		return;
	}

	const ZoneData &zone = zone_table[zrn];
	json zone_data = SerializeZoneData(zone, zone_vnum);

	json response;
	response["status"] = "ok";
	response["zone"] = zone_data;

	SendJsonResponse(d, response);
}

void HandleUpdateZone(DescriptorData* d, int zone_vnum, const char* json_data)
{
	ZoneRnum zrn = GetZoneRnum(zone_vnum);
	if (zrn == -1)
	{
		SendErrorResponse(d, "Zone not found");
		return;
	}

	try
	{
		json data = json::parse(json_data);
		ZoneData* zone = &zone_table[zrn];

		// Apply updates
		ParseZoneUpdate(zone, data);

		// Note: changes are applied directly to zone_table in memory and intentionally bypass
		// the OLC pipeline. OLC (zedit) also edits zone headers in memory only -- it defers
		// disk writes to an explicit "save zone" command issued by the builder. The Admin API
		// follows the same model: call the "save_zone" endpoint (or trigger OLC save) to
		// persist changes to disk via the active IWorldDataSource.

		json response;
		response["status"] = "ok";
		response["message"] = "Zone updated successfully";
		SendJsonResponse(d, response);
	}
	catch (const json::parse_error&)
	{
		SendErrorResponse(d, "Invalid JSON format");
	}
	catch (const std::exception& e)
	{
		SendErrorResponse(d, e.what());
	}
}

void HandleListMobs(DescriptorData* d, const char* zone_vnum_str)
{
	json response;
	response["status"] = "ok";
	response["mobs"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);
	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

	if (zone_rnum < 0)
	{
		SendErrorResponse(d, "Zone not found");
		return;
	}

	// Iterate through all mobs and find those in this zone
	for (MobRnum i = 0; i <= top_of_mobt; ++i)
	{
		int mob_vnum = mob_index[i].vnum;
		if (mob_vnum / 100 != zone_vnum)
		{
			continue;
		}

		json mob_data;
		mob_data["vnum"] = mob_vnum;
		mob_data["aliases"] = admin_api::json::Koi8rToUtf8(mob_proto[i].get_npc_name());
		mob_data["short_desc"] = admin_api::json::Koi8rToUtf8(mob_proto[i].player_data.long_descr);
		mob_data["level"] = mob_proto[i].GetLevel();

		response["mobs"].push_back(mob_data);
	}

	SendJsonResponse(d, response);
}

void HandleCreateMob(DescriptorData* d, const char* json_data)
{
	// TODO: Implement using parsers
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleDeleteMob(DescriptorData* d, int mob_vnum)
{
	// TODO: Implement
	(void)mob_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleListObjects(DescriptorData* d, const char* zone_vnum_str)
{
	json response;
	response["status"] = "ok";
	response["objects"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);
	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

	if (zone_rnum < 0)
	{
		SendErrorResponse(d, "Zone not found");
		return;
	}

	// Iterate through all objects and find those in this zone
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		if (obj_proto.zone(i) != zone_rnum)
		{
			continue;
		}

		const auto &obj = obj_proto[i];

		json obj_data;
		obj_data["vnum"] = obj->get_vnum();
		obj_data["aliases"] = admin_api::json::Koi8rToUtf8(obj->get_aliases());
		obj_data["short_desc"] = admin_api::json::Koi8rToUtf8(obj->get_short_description());
		obj_data["type"] = static_cast<int>(obj->get_type());

		response["objects"].push_back(obj_data);
	}

	SendJsonResponse(d, response);
}

void HandleGetObject(DescriptorData* d, int obj_vnum)
{
	ObjRnum rnum = GetObjRnum(obj_vnum);

	if (rnum < 0)
	{
		SendErrorResponse(d, "Object not found");
		return;
	}

	const auto &obj = obj_proto[rnum];
	json obj_data = SerializeObject(*obj, obj_vnum);

	json response;
	response["status"] = "ok";
	response["object"] = obj_data;

	SendJsonResponse(d, response);
}

void HandleUpdateObject(DescriptorData* d, int obj_vnum, const char* json_data)
{
	ObjRnum rnum = GetObjRnum(obj_vnum);
	if (rnum < 0)
	{
		SendErrorResponse(d, "Object not found");
		return;
	}

	try
	{
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
		temp_d->olc->obj->set_rnum(rnum);

		// Use parser to apply JSON updates
		ParseObjectUpdate(temp_d->olc->obj, data);

		// Save changes using OLC
		oedit_save_internally(temp_d);

		// Cleanup
		// NOTE: Do NOT delete temp_d->olc->obj - it was moved into obj_proto by oedit_save_internally
		temp_d->olc->obj = nullptr;  // Clear pointer to avoid accidental use
		delete temp_d->olc;
		delete temp_d;

		// Return updated object data
		json response;
		response["status"] = "ok";
		response["message"] = "Object updated successfully";
		response["object"] = SerializeObject(*obj_proto[rnum], obj_vnum);

		SendJsonResponse(d, response);
	}
	catch (const json::parse_error&)
	{
		SendErrorResponse(d, "Invalid JSON format");
	}
	catch (const std::exception& e)
	{
		SendErrorResponse(d, e.what());
	}
}

void HandleCreateObject(DescriptorData* d, const char* /* json_data */)
{
	SendErrorResponse(d, "Object creation not implemented - use in-game OLC");
}

void HandleDeleteObject(DescriptorData* d, int obj_vnum)
{
	(void)obj_vnum;
	SendErrorResponse(d, "Object deletion via API disabled for safety. Use in-game OLC instead.");
}

void HandleListRooms(DescriptorData* d, const char* zone_vnum_str)
{
	json response;
	response["status"] = "ok";
	response["rooms"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);
	ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

	if (zone_rnum < 0)
	{
		SendErrorResponse(d, "Zone not found");
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	RoomRnum first_room = zone.RnumRoomsLocation.first;
	RoomRnum last_room = zone.RnumRoomsLocation.second;

	for (RoomRnum rnum = first_room; rnum <= last_room && rnum <= top_of_world; ++rnum)
	{
		if (world[rnum]->vnum < 0) continue;

		json room_obj;
		room_obj["vnum"] = world[rnum]->vnum;
		room_obj["name"] = admin_api::json::Koi8rToUtf8(world[rnum]->name);

		response["rooms"].push_back(room_obj);
	}

	SendJsonResponse(d, response);
}

void HandleGetRoom(DescriptorData* d, int room_vnum)
{
	RoomRnum rnum = GetRoomRnum(room_vnum);

	if (rnum == kNowhere)
	{
		SendErrorResponse(d, "Room not found");
		return;
	}

	RoomData *room = world[rnum];
	json room_data = SerializeRoom(*room, room_vnum);

	json response;
	response["status"] = "ok";
	response["room"] = room_data;

	SendJsonResponse(d, response);
}

void HandleUpdateRoom(DescriptorData* d, int room_vnum, const char* json_data)
{
	RoomRnum rnum = GetRoomRnum(room_vnum);
	if (rnum == kNowhere)
	{
		SendErrorResponse(d, "Room not found");
		return;
	}

	try
	{
		json data = json::parse(json_data);
		ZoneRnum zone_rnum = world[rnum]->zone_rn;

		// Create temporary OLC descriptor
		auto *temp_d = new DescriptorData();
		temp_d->olc = new olc_data();
		temp_d->olc->number = room_vnum;
		temp_d->olc->zone_num = zone_rnum;

		// Create room copy for editing
		RoomData *room = new RoomData;
		CopyRoom(room, world[rnum]);
		// Get description from global storage
		room->temp_description = nullptr;
		if (world[rnum]->description_num > 0)
		{
			room->temp_description = str_dup(GlobalObjects::descriptions().get(world[rnum]->description_num).c_str());
		}
		temp_d->olc->room = room;

		// Apply JSON fields
		ParseRoomUpdate(room, data);

		// Save via OLC
		redit_save_internally(temp_d);

		// Cleanup
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Room updated successfully";
		SendJsonResponse(d, response);
	}
	catch (const json::parse_error&)
	{
		SendErrorResponse(d, "Invalid JSON format");
	}
	catch (const std::exception& e)
	{
		SendErrorResponse(d, e.what());
	}
}

void HandleCreateRoom(DescriptorData* d, const char* json_data)
{
	(void)json_data;
	SendErrorResponse(d, "Room creation not implemented - use in-game OLC");
}

void HandleDeleteRoom(DescriptorData* d, int room_vnum)
{
	(void)room_vnum;
	SendErrorResponse(d, "Room deletion via API disabled for safety. Use in-game OLC instead.");
}

void HandleListTriggers(DescriptorData* d, const char* zone_vnum_str)
{
	json response;
	response["status"] = "ok";
	response["triggers"] = json::array();

	int zone_vnum = atoi(zone_vnum_str);

	// Iterate through all triggers and filter by zone
	for (int rnum = 0; rnum < top_of_trigt; ++rnum)
	{
		if (!trig_index[rnum]) continue;

		int trig_vnum = trig_index[rnum]->vnum;
		// Check if trigger belongs to the zone
		int trig_zone = trig_vnum / 100;
		if (trig_zone != zone_vnum) continue;

		if (!trig_index[rnum]->proto)
		{
			continue;
		}

		Trigger *trig = trig_index[rnum]->proto;

		json trig_data;
		trig_data["vnum"] = trig_vnum;
		trig_data["name"] = admin_api::json::Koi8rToUtf8(trig->get_name());
		trig_data["attach_type"] = static_cast<int>(trig->get_attach_type());

		response["triggers"].push_back(trig_data);
	}

	SendJsonResponse(d, response);
}

void HandleGetTrigger(DescriptorData* d, int trig_vnum)
{
	TrgRnum rnum = find_trig_rnum(trig_vnum);

	if (rnum < 0 || !trig_index[rnum] || !trig_index[rnum]->proto)
	{
		SendErrorResponse(d, "Trigger not found");
		return;
	}

	Trigger *trig = trig_index[rnum]->proto;
	json trig_data = SerializeTrigger(*trig, trig_vnum);

	json response;
	response["status"] = "ok";
	response["trigger"] = trig_data;

	SendJsonResponse(d, response);
}

void HandleUpdateTrigger(DescriptorData* d, int trig_vnum, const char* json_data)
{
	using admin_api::json::Utf8ToKoi8r;
	using admin_api::json::Koi8rToUtf8;

	int rnum = find_trig_rnum(trig_vnum);
	if (rnum < 0 || !trig_index[rnum] || !trig_index[rnum]->proto)
	{
		SendErrorResponse(d, "Trigger not found");
		return;
	}

	try
	{
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
		temp_d->character->desc = temp_d;

		// Create trigger copy for editing
		Trigger *trig = new Trigger(*trig_index[rnum]->proto);
		// Ensure cmdlist is initialized (may be nullptr after copy if proto was empty)
		if (!trig->cmdlist)
		{
			trig->cmdlist.reset(new cmdlist_element::shared_ptr());
		}
		temp_d->olc->trig = trig;

		// Apply JSON fields via parser
		ParseTriggerUpdate(trig, data);

		// Script (OLC format: single text block with \n separators)
		if (data.contains("script"))
		{
			std::string script = Utf8ToKoi8r(data["script"].get<std::string>());
			temp_d->olc->storage = str_dup(script.c_str());
		}
		else
		{
			// Build script from existing cmdlist if not provided
			std::string script;
			if (trig->cmdlist)
			{
				for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next)
				{
					if (!script.empty()) script += "\n";
					script += cmd->cmd;
				}
			}
			temp_d->olc->storage = str_dup(script.c_str());
		}

		// Save via OLC (compiles script, updates proto, updates live triggers, saves to disk)
		trigedit_save(temp_d);

		// Clean up
		if (temp_d->olc->storage)
		{
			free(temp_d->olc->storage);
		}
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Trigger updated successfully via OLC";
		SendJsonResponse(d, response);
	}
	catch (const json::parse_error&)
	{
		SendErrorResponse(d, "Invalid JSON format");
	}
	catch (const std::exception& e)
	{
		SendErrorResponse(d, (std::string("Update failed: ") + e.what()).c_str());
	}
}

void HandleCreateTrigger(DescriptorData* d, int zone_vnum, const char* json_data)
{
	using admin_api::json::Utf8ToKoi8r;
	using admin_api::json::Koi8rToUtf8;

	try
	{
		// Parse data directly (zone comes as parameter)
		json data = json::parse(json_data);

		if (zone_vnum < 0)
		{
			SendErrorResponse(d, "Zone vnum required");
			return;
		}

		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);
		if (zone_rnum < 0)
		{
			SendErrorResponse(d, "Zone not found");
			return;
		}

		// Find available vnum
		int vnum = data.value("vnum", -1);
		if (vnum < 0)
		{
			vnum = zone_vnum * 100;
			while (vnum < (zone_vnum + 1) * 100 && find_trig_rnum(vnum) >= 0)
			{
				++vnum;
			}
			if (vnum >= (zone_vnum + 1) * 100)
			{
				SendErrorResponse(d, "No available vnums in zone range");
				return;
			}
		}
		else
		{
			if (vnum / 100 != zone_vnum)
			{
				SendErrorResponse(d, "Vnum must be in zone range");
				return;
			}
			if (find_trig_rnum(vnum) >= 0)
			{
				SendErrorResponse(d, "Vnum already in use");
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
		temp_d->character->desc = temp_d;

		// Create trigger with OLC defaults (matching trigedit_setup_new)
		Trigger *trig = new Trigger(-1, "new trigger", MTRIG_GREET);
		trig->narg = 100;
		temp_d->olc->trig = trig;

		// Apply JSON fields
		if (data.contains("name"))
		{
			trig->set_name(Utf8ToKoi8r(data["name"].get<std::string>()));
		}
		if (data.contains("arglist"))
		{
			trig->arglist = Utf8ToKoi8r(data["arglist"].get<std::string>());
		}
		if (data.contains("narg"))
		{
			trig->narg = data["narg"].get<int>();
		}
		if (data.contains("trigger_type"))
		{
			trig->set_trigger_type(data["trigger_type"].get<long>());
		}
		if (data.contains("attach_type"))
		{
			trig->set_attach_type(static_cast<byte>(data["attach_type"].get<int>()));
		}

		// Script
		std::string script;
		if (data.contains("script"))
		{
			script = Utf8ToKoi8r(data["script"].get<std::string>());
		}
		else
		{
			script = "say My trigger commandlist is not complete!";
		}
		temp_d->olc->storage = str_dup(script.c_str());

		// Save via OLC (handles array expansion, indexing, disk save)
		trigedit_save(temp_d);

		// Capture OLC output
		std::string olc_output;
		if (temp_d->output && temp_d->bufptr > 0)
		{
			olc_output = Koi8rToUtf8(std::string(temp_d->output, temp_d->bufptr));
		}

		// Clean up
		if (temp_d->olc->storage)
		{
			free(temp_d->olc->storage);
		}
		delete temp_d->olc;
		delete temp_d;

		json response;
		response["status"] = "ok";
		response["message"] = "Trigger created successfully via OLC";
		response["vnum"] = vnum;
		if (!olc_output.empty())
		{
			response["olc_output"] = olc_output;
		}
		SendJsonResponse(d, response);
	}
	catch (const json::parse_error&)
	{
		SendErrorResponse(d, "Invalid JSON format");
	}
	catch (const std::exception& e)
	{
		SendErrorResponse(d, (std::string("Create failed: ") + e.what()).c_str());
	}
}

void HandleDeleteTrigger(DescriptorData* d, int trig_vnum)
{
	(void)trig_vnum;
	SendErrorResponse(d, "Trigger deletion via API disabled for safety. Use in-game OLC instead.");
}

void HandleGetStats(DescriptorData* d)
{
	json response;
	response["status"] = "ok";

	// Count zones (skip zone_table[0])
	response["zones"] = zone_table.size() - 1;

	// Count mobs
	response["mobs"] = top_of_mobt + 1;

	// Count objects
	response["objects"] = obj_proto.size();

	// Count rooms
	response["rooms"] = world.size();

	// Count triggers
	response["triggers"] = top_of_trigt + 1;

	SendJsonResponse(d, response);
}

void HandleGetPlayers(DescriptorData* d)
{
	json response;
	response["status"] = "ok";
	response["players"] = json::array();

	// Iterate through all descriptors
	for (DescriptorData *desc = descriptor_list; desc; desc = desc->next)
	{
		// Skip non-playing connections
		bool is_playing = (desc->state == EConState::kPlaying);
		bool is_olc = (desc->state == EConState::kOedit || desc->state == EConState::kRedit ||
		               desc->state == EConState::kZedit || desc->state == EConState::kMedit ||
		               desc->state == EConState::kTrigedit || desc->state == EConState::kMredit ||
		               desc->state == EConState::kSedit);
		if (!is_playing && !is_olc)
		{
			continue;
		}

		CharData *ch = desc->character.get();
		if (!ch)
		{
			continue;
		}

		json player;
		player["name"] = admin_api::json::Koi8rToUtf8(ch->get_name().c_str());
		player["level"] = ch->GetLevel();
		player["remort"] = ch->get_remort();
		player["class"] = admin_api::json::Koi8rToUtf8(MUD::Class(ch->GetClass()).GetName().c_str());
		player["room"] = GET_ROOM_VNUM(ch->in_room);
		player["is_immortal"] = (ch->GetLevel() >= kLvlImmortal);

		response["players"].push_back(player);
	}

	response["count"] = response["players"].size();

	SendJsonResponse(d, response);
}

}  // namespace admin_api::handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
