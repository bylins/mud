/**
 \file crud_handlers.cpp
 \brief CRUD operation handlers implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "crud_handlers.h"
#include "serializers.h"
#include "parsers.h"
#include "../../../engine/network/descriptor_data.h"
#include "../../../engine/db/db.h"
#include "../../../engine/db/obj_prototypes.h"
#include "../../../engine/entities/zone.h"
#include "../../../engine/olc/olc.h"
#include "../../../utils/utils.h"

// External declarations
extern MobRnum top_of_mobt;
extern IndexData *mob_index;
extern CharData *mob_proto;

// External OLC functions
extern void medit_save_internally(DescriptorData *d);

// External admin_api helpers (from admin_api.cpp)
extern void admin_api_send_json(DescriptorData *d, const char *json_str);
extern void admin_api_send_error(DescriptorData *d, const char *error_message);

namespace admin_api::handlers {

using namespace admin_api::serializers;
using namespace admin_api::parsers;

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
	catch (const json::parse_error& e)
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
	// TODO: Implement using serializers
	SendErrorResponse(d, "Not implemented yet");
}

void HandleGetZone(DescriptorData* d, int zone_vnum)
{
	// TODO: Implement using serializers
	(void)zone_vnum;  // Suppress unused warning
	SendErrorResponse(d, "Not implemented yet");
}

void HandleUpdateZone(DescriptorData* d, int zone_vnum, const char* json_data)
{
	// TODO: Implement using parsers
	(void)zone_vnum;
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleListMobs(DescriptorData* d, const char* zone_name)
{
	// TODO: Implement using serializers
	(void)zone_name;
	SendErrorResponse(d, "Not implemented yet");
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

void HandleListObjects(DescriptorData* d, const char* zone_name)
{
	// TODO: Implement
	(void)zone_name;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleGetObject(DescriptorData* d, int obj_vnum)
{
	// TODO: Implement using serializers
	(void)obj_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleUpdateObject(DescriptorData* d, int obj_vnum, const char* json_data)
{
	// TODO: Implement using parsers
	(void)obj_vnum;
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleCreateObject(DescriptorData* d, const char* json_data)
{
	// TODO: Implement
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleDeleteObject(DescriptorData* d, int obj_vnum)
{
	// TODO: Implement
	(void)obj_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleListRooms(DescriptorData* d, const char* zone_name)
{
	// TODO: Implement
	(void)zone_name;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleGetRoom(DescriptorData* d, int room_vnum)
{
	// TODO: Implement using serializers
	(void)room_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleUpdateRoom(DescriptorData* d, int room_vnum, const char* json_data)
{
	// TODO: Implement using parsers
	(void)room_vnum;
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleCreateRoom(DescriptorData* d, const char* json_data)
{
	// TODO: Implement
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleDeleteRoom(DescriptorData* d, int room_vnum)
{
	// TODO: Implement
	(void)room_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleListTriggers(DescriptorData* d, const char* zone_name)
{
	// TODO: Implement
	(void)zone_name;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleGetTrigger(DescriptorData* d, int trig_vnum)
{
	// TODO: Implement using serializers
	(void)trig_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleUpdateTrigger(DescriptorData* d, int trig_vnum, const char* json_data)
{
	// TODO: Implement using parsers
	(void)trig_vnum;
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleCreateTrigger(DescriptorData* d, const char* json_data)
{
	// TODO: Implement
	(void)json_data;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleDeleteTrigger(DescriptorData* d, int trig_vnum)
{
	// TODO: Implement
	(void)trig_vnum;
	SendErrorResponse(d, "Not implemented yet");
}

void HandleGetStats(DescriptorData* d)
{
	// TODO: Implement
	SendErrorResponse(d, "Not implemented yet");
}

void HandleGetPlayers(DescriptorData* d)
{
	// TODO: Implement
	SendErrorResponse(d, "Not implemented yet");
}

}  // namespace admin_api::handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
