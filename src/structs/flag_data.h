/**
** \brief Unpacks flags from string #flag into flags array #to
** \details Заголовок/интерфейс класса, битовых флагов.
**
** \param [in] flag String that represents flags values.
** \param [out] to  Pointer to the array of integers that will be populated by unpacked flags values.
**
** \note Be careful: this function does not perform any checks of bounds of #to array.
*/

#ifndef BYLINS_SRC_STRUCTS_FLAG_DATA_H_
#define BYLINS_SRC_STRUCTS_FLAG_DATA_H_

#include "structs.h"

#include <array>
#include <cstdint>

void asciiflag_conv(const char *flag, void *to);

int ext_search_block(const char *arg, const char *const *list, int exact);

class FLAG_DATA {
 public:
	static constexpr size_t PLANES_NUMBER = 4;
	using flags_t = std::array<uint32_t, PLANES_NUMBER>;
	static constexpr size_t PLANE_SIZE = 8 * sizeof(flags_t::value_type) - 2;    // 2 bits spent for plane number

	FLAG_DATA() { clear(); }
	FLAG_DATA &operator+=(const FLAG_DATA &r);
	bool operator!=(const FLAG_DATA &r) const {
		return m_flags[0] != r.m_flags[0] || m_flags[1] != r.m_flags[1] || m_flags[2] != r.m_flags[2]
			|| m_flags[3] != r.m_flags[3];
	}
	bool operator==(const FLAG_DATA &r) const { return !(*this != r); }

	bool empty() const { return 0 == m_flags[0] && 0 == m_flags[1] && 0 == m_flags[2] && 0 == m_flags[3]; }

	void clear() { m_flags[0] = m_flags[1] = m_flags[2] = m_flags[3] = 0; }
	void set_all() { m_flags[0] = m_flags[1] = m_flags[2] = m_flags[3] = 0x3fffffff; }

	template<class T>
	bool get(const T packed_flag) const {
		return 0 != (m_flags[to_underlying(packed_flag) >> 30] & (to_underlying(packed_flag) & 0x3fffffff));
	}
	bool get_flag(const size_t plane, const uint32_t flag) const { return 0 != (m_flags[plane] & (flag & 0x3fffffff)); }
	uint32_t get_plane(const size_t number) const { return m_flags[number]; }
	bool plane_not_empty(const int packet_flag) const { return 0 != m_flags[packet_flag >> 30]; }

	template<class T>
	void set(const T packed_flag) {
		m_flags[to_underlying(packed_flag) >> 30] |= to_underlying(packed_flag) & 0x3fffffff;
	}
	void set_flag(const size_t plane, const uint32_t flag) { m_flags[plane] |= flag; }
	void set_plane(const size_t number, const uint32_t value) { m_flags[number] = value; }

	template<class T>
	void unset(const T packed_flag) {
		m_flags[to_underlying(packed_flag) >> 30] &= ~(to_underlying(packed_flag) & 0x3fffffff);
	}

	template<class T>
	bool toggle(const T packed_flag) {
		return 0 != ((m_flags[to_underlying(packed_flag) >> 30] ^= (to_underlying(packed_flag) & 0x3fffffff))
			& (to_underlying(packed_flag) & 0x3fffffff));
	}
	bool toggle_flag(const size_t plane, const uint32_t flag) { return 0 != ((m_flags[plane] ^= flag) & flag); }

	void from_string(const char *flag);
	void tascii(int num_planes, char *ascii) const;
	bool sprintbits(const char *names[], char *result, const char *div, const int print_flag) const;
	bool sprintbits(const char *names[], char *result, const char *div) const {
		return sprintbits(names,
						  result,
						  div,
						  0);
	};

	/// Изменение указанного флага
	void gm_flag(const char *subfield, const char *const *const list, char *res);

 protected:
	std::array<uint32_t, PLANES_NUMBER> m_flags;
};

template<>
inline bool FLAG_DATA::get(const uint32_t packed_flag) const {
	return 0 != (m_flags[packed_flag >> 30] & (packed_flag & 0x3fffffff));
}
template<>
inline bool FLAG_DATA::get(const int packed_flag) const { return get(static_cast<uint32_t>(packed_flag)); }
template<>
inline void FLAG_DATA::set(const uint32_t packed_flag) { m_flags[packed_flag >> 30] |= packed_flag & 0x3fffffff; }
template<>
inline void FLAG_DATA::set(const int packed_flag) { set(static_cast<uint32_t>(packed_flag)); }
template<>
inline void FLAG_DATA::unset(const uint32_t packed_flag) { m_flags[packed_flag >> 30] &= ~(packed_flag & 0x3fffffff); }
template<>
inline void FLAG_DATA::unset(const int packed_flag) { unset(static_cast<uint32_t>(packed_flag)); }
template<>
inline bool FLAG_DATA::toggle(const uint32_t packed_flag) {
	return 0 != ((m_flags[packed_flag >> 30] ^= (packed_flag & 0x3fffffff)) & (packed_flag & 0x3fffffff));
}
template<>
inline bool FLAG_DATA::toggle(const int packed_flag) { return toggle(static_cast<uint32_t>(packed_flag)); }

inline FLAG_DATA &FLAG_DATA::operator+=(const FLAG_DATA &r) {
	m_flags[0] |= r.m_flags[0];
	m_flags[1] |= r.m_flags[1];
	m_flags[2] |= r.m_flags[2];
	m_flags[3] |= r.m_flags[3];

	return *this;
}

extern const FLAG_DATA clear_flags;

class unique_bit_flag_data : public FLAG_DATA {
 public:
	bool operator==(const unique_bit_flag_data &r) const;
	bool operator!=(const unique_bit_flag_data &r) const { return !(*this == r); }
	bool operator<(const unique_bit_flag_data &r) const;
	bool operator>(const unique_bit_flag_data &r) const;

	unique_bit_flag_data() : FLAG_DATA(clear_flags) {}
	unique_bit_flag_data(const FLAG_DATA &__base) : FLAG_DATA(__base) {}
};

inline bool unique_bit_flag_data::operator==(const unique_bit_flag_data &r) const {
	return 0 != (m_flags[0] & r.m_flags[0])
		|| 0 != (m_flags[1] & r.m_flags[1])
		|| 0 != (m_flags[2] & r.m_flags[2])
		|| 0 != (m_flags[3] & r.m_flags[3]);
}

inline bool unique_bit_flag_data::operator<(const unique_bit_flag_data &r) const {
	return *this != r
		&& (m_flags[0] < r.m_flags[0]
			|| m_flags[1] < r.m_flags[1]
			|| m_flags[2] < r.m_flags[2]
			|| m_flags[3] < r.m_flags[3]);
}

inline bool unique_bit_flag_data::operator>(const unique_bit_flag_data &r) const {
	return *this != r
		&& (m_flags[0] > r.m_flags[0]
			|| m_flags[1] > r.m_flags[1]
			|| m_flags[2] > r.m_flags[2]
			|| m_flags[3] > r.m_flags[3]);
}

void tascii(const uint32_t *pointer, int num_planes, char *ascii);

inline int flag_data_by_num(const int &num) {
	return num < 0 ? 0 :
		   num < 30 ? (1 << num) :
		   num < 60 ? (kIntOne | (1 << (num - 30))) :
		   num < 90 ? (kIntTwo | (1 << (num - 60))) :
		   num < 120 ? (kIntThree | (1 << (num - 90))) : 0;
}

#endif //BYLINS_SRC_STRUCTS_FLAG_DATA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :