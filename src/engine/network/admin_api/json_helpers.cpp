/**
 \file json_helpers.cpp
 \brief JSON parsing and serialization utilities implementation
 \authors Bylins team
 \date 2026-02-13
*/

#include "json_helpers.h"
#include "../../../engine/structs/structs.h"
#include "../../../utils/utils.h"

namespace admin_api::json {

// ============================================================================
// Flag Serialization
// ============================================================================

json SerializeFlags(const FlagData& flagset)
{
	json flags_array = json::array();

	// Iterate through all 4 planes
	for (size_t plane = 0; plane < 4; ++plane)
	{
		Bitvector plane_bits = flagset.get_plane(plane);

		// Check each bit in the plane (30 bits per plane)
		for (size_t bit = 0; bit < 30; ++bit)
		{
			if (plane_bits & (1U << bit))
			{
				// Encode as: (plane << 30) | (1 << bit)
				unsigned int flag_value = (static_cast<unsigned int>(plane) << 30) | (1U << bit);
				flags_array.push_back(static_cast<int>(flag_value));
			}
		}
	}

	return flags_array;
}

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

std::string Koi8rToUtf8(const char* koi8r)
{
	if (!koi8r)
	{
		return "";
	}

	char utf8_buf[kMaxStringLength];
	koi_to_utf8(const_cast<char*>(koi8r), utf8_buf);
	return std::string(utf8_buf);
}

std::string Koi8rToUtf8(const std::string& koi8r)
{
	char utf8_buf[kMaxSockBuf * 6];
	char koi8r_buf[kMaxSockBuf * 6];

	// Initialize buffers to prevent returning garbage if conversion fails
	utf8_buf[0] = '\0';

	strncpy(koi8r_buf, koi8r.c_str(), sizeof(koi8r_buf) - 1);
	koi8r_buf[sizeof(koi8r_buf) - 1] = '\0';

	koi_to_utf8(koi8r_buf, utf8_buf);

	return std::string(utf8_buf);
}

std::string Utf8ToKoi8r(const std::string& utf8)
{
	char koi8r_buf[kMaxSockBuf * 6];
	char utf8_buf[kMaxSockBuf * 6];

	strncpy(utf8_buf, utf8.c_str(), sizeof(utf8_buf) - 1);
	utf8_buf[sizeof(utf8_buf) - 1] = '\0';

	utf8_to_koi(utf8_buf, koi8r_buf);

	return std::string(koi8r_buf);
}

}  // namespace admin_api::json

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
