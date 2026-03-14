/**
 \file parsers.h
 \brief JSON to Entity parsing for Admin API
 \authors Bylins team
 \date 2026-02-13

 Provides functions to parse JSON data and apply updates to game entities
 (mobs, objects, rooms, zones, triggers). Eliminates code duplication and
 provides consistent parsing across all CRUD operations.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_PARSERS_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_PARSERS_H_

#include "json_helpers.h"
#include "../../../engine/entities/char_data.h"
#include "../../../engine/entities/obj_data.h"
#include "../../../engine/entities/room_data.h"
#include "../../../engine/entities/zone.h"

namespace admin_api::parsers {

using json = nlohmann::json;

// ============================================================================
// Mob Parsing
// ============================================================================

/**
 * \brief Parse JSON and update mob fields
 * \param mob Mob to update (OLC copy, not mob_proto directly)
 * \param data JSON object with mob update data
 *
 * Updates mob fields based on JSON content. Only updates fields that are
 * present in JSON - missing fields are left unchanged.
 *
 * Supported fields:
 * - names (aliases, nominative, genitive, dative, accusative, instrumental, prepositional)
 * - descriptions (short_desc, long_desc)
 * - stats (level, armor, hitroll_penalty, sex, race, alignment, hp, damage)
 * - abilities (strength, dexterity, constitution, intelligence, wisdom, charisma)
 * - physical (height, weight, size, extra_attack, like_work, maxfactor, remort)
 * - death_load (array of {obj_vnum, load_prob, load_type, spec_param})
 * - roles (array of role indices)
 * - resistances (fire, air, water, earth, vitality, mind, immunity)
 * - savings (will, stability, reflex)
 * - position (default_position, load_position)
 * - behavior (class, attack_type, gold, helpers)
 * - flags (mob_flags, npc_flags, affect_flags)
 * - triggers (array of trigger vnums)
 */
void ParseMobUpdate(CharData* mob, const json& data);

// ============================================================================
// Object Parsing (stub for now)
// ============================================================================

/**
 * \brief Parse JSON and update object fields
 * \param obj Object to update
 * \param data JSON object with object update data
 */
void ParseObjectUpdate(CObjectPrototype* obj, const json& data);

// ============================================================================
// Room Parsing (stub for now)
// ============================================================================

/**
 * \brief Parse JSON and update room fields
 * \param room Room to update
 * \param data JSON object with room update data
 */
void ParseRoomUpdate(RoomData* room, const json& data);

// ============================================================================
// Zone Parsing (stub for now)
// ============================================================================

/**
 * \brief Parse JSON and update zone fields
 * \param zone Zone to update
 * \param data JSON object with zone update data
 */
void ParseZoneUpdate(ZoneData* zone, const json& data);

// ============================================================================
// Trigger Parsing (stub for now)
// ============================================================================

/**
 * \brief Parse JSON and update trigger fields
 * \param trig Trigger to update
 * \param data JSON object with trigger update data
 */
void ParseTriggerUpdate(Trigger* trig, const json& data);

}  // namespace admin_api::parsers

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_PARSERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
