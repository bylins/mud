/**
 \file json_helpers.cpp
 \brief JSON parsing and serialization utilities implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "json_helpers.h"
#include "utils/utils_encoding.h"
#include "../../../engine/structs/structs.h"
#include "../../../utils/utils.h"

namespace admin_api::json {

// ============================================================================
// Flag Serialization
// ============================================================================

json SerializeBitvector(Bitvector bits, size_t plane)
{
	json flags_array = json::array();

	for (size_t bit = 0; bit < 30; ++bit)
	{
		if (bits & (1U << bit))
		{
			unsigned int flag_value = (static_cast<unsigned int>(plane) << 30) | (1U << bit);
			flags_array.push_back(static_cast<int>(flag_value));
		}
	}

	return flags_array;
}

// ============================================================================
// String Conversion (KOI8-R <-> UTF-8)
// ============================================================================



std::string Utf8ToKoi8r(const std::string& utf8)
{
	char koi8r_buf[kMaxSockBuf * 6];
	char utf8_buf[kMaxSockBuf * 6];

	strncpy(utf8_buf, utf8.c_str(), sizeof(utf8_buf) - 1);
	utf8_buf[sizeof(utf8_buf) - 1] = '\0';

	codepages::utf8_to_koi(utf8_buf, koi8r_buf);

	return std::string(koi8r_buf);
}

}  // namespace admin_api::json

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
