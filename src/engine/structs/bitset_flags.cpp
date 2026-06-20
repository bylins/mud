/**
 \brief Non-template serialization helpers for BitsetFlags.
 \details These operate on 30-bit "plane" values so the on-disk bytes stay byte-identical to FlagData.
   Keeping them out of the header avoids pulling the logger / sprintbitwd machinery into every
   translation unit that merely tests or sets a flag.
*/

#include "engine/structs/bitset_flags.h"

#include "utils/utils.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <utility>

namespace bitset_flags_detail {

void report_overflow(const std::type_info &tag, std::size_t index, std::size_t logical_size,
					 std::size_t capacity, bool dropped) {
	static std::set<std::pair<std::size_t, std::size_t>> seen;
	const auto key = std::make_pair(tag.hash_code(), index);
	if (!seen.insert(key).second) {
		return;  // already reported this (enum, index) pair -- do not spam the log on hot paths
	}
	if (dropped) {
		log("SYSERR: BitsetFlags<%s>: flag index %zu exceeds capacity %zu (size %zu) -- dropped",
			tag.name(), index, capacity, logical_size);
	} else {
		log("SYSERR: BitsetFlags<%s>: flag index %zu is outside declared size %zu (capacity %zu) -- "
			"an enumerator may have been added without updating kCount",
			tag.name(), index, logical_size, capacity);
	}
}

std::vector<std::uint32_t> parse_string(const char *flag) {
	// 1) Numeric form: two or more space-separated non-negative integers, each one 30-bit plane.
	//    A single all-digit token is deliberately NOT this form -- it is the legacy packed number
	//    handled in step 3 (where the plane is encoded in the top 2 bits).
	{
		std::vector<std::uint32_t> values;
		const char *p = flag;
		bool valid = true;
		while (true) {
			while (*p == ' ') {
				++p;
			}
			if (*p == '\0') {
				break;
			}
			if (!isdigit(static_cast<unsigned char>(*p))) {
				valid = false;
				break;
			}
			char *end = nullptr;
			const unsigned long v = strtoul(p, &end, 10);
			if (end == p) {
				valid = false;
				break;
			}
			values.push_back(static_cast<std::uint32_t>(v));
			p = end;
		}
		if (valid && values.size() >= 2) {
			return values;
		}
	}

	// 2) Legacy ASCII "letter+plane-digit" form; also track whether the whole string is digits.
	std::vector<std::uint32_t> planes;
	const auto ensure_plane = [&planes](std::size_t plane) {
		if (plane >= planes.size()) {
			planes.resize(plane + 1, 0u);
		}
	};
	bool is_number = true;
	int i = 0;
	for (const char *p = flag; *p; p += i + 1) {
		i = 0;
		if (islower(static_cast<unsigned char>(*p))) {
			std::size_t plane = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				plane = static_cast<std::size_t>(*(p + 1) - '0');
				i = 1;
			}
			ensure_plane(plane);
			planes[plane] |= (0x3FFFFFFFu & (1u << (*p - 'a')));
		} else if (isupper(static_cast<unsigned char>(*p))) {
			std::size_t plane = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				plane = static_cast<std::size_t>(*(p + 1) - '0');
				i = 1;
			}
			ensure_plane(plane);
			planes[plane] |= (0x3FFFFFFFu & (1u << (26 + (*p - 'A'))));
		}
		if (!isdigit(static_cast<unsigned char>(*p))) {
			is_number = false;
		}
	}

	// 3) A single packed decimal number: plane encoded in the top 2 bits, value in the low 30.
	if (is_number && *flag) {
		const unsigned long packed = strtoul(flag, nullptr, 10);
		const std::size_t plane = packed >> 30;
		ensure_plane(plane);
		planes[plane] = static_cast<std::uint32_t>(packed & 0x3FFFFFFFu);
	}

	return planes;
}

std::string to_numeric_string(const std::vector<std::uint32_t> &planes) {
	std::string result;
	result.reserve(planes.size() * 11);
	for (std::size_t i = 0; i < planes.size(); ++i) {
		if (i > 0) {
			result += ' ';
		}
		result += std::to_string(planes[i]);
	}
	return result;
}

void tascii(const std::vector<std::uint32_t> &planes, int num_planes, char *ascii, std::size_t ascii_size) {
	bool found = false;
	for (int i = 0; i < num_planes; ++i) {
		const std::uint32_t plane = (static_cast<std::size_t>(i) < planes.size()) ? planes[i] : 0u;
		for (int c = 0; c < 30; ++c) {
			if (plane & (1u << c)) {
				found = true;
				const std::size_t len = strlen(ascii);
				snprintf(ascii + len, ascii_size - len, "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}
	strncat(ascii, found ? " " : "0 ", ascii_size - strlen(ascii) - 1);
}

bool sprintbits(const std::vector<std::uint32_t> &planes, const char *names[], char *result,
				std::size_t result_size, const char *div, int print_flag) {
	bool have_flags = false;
	char buffer[kMaxStringLength];
	*result = '\0';

	for (int i = 0; i < 4; ++i) {
		const std::uint32_t plane = (static_cast<std::size_t>(i) < planes.size()) ? planes[i] : 0u;
		if (sprintbitwd(plane | (static_cast<std::uint32_t>(i) << 30), names, buffer, sizeof(buffer), div,
						print_flag)) {
			if ('\0' != *result) {
				strncat(result, div, result_size - strlen(result) - 1);
			}
			strncat(result, buffer, result_size - strlen(result) - 1);
			have_flags = true;
		}
	}

	if ('\0' == *result) {
		strncat(result, CommonMsg(ECommonMsg::kNothing).c_str(), result_size - strlen(result) - 1);
	}

	return have_flags;
}

}  // namespace bitset_flags_detail

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
