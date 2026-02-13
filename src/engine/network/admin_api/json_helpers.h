/**
 \file json_helpers.h
 \brief JSON parsing and serialization utilities for Admin API
 \authors Bylins team
 \date 2026-02-13

 Provides reusable JSON helper functions to eliminate code duplication
 in Admin API CRUD operations. Handles type-safe field extraction,
 flag parsing/serialization, and error handling.
*/

#ifndef BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_JSON_HELPERS_H_
#define BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_JSON_HELPERS_H_

#include "../../../third_party_libs/nlohmann/json.hpp"
#include "../../../engine/structs/flag_data.h"
#include "../../../engine/structs/structs.h"

#include <optional>
#include <string>
#include <string_view>

namespace admin_api::json {

using json = nlohmann::json;

// ============================================================================
// Safe JSON Field Extraction
// ============================================================================

/**
 * \brief Safely extract a field from JSON with default value
 * \tparam T Type of the field to extract
 * \param j JSON object to extract from
 * \param key Field name to extract
 * \param default_val Default value if field is missing or wrong type
 * \return Field value or default
 *
 * Usage:
 *   int level = GetField(data, "level", 1);
 *   std::string name = GetField(data, "name", std::string(""));
 */
template<typename T>
T GetField(const json& j, const char* key, T default_val)
{
	if (!j.contains(key))
	{
		return default_val;
	}

	try
	{
		return j[key].get<T>();
	}
	catch (const json::type_error&)
	{
		return default_val;
	}
}

/**
 * \brief Check if JSON contains a field of specific type
 * \param j JSON object
 * \param key Field name
 * \return true if field exists and is an array
 */
inline bool HasArray(const json& j, const char* key)
{
	return j.contains(key) && j[key].is_array();
}

/**
 * \brief Check if JSON contains a field of specific type
 * \param j JSON object
 * \param key Field name
 * \return true if field exists and is an object
 */
inline bool HasObject(const json& j, const char* key)
{
	return j.contains(key) && j[key].is_object();
}

/**
 * \brief Check if JSON contains a field of specific type
 * \param j JSON object
 * \param key Field name
 * \return true if field exists and is a string
 */
inline bool HasString(const json& j, const char* key)
{
	return j.contains(key) && j[key].is_string();
}

/**
 * \brief Check if JSON contains a field of specific type
 * \param j JSON object
 * \param key Field name
 * \return true if field exists and is a number
 */
inline bool HasNumber(const json& j, const char* key)
{
	return j.contains(key) && j[key].is_number();
}

// ============================================================================
// Flag Parsing and Serialization
// ============================================================================

/**
 * \brief Parse flags from JSON array and apply to flagset
 * \tparam FlagType Enum type for flags (EMobFlag, ENpcFlag, etc.)
 * \param j JSON object containing the flags array
 * \param key Field name for flags array
 * \param flagset Flagset to apply flags to
 *
 * JSON format: "key": [flag1, flag2, ...]
 * Flags are encoded as: (plane << 30) | (1 << bit)
 *
 * Usage:
 *   ParseFlags<EMobFlag>(data, "mob_flags", mob->char_specials.saved.act);
 */
template<typename FlagType, typename FlagsetType>
void ParseFlags(const json& j, const char* key, FlagsetType& flagset)
{
	if (!HasArray(j, key))
	{
		return;
	}

	for (const auto& flag_val : j[key])
	{
		if (flag_val.is_number_integer())
		{
			flagset.set(static_cast<FlagType>(flag_val.get<int>()));
		}
	}
}

/**
 * \brief Serialize flagset to JSON array
 * \param flagset Flagset to serialize (FlagData with planes)
 * \return JSON array of flag values
 *
 * Output format: [flag1, flag2, ...]
 * Flags are encoded as: (plane << 30) | (1 << bit)
 *
 * Usage:
 *   json arr = SerializeFlags(mob.char_specials.saved.act);
 */
json SerializeFlags(const FlagData& flagset);

/**
 * \brief Serialize single Bitvector to JSON array
 * \param bits Bitvector to serialize
 * \param plane Plane number (0-3) for encoding
 * \return JSON array of flag values
 */
json SerializeBitvector(Bitvector bits, size_t plane);

// ============================================================================
// Nested Object Parsing
// ============================================================================

/**
 * \brief Extract nested JSON object with validation
 * \param j Parent JSON object
 * \param key Field name for nested object
 * \return Optional containing nested object if valid, nullopt otherwise
 *
 * Usage:
 *   if (auto stats = ParseNested(data, "stats")) {
 *       int str = GetField(*stats, "strength", 10);
 *   }
 */
inline std::optional<json> ParseNested(const json& j, const char* key)
{
	if (!HasObject(j, key))
	{
		return std::nullopt;
	}
	return j[key];
}

// ============================================================================
// String Conversion Helpers (KOI8-R Б├■ UTF-8)
// ============================================================================

/**
 * \brief Convert KOI8-R string to UTF-8 for JSON
 * \param koi8r KOI8-R encoded string
 * \return UTF-8 encoded string
 *
 * Used when serializing entity data to JSON (which requires UTF-8)
 */
std::string Koi8rToUtf8(const char* koi8r);
std::string Koi8rToUtf8(const std::string& koi8r);

/**
 * \brief Convert UTF-8 string to KOI8-R for game data
 * \param utf8 UTF-8 encoded string (from JSON)
 * \return KOI8-R encoded string
 *
 * Used when parsing JSON data to apply to game entities
 */
std::string Utf8ToKoi8r(const std::string& utf8);

}  // namespace admin_api::json

#endif  // BYLINS_SRC_ENGINE_NETWORK_ADMIN_API_JSON_HELPERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
