/**
 \author Created by Sventovit, realization by Krodo.
 \date 12.01.2022.
 \brief Реализация FlagData.
 \details Реализация класса битиовых флагов.
*/

#include "flag_data.h"
#include "utils/utils.h"

int ext_search_block(const char *arg, const char *const *const list, int exact) {
	unsigned int i, j, o;

	if (exact) {
		for (i = j = 0, o = 1; j != 1 && **(list + i); i++) {
			if (**(list + i) == '\n') {
				o = 1;
				switch (j) {
					case 0: j = kIntOne;
						break;
					case kIntOne: j = kIntTwo;
						break;
					case kIntTwo: j = kIntThree;
						break;
					default: j = 1;
						break;
				}
			} else if (!str_cmp(arg, *(list + i))) {
				return j | o;
			} else {
				o <<= 1;
			}
		}
	} else {
		size_t l = strlen(arg);
		if (!l) {
			l = 1;    // Avoid "" to match the first available string
		}
		for (i = j = 0, o = 1; j != 1; i++) {
			if (**(list + i) == '\n') {
				o = 1;
				switch (j) {
					case 0: j = kIntOne;
						break;
					case kIntOne: j = kIntTwo;
						break;
					case kIntTwo: j = kIntThree;
						break;
					default: j = 1;
						break;
				}
			} else {
				if (!strn_cmp(arg, *(list + i), l)) {
					return j | o;
				} else {
					o <<= 1;
				}
			}
		}
	}

	return 0;
}

void asciiflag_conv(const char *flag, void *to) {
	int *flags = (int *) to;
	unsigned int is_number = 1, block = 0, i;
	const char *p;

	for (p = flag; *p; p += i + 1) {
		i = 0;
		if (islower(*p)) {
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				block = (int) *(p + 1) - '0';
				i = 1;
			} else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (*p - 'a')));
		} else if (isupper(*p)) {
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				block = (int) *(p + 1) - '0';
				i = 1;
			} else
				block = 0;
			*(flags + block) |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}
		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number) {
		is_number = atol(flag);
		block = is_number < kIntOne ? 0 : is_number < kIntTwo ? 1 : is_number < kIntThree ? 2 : 3;
		*(flags + block) = is_number & 0x3FFFFFFF;
	}
}

void FlagData::from_string(const char *flag) {
	// Detect new numeric format: exactly kPlanesNumber space-separated unsigned integers,
	// e.g. "0 12345 0 67890".  Written by to_numeric_string().
	{
		Bitvector values[kPlanesNumber] = {};
		size_t count = 0;
		const char *p = flag;
		bool valid = true;

		while (count < kPlanesNumber && valid) {
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
			char *end;
			unsigned long v = strtoul(p, &end, 10);
			if (end == p) {
				valid = false;
				break;
			}
			values[count++] = static_cast<Bitvector>(v);
			p = end;
		}
		while (*p == ' ') {
			++p;
		}

		if (valid && count == kPlanesNumber && *p == '\0') {
			for (size_t i = 0; i < kPlanesNumber; ++i) {
				m_flags[i] = values[i];
			}
			return;
		}
	}

	// Legacy ASCII format: letter+digit encoding or single packed number.
	Bitvector is_number = 1;
	int i;

	for (const char *p = flag; *p; p += i + 1) {
		i = 0;
		if (islower(*p)) {
			size_t block = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				block = (int) *(p + 1) - '0';
				i = 1;
			}
			m_flags[block] |= (0x3FFFFFFF & (1 << (*p - 'a')));
		} else if (isupper(*p)) {
			size_t block = 0;
			if (*(p + 1) >= '0' && *(p + 1) <= '9') {
				block = (int) *(p + 1) - '0';
				i = 1;
			}
			m_flags[block] |= (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
		}

		if (!isdigit(*p)) {
			is_number = 0;
		}
	}

	if (is_number) {
		is_number = atol(flag);
		const size_t block = is_number >> 30;
		m_flags[block] = is_number & 0x3FFFFFFF;
	}
}

std::string FlagData::to_numeric_string() const {
	std::string result;
	result.reserve(40);
	for (size_t i = 0; i < kPlanesNumber; ++i) {
		if (i > 0) {
			result += ' ';
		}
		result += std::to_string(m_flags[i]);
	}
	return result;
}

void FlagData::tascii(int num_planes, char *ascii, size_t ascii_size) const {
	bool found = false;

	for (int i = 0; i < num_planes; i++) {
		for (int c = 0; c < 30; c++) {
			if (m_flags[i] & (1 << c)) {
				found = true;
				size_t ascii_len = strlen(ascii);
				snprintf(ascii + ascii_len, ascii_size - ascii_len, "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strncat(ascii, found ? " " : "0 ", ascii_size - strlen(ascii) - 1);
}


bool FlagData::sprintbits(const char *names[], char *result, size_t result_size, const char *div, const int print_flag) const {
	bool have_flags = false;
	char buffer[kMaxStringLength];
	*result = '\0';

	for (int i = 0; i < 4; i++) {
		if (sprintbitwd(m_flags[i] | (i << 30), names, buffer, sizeof(buffer), div, print_flag)) {
			if ('\0' != *result) {
				strncat(result, div, result_size - strlen(result) - 1);
			}
			strncat(result, buffer, result_size - strlen(result) - 1);
			have_flags = true;
		}
	}

	if ('\0' == *result) {
		strncat(result, nothing_string, result_size - strlen(result) - 1);
	}

	return have_flags;
}

int FlagData::gm_flag(const char *subfield, const char *const *const list, char *res) {
//	strcpy(res, "0");

	if ('\0' == *subfield) {
		return 0;
	}

	if (*subfield == '-') {
		const int flag = ext_search_block(subfield + 1, list, false);
		if (flag) {
			unset(flag);
			if (plane_not_empty(flag))    // looks like an error: we should check flag, but not whole plane
			{
				strcpy(res, "");
			}
		} else 
			return 0;
	} else if (*subfield == '+') {
		const int flag = ext_search_block(subfield + 1, list, false);
		if (flag) {
			set(flag);
			if (plane_not_empty(flag))    // looks like an error: we should check flag, but not whole plane
			{
				strcpy(res, "");
			}
		} else 
			return 0;
	} else {
		const int flag = ext_search_block(subfield, list, false);
		if (flag) {
			if (get(flag)) {
				strcpy(res, "1");
			} else
				strcpy(res, "0");
		} else
			return 0;
	}
	return 1;
}

// заколебали эти флаги... сравниваем num и все поля в flags
bool CompareBits(const FlagData &flags, const char *names[], int affect) {
	int i;
	for (i = 0; i < 4; i++) {
		int nr = 0;
		int fail = i;
		Bitvector bitvector = flags.get_plane(i);

		while (fail) {
			if (*names[nr] == '\n')
				fail--;
			nr++;
		}

		for (; bitvector; bitvector >>= 1) {
			if (IS_SET(bitvector, 1))
				if (*names[nr] != '\n')
					if (nr == affect)
						return true;
			if (*names[nr] != '\n')
				nr++;
		}
	}
	return false;
}

void tascii(const Bitvector *pointer, int num_planes, char *ascii, size_t ascii_size) {
	bool found = false;

	for (int i = 0; i < num_planes; i++) {
		for (int c = 0; c < 30; c++) {
			if (pointer[i] & (1 << c)) {
				found = true;
				size_t ascii_len = strlen(ascii);
				snprintf(ascii + ascii_len, ascii_size - ascii_len, "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strncat(ascii, found ? " " : "0 ", ascii_size - strlen(ascii) - 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
