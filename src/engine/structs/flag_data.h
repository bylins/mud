// issue.flags-migration: the FlagData class has been deleted -- all flag storage is BitsetFlags
// now (see bitset_flags.h). This header retains only the shared flag-serialization free functions,
// the 30-bit-plane constants, and the standalone obj-set class-activation map key.

#ifndef BYLINS_SRC_STRUCTS_FLAG_DATA_H_
#define BYLINS_SRC_STRUCTS_FLAG_DATA_H_

#include "structs.h"

#include <array>
#include <cstdint>
#include <string>

constexpr size_t kFlagPlanes = 4;
constexpr size_t kFlagPlaneSize = 30;   // usable bits per 32-bit plane (top 2 bits encode the plane)

// Unpacks flags from string #flag into flags array #to (no bounds checks on #to).
void asciiflag_conv(const char *flag, void *to);

int ext_search_block(const char *arg, const char *const *list, int exact);
void tascii(const Bitvector *pointer, int num_planes, char *ascii, size_t ascii_size);

inline int flag_data_by_num(const int &num) {
	return num < 0 ? 0 :
		   num < 30 ? (1 << num) :
		   num < 60 ? (kIntOne | (1 << (num - 30))) :
		   num < 90 ? (kIntTwo | (1 << (num - 60))) :
		   num < 120 ? (kIntThree | (1 << (num - 90))) : 0;
}

// Intersect-matching key for the obj-set class->activation map (formerly a FlagData subclass).
// operator== is INTERSECT (any common bit in any plane); operator< orders non-intersecting keys so
// that intersecting keys compare equivalent under std::map. .set() uses the legacy packed encoding
// (top 2 bits = plane). Behaviour is byte-for-byte the old unique_bit_flag_data.
struct unique_bit_flag_data {
	std::array<Bitvector, kFlagPlanes> m_flags{};

	void set(const Bitvector packed_flag) { m_flags[(packed_flag >> 30) & 3] |= packed_flag & 0x3fffffff; }
	template<class T> void set(const T packed_flag) { set(static_cast<Bitvector>(packed_flag)); }
	bool get_flag(const size_t plane, const Bitvector flag) const { return 0 != (m_flags[plane] & (flag & 0x3fffffff)); }

	bool operator==(const unique_bit_flag_data &r) const {
		return 0 != (m_flags[0] & r.m_flags[0])
			|| 0 != (m_flags[1] & r.m_flags[1])
			|| 0 != (m_flags[2] & r.m_flags[2])
			|| 0 != (m_flags[3] & r.m_flags[3]);
	}
	bool operator!=(const unique_bit_flag_data &r) const { return !(*this == r); }
	bool operator<(const unique_bit_flag_data &r) const {
		return *this != r
			&& (m_flags[0] < r.m_flags[0]
				|| m_flags[1] < r.m_flags[1]
				|| m_flags[2] < r.m_flags[2]
				|| m_flags[3] < r.m_flags[3]);
	}
	bool operator>(const unique_bit_flag_data &r) const {
		return *this != r
			&& (m_flags[0] > r.m_flags[0]
				|| m_flags[1] > r.m_flags[1]
				|| m_flags[2] > r.m_flags[2]
				|| m_flags[3] > r.m_flags[3]);
	}
};

#endif //BYLINS_SRC_STRUCTS_FLAG_DATA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
