/**
 \brief Typed, fixed-size bit-flag container meant to replace FlagData and raw Bitvector flags.
 \details
   BitsetFlags<EnumT, N> stores N logical flags as a flat std::bitset in memory, but is type-tagged
   by EnumT so a flag of one enum can never be written into a container of another enum.

   Key properties (see OUTPUT/issue.bitset-flags-plan.md):
     - In memory the layout is FLAT: the bit index of a flag is simply static_cast<size_t>(flag).
       Enumerators are therefore expected to be plain integers (kFoo = 7), not bitmasks (1u << 7).
     - On disk NOTHING changes. All existing serialization forms (legacy ASCII "letter+plane-digit",
       the numeric "n n n ..." form, and the name-based YAML/JSON forms) share one numbering:
       bit index i  <->  (plane = i / 30, bit = i % 30), 30 usable bits per plane. BitsetFlags keeps
       that 30-bit-plane regrouping strictly inside its (de)serializers, so the bytes it produces are
       identical to FlagData's.
     - Storage is an inline std::bitset sized to the rounded capacity (a multiple of 64). The extra
       bits in the last word form a small "reserve": writing/reading a flag whose index is past the
       logical size N but still inside the capacity still works (no data lost) and is logged once, as
       a safety net for "added an enumerator but forgot to bump the counter". An index past the
       capacity is refused and logged (never undefined behaviour, never the throwing std::bitset API).

   The flag count N defaults to flag_traits<EnumT>::count, which by default reads a terminal kCount
   enumerator on the enum. Adding a flag before kCount auto-bumps the count, so there is no separate
   constant to forget. (This indirection is also the future smart_enum seam.)
*/

#ifndef BYLINS_SRC_ENGINE_STRUCTS_BITSET_FLAGS_H_
#define BYLINS_SRC_ENGINE_STRUCTS_BITSET_FLAGS_H_

#include <bitset>
#include <cstdint>
#include <cstring>
#include <string>
#include <typeinfo>
#include <vector>

// Defined in flag_data.cpp; used by gm_flag() to resolve a flag name to its packed legacy value.
int ext_search_block(const char *arg, const char *const *list, int exact);

namespace bitset_flags_detail {

// usable bits per on-disk plane (the top 2 bits of a 32-bit word historically encoded the plane).
constexpr std::size_t kPlaneSize = 30;

// Decode a legacy "packed" flag value -- (plane << 30) | (1 << bit) -- into a flat bit index.
constexpr std::size_t packed_to_index(std::uint32_t packed) {
	const std::size_t plane = packed >> 30;
	const std::uint32_t mask = packed & 0x3FFFFFFFu;
	std::size_t bit = 0;
	if (mask) {
		while (bit < kPlaneSize && !((mask >> bit) & 1u)) {
			++bit;
		}
	}
	return plane * kPlaneSize + bit;
}

// Logs (once per enum+index) that a flag index fell outside the declared range. Defined in the .cpp
// so the hot path never pulls the logger into translation units that only test/set flags.
void report_overflow(const std::type_info &tag, std::size_t index, std::size_t logical_size,
					 std::size_t capacity, bool dropped);

// Parse a flag string into 30-bit plane values. Handles, exactly like FlagData::from_string:
//   - the numeric form "n n n ..." (two or more space-separated integers),
//   - the legacy ASCII "letter+plane-digit" form,
//   - a single packed decimal number (plane encoded in the top 2 bits).
std::vector<std::uint32_t> parse_string(const char *flag);

// Join plane values as the numeric "n n n ..." form.
std::string to_numeric_string(const std::vector<std::uint32_t> &planes);

// Emit the legacy ASCII "letter+plane-digit" form (byte-identical to FlagData::tascii).
void tascii(const std::vector<std::uint32_t> &planes, int num_planes, char *ascii, std::size_t ascii_size);

// Print flag names via the shared sprintbitwd() (byte-identical to FlagData::sprintbits).
bool sprintbits(const std::vector<std::uint32_t> &planes, const char *names[], char *result,
				std::size_t result_size, const char *div, int print_flag);

}  // namespace bitset_flags_detail

// Customization point for the flag count. By default reads a terminal kCount enumerator on the enum;
// specialize for enums that cannot host kCount. Later this becomes smart_enum<E>::size() with no
// change to BitsetFlags.
template<class EnumT>
struct flag_traits {
	static constexpr std::size_t count = static_cast<std::size_t>(EnumT::kCount);
};

template<class EnumT>
inline constexpr std::size_t flag_count_v = flag_traits<EnumT>::count;

// Maps a flag enumerator to its dense bit index. Default: the enum value IS the index (plain-integer
// enums). Specialize for enums still using the legacy packed-bitmask encoding -- a transitional hook
// so a packed enum can be stored in a BitsetFlags without renumbering it yet (the bit numbers, and
// therefore the on-disk bytes, stay identical to FlagData).
template<class EnumT>
struct flag_index_mapping {
	static constexpr std::size_t to_index(EnumT f) { return static_cast<std::size_t>(f); }
};

template<class EnumT, std::size_t N = flag_count_v<EnumT>>
class BitsetFlags {
 public:
	using enum_type = EnumT;

	/// Number of logical flags (the declared range).
	static constexpr std::size_t size() { return N; }
	/// Physical capacity in bits (N rounded up to a whole 64-bit word); >= size().
	static constexpr std::size_t capacity() { return kCapacity; }

	BitsetFlags() = default;
	BitsetFlags(std::initializer_list<EnumT> flags) {
		for (const auto f : flags) {
			set(f);
		}
	}
	explicit BitsetFlags(const char *legacy_or_numeric) { from_string(legacy_or_numeric); }

	// --- core, typed operations (no bitwise arithmetic at call sites) ---
	void set(EnumT f) { set_index(idx(f)); }
	void unset(EnumT f) { unset_index(idx(f)); }
	bool test(EnumT f) const { return test_index(idx(f)); }
	bool get(EnumT f) const { return test_index(idx(f)); }  // FlagData-compatible spelling
	bool toggle(EnumT f) {
		const std::size_t i = idx(f);
		if (!check_index(i)) {
			return false;
		}
		m_bits[i] = !m_bits[i];
		return m_bits[i];
	}

	void clear() { m_bits.reset(); }
	void set_all() {
		for (std::size_t i = 0; i < N; ++i) {
			m_bits[i] = true;
		}
	}

	bool any() const { return m_bits.any(); }
	bool none() const { return m_bits.none(); }
	bool empty() const { return m_bits.none(); }
	std::size_t count() const { return m_bits.count(); }

	// --- set algebra (named; replaces FlagData::operator+= and the intersect-== footgun) ---
	BitsetFlags &merge(const BitsetFlags &r) { m_bits |= r.m_bits; return *this; }
	BitsetFlags &mask(const BitsetFlags &r) { m_bits &= r.m_bits; return *this; }
	BitsetFlags &subtract(const BitsetFlags &r) { m_bits &= ~r.m_bits; return *this; }
	bool intersects(const BitsetFlags &r) const { return (m_bits & r.m_bits).any(); }

	BitsetFlags &operator|=(const BitsetFlags &r) { return merge(r); }
	BitsetFlags &operator+=(const BitsetFlags &r) { return merge(r); }  // deprecated alias, drop-in
	bool operator==(const BitsetFlags &r) const { return m_bits == r.m_bits; }
	bool operator!=(const BitsetFlags &r) const { return m_bits != r.m_bits; }

	// --- raw-index API for the name-based formats (dictionary / YAML / JSON / OLC / DG scripts) ---
	bool test_index(std::size_t i) const { return check_index(i) ? m_bits[i] : false; }
	void set_index(std::size_t i) { if (check_index(i)) { m_bits[i] = true; } }
	void unset_index(std::size_t i) { if (check_index(i)) { m_bits[i] = false; } }
	bool toggle_index(std::size_t i) {
		if (!check_index(i)) {
			return false;
		}
		m_bits[i] = !m_bits[i];
		return m_bits[i];
	}
	// Legacy 30-bit "plane" view: bits [plane*30, plane*30+30). For checksum / FlagData-compatible
	// serialization paths that still think in planes.
	std::uint32_t get_plane(std::size_t plane) const {
		std::uint32_t v = 0;
		for (std::size_t b = 0; b < bitset_flags_detail::kPlaneSize; ++b) {
			const std::size_t i = plane * bitset_flags_detail::kPlaneSize + b;
			if (i < kCapacity && m_bits[i]) {
				v |= (1u << b);
			}
		}
		return v;
	}
	template<class F>
	void for_each_set(F &&f) const {
		for (std::size_t i = 0; i < kCapacity; ++i) {
			if (m_bits[i]) {
				f(i);
			}
		}
	}

	// --- serialization (byte-compatible with FlagData) ---
	void from_string(const char *flag) {
		clear();
		const std::vector<std::uint32_t> planes = bitset_flags_detail::parse_string(flag);
		for (std::size_t p = 0; p < planes.size(); ++p) {
			const std::uint32_t v = planes[p] & 0x3FFFFFFFu;
			for (std::size_t b = 0; b < bitset_flags_detail::kPlaneSize; ++b) {
				if (v & (1u << b)) {
					set_index(p * bitset_flags_detail::kPlaneSize + b);
				}
			}
		}
	}

	std::string to_numeric_string() const {
		return bitset_flags_detail::to_numeric_string(extract_planes(serialized_planes()));
	}

	void tascii(int num_planes, char *ascii, std::size_t ascii_size) const {
		bitset_flags_detail::tascii(extract_planes(static_cast<std::size_t>(num_planes)),
									num_planes, ascii, ascii_size);
	}

	bool sprintbits(const char *names[], char *result, std::size_t result_size, const char *div,
					int print_flag = 0) const {
		return bitset_flags_detail::sprintbits(extract_planes(kLegacyPlanes), names, result,
											   result_size, div, print_flag);
	}

	// DG-script style getter/setter by flag name (mirrors FlagData::gm_flag).
	int gm_flag(const char *subfield, const char *const *list, char *res) {
		if ('\0' == *subfield) {
			return 0;
		}
		const bool remove = (*subfield == '-');
		const bool add = (*subfield == '+');
		const char *name = (remove || add) ? subfield + 1 : subfield;
		const int flag = ext_search_block(name, list, false);
		if (!flag) {
			return 0;
		}
		const std::size_t i = bitset_flags_detail::packed_to_index(static_cast<std::uint32_t>(flag));
		if (remove) {
			unset_index(i);
			strcpy(res, "");
		} else if (add) {
			set_index(i);
			strcpy(res, "");
		} else {
			strcpy(res, test_index(i) ? "1" : "0");
		}
		return 1;
	}

 private:
	static constexpr std::size_t kWordBits = 64;
	static constexpr std::size_t kCapacity = (N + kWordBits - 1) / kWordBits * kWordBits;  // reserve slack
	// FlagData always serialized 4 planes; keep that for byte-identical output up to 120 flags,
	// and widen only for genuinely larger enums.
	static constexpr std::size_t kLegacyPlanes = 4;

	std::bitset<kCapacity> m_bits{};

	static constexpr std::size_t idx(EnumT f) { return flag_index_mapping<EnumT>::to_index(f); }

	// Number of 30-bit planes to write: enough to cover every set bit (so a reserve bit past N is
	// still persisted), but never fewer than the legacy 4 -- so output stays byte-identical to
	// FlagData for the normal case (all bits < 120).
	std::size_t serialized_planes() const {
		std::size_t highest = 0;
		bool any = false;
		for (std::size_t i = 0; i < kCapacity; ++i) {
			if (m_bits[i]) {
				highest = i;
				any = true;
			}
		}
		const std::size_t needed = any ? (highest / bitset_flags_detail::kPlaneSize + 1) : 0;
		return needed > kLegacyPlanes ? needed : kLegacyPlanes;
	}

	// Bounds policy (see class doc): in [0,N) ok; in [N,capacity) ok + log once; >= capacity refuse.
	bool check_index(std::size_t i) const {
		if (i < N) {
			return true;
		}
		if (i < kCapacity) {
			bitset_flags_detail::report_overflow(typeid(EnumT), i, N, kCapacity, false);
			return true;
		}
		bitset_flags_detail::report_overflow(typeid(EnumT), i, N, kCapacity, true);
		return false;
	}

	std::vector<std::uint32_t> extract_planes(std::size_t num_planes) const {
		std::vector<std::uint32_t> planes(num_planes, 0u);
		for (std::size_t p = 0; p < num_planes; ++p) {
			std::uint32_t v = 0;
			for (std::size_t b = 0; b < bitset_flags_detail::kPlaneSize; ++b) {
				const std::size_t i = p * bitset_flags_detail::kPlaneSize + b;
				if (i < kCapacity && m_bits[i]) {
					v |= (1u << b);
				}
			}
			planes[p] = v;
		}
		return planes;
	}
};

#endif  // BYLINS_SRC_ENGINE_STRUCTS_BITSET_FLAGS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
