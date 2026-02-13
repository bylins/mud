/**
 \file serializers.h
 \brief Entity to JSON serialization for Admin API
 \authors Bylins team
 \date 2026-02-13

 Provides functions to convert game entities (mobs, objects, rooms, zones, triggers)
 to JSON format for Admin API responses. Eliminates code duplication and provides
 consistent serialization across all CRUD operations.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_SERIALIZERS_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_SERIALIZERS_H_

#include "json_helpers.h"
#include "../../../engine/entities/char_data.h"
#include "../../../engine/entities/obj_data.h"
#include "../../../engine/entities/room_data.h"
#include "../../../engine/entities/zone.h"

namespace admin_api::serializers {

using json = nlohmann::json;

// ============================================================================
// Mob Serialization
// ============================================================================

/**
 * \brief Serialize mob to JSON
 * \param mob Mob to serialize (mob_proto[rnum])
 * \param vnum Virtual number of the mob
 * \return JSON object with mob data
 *
 * Output format:
 * {
 *   "vnum": 100,
 *   "names": { "aliases", "nominative", ... },
 *   "descriptions": { "short_desc", "long_desc" },
 *   "stats": { "level", "hp", "damage", ... },
 *   "abilities": { "strength", "dexterity", ... },
 *   "flags": { "mob_flags": [...], "npc_flags": [...], "affect_flags": [...] },
 *   "triggers": [100, 101]
 * }
 */
json SerializeMob(const CharData& mob, int vnum);

// ============================================================================
// Object Serialization
// ============================================================================

/**
 * \brief Serialize object to JSON
 * \param obj Object to serialize (obj_proto[rnum])
 * \param vnum Virtual number of the object
 * \return JSON object with object data
 */
json SerializeObject(const CObjectPrototype& obj, int vnum);

// ============================================================================
// Room Serialization
// ============================================================================

/**
 * \brief Serialize room to JSON
 * \param room Room to serialize (world[rnum])
 * \param vnum Virtual number of the room
 * \return JSON object with room data
 */
json SerializeRoom(RoomData& room, int vnum);

// ============================================================================
// ZoneData Serialization
// ============================================================================

/**
 * \brief Serialize zone to JSON
 * \param zone ZoneData to serialize
 * \param vnum Virtual number of the zone
 * \return JSON object with zone data
 */
json SerializeZoneData(const ZoneData& zone, int vnum);

// ============================================================================
// Trigger Serialization
// ============================================================================

/**
 * \brief Serialize trigger to JSON
 * \param trig Trigger to serialize
 * \param vnum Virtual number of the trigger
 * \return JSON object with trigger data
 */
json SerializeTrigger(const Trigger& trig, int vnum);

}  // namespace admin_api::serializers

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_SERIALIZERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
