/**
 \file crud_handlers.h
 \brief CRUD operation handlers for Admin API
 \authors Bylins team
 \date 2026-02-13

 Provides high-level CRUD handlers that combine serializers, parsers,
 and OLC operations. These handlers replace the individual admin_api_*
 functions and significantly reduce code duplication.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_CRUD_HANDLERS_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_CRUD_HANDLERS_H_

#include "json_helpers.h"

// Forward declarations
struct DescriptorData;

namespace admin_api::handlers {

using json = nlohmann::json;

// ============================================================================
// Response Helpers
// ============================================================================

/**
 * \brief Send JSON response to descriptor
 * \param d Descriptor to send to
 * \param response JSON object to send
 */
void SendJsonResponse(DescriptorData* d, const json& response);

/**
 * \brief Send error response to descriptor
 * \param d Descriptor to send to
 * \param error_message Error message
 */
void SendErrorResponse(DescriptorData* d, const char* error_message);

// ============================================================================
// Zone Handlers
// ============================================================================

/**
 * \brief List all zones
 * \param d Descriptor to send response to
 */
void HandleListZones(DescriptorData* d);

/**
 * \brief Get zone details
 * \param d Descriptor to send response to
 * \param zone_vnum Zone virtual number
 */
void HandleGetZone(DescriptorData* d, int zone_vnum);

/**
 * \brief Update zone
 * \param d Descriptor to send response to
 * \param zone_vnum Zone virtual number
 * \param json_data JSON update data
 */
void HandleUpdateZone(DescriptorData* d, int zone_vnum, const char* json_data);

// ============================================================================
// Mob Handlers
// ============================================================================

/**
 * \brief List mobs in a zone
 * \param d Descriptor to send response to
 * \param zone_name Zone name (empty for all zones)
 */
void HandleListMobs(DescriptorData* d, const char* zone_name);

/**
 * \brief Get mob details
 * \param d Descriptor to send response to
 * \param mob_vnum Mob virtual number
 */
void HandleGetMob(DescriptorData* d, int mob_vnum);

/**
 * \brief Update mob
 * \param d Descriptor to send response to
 * \param mob_vnum Mob virtual number
 * \param json_data JSON update data
 */
void HandleUpdateMob(DescriptorData* d, int mob_vnum, const char* json_data);

/**
 * \brief Create new mob
 * \param d Descriptor to send response to
 * \param json_data JSON mob data
 */
void HandleCreateMob(DescriptorData* d, const char* json_data);

/**
 * \brief Delete mob
 * \param d Descriptor to send response to
 * \param mob_vnum Mob virtual number
 */
void HandleDeleteMob(DescriptorData* d, int mob_vnum);

// ============================================================================
// Object Handlers
// ============================================================================

/**
 * \brief List objects in a zone
 * \param d Descriptor to send response to
 * \param zone_name Zone name (empty for all zones)
 */
void HandleListObjects(DescriptorData* d, const char* zone_name);

/**
 * \brief Get object details
 * \param d Descriptor to send response to
 * \param obj_vnum Object virtual number
 */
void HandleGetObject(DescriptorData* d, int obj_vnum);

/**
 * \brief Update object
 * \param d Descriptor to send response to
 * \param obj_vnum Object virtual number
 * \param json_data JSON update data
 */
void HandleUpdateObject(DescriptorData* d, int obj_vnum, const char* json_data);

/**
 * \brief Create new object
 * \param d Descriptor to send response to
 * \param json_data JSON object data
 */
void HandleCreateObject(DescriptorData* d, const char* json_data);

/**
 * \brief Delete object
 * \param d Descriptor to send response to
 * \param obj_vnum Object virtual number
 */
void HandleDeleteObject(DescriptorData* d, int obj_vnum);

// ============================================================================
// Room Handlers
// ============================================================================

/**
 * \brief List rooms in a zone
 * \param d Descriptor to send response to
 * \param zone_name Zone name (empty for all zones)
 */
void HandleListRooms(DescriptorData* d, const char* zone_name);

/**
 * \brief Get room details
 * \param d Descriptor to send response to
 * \param room_vnum Room virtual number
 */
void HandleGetRoom(DescriptorData* d, int room_vnum);

/**
 * \brief Update room
 * \param d Descriptor to send response to
 * \param room_vnum Room virtual number
 * \param json_data JSON update data
 */
void HandleUpdateRoom(DescriptorData* d, int room_vnum, const char* json_data);

/**
 * \brief Create new room
 * \param d Descriptor to send response to
 * \param json_data JSON room data
 */
void HandleCreateRoom(DescriptorData* d, const char* json_data);

/**
 * \brief Delete room
 * \param d Descriptor to send response to
 * \param room_vnum Room virtual number
 */
void HandleDeleteRoom(DescriptorData* d, int room_vnum);

// ============================================================================
// Trigger Handlers
// ============================================================================

/**
 * \brief List triggers in a zone
 * \param d Descriptor to send response to
 * \param zone_name Zone name (empty for all zones)
 */
void HandleListTriggers(DescriptorData* d, const char* zone_name);

/**
 * \brief Get trigger details
 * \param d Descriptor to send response to
 * \param trig_vnum Trigger virtual number
 */
void HandleGetTrigger(DescriptorData* d, int trig_vnum);

/**
 * \brief Update trigger
 * \param d Descriptor to send response to
 * \param trig_vnum Trigger virtual number
 * \param json_data JSON update data
 */
void HandleUpdateTrigger(DescriptorData* d, int trig_vnum, const char* json_data);

/**
 * \brief Create new trigger
 * \param d Descriptor to send response to
 * \param zone_vnum Zone virtual number
 * \param json_data JSON trigger data
 */
void HandleCreateTrigger(DescriptorData* d, int zone_vnum, const char* json_data);

/**
 * \brief Delete trigger
 * \param d Descriptor to send response to
 * \param trig_vnum Trigger virtual number
 */
void HandleDeleteTrigger(DescriptorData* d, int trig_vnum);

// ============================================================================
// Statistics and Players
// ============================================================================

/**
 * \brief Get server statistics
 * \param d Descriptor to send response to
 */
void HandleGetStats(DescriptorData* d);

/**
 * \brief Get online players
 * \param d Descriptor to send response to
 */
void HandleGetPlayers(DescriptorData* d);

}  // namespace admin_api::handlers

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_CRUD_HANDLERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
