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
		for (i = j = 0, o = 1; j != 1 && **(list + i); i++)    // shapirus: попытка в лоб убрать креш
		{
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
		for (i = j = 0, o = 1; j != 1 && **(list + i); i++)    // shapirus: попытка в лоб убрать креш
		{
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
			} else if (!strn_cmp(arg, *(list + i), l)) {
				return j | o;
			} else {
				o <<= 1;
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

void FLAG_DATA::from_string(const char *flag) {
	uint32_t is_number = 1;
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

void FLAG_DATA::tascii(int num_planes, char *ascii) const {
	bool found = false;

	for (int i = 0; i < num_planes; i++) {
		for (int c = 0; c < 30; c++) {
			if (m_flags[i] & (1 << c)) {
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}


bool FLAG_DATA::sprintbits(const char *names[], char *result, const char *div, const int print_flag) const {
	bool have_flags = false;
	char buffer[kMaxStringLength];
	*result = '\0';

	for (int i = 0; i < 4; i++) {
		if (sprintbitwd(m_flags[i] | (i << 30), names, buffer, div, print_flag)) {
			if ('\0'
				!= *result)    // We don't need to calculate length of result. We just want to know if it is not empty.
			{
				strcat(result, div);
			}
			strcat(result, buffer);
			have_flags = true;
		}
	}

	if ('\0' == *result) {
		strcat(result, nothing_string);
	}

	return have_flags;
}

void FLAG_DATA::gm_flag(const char *subfield, const char *const *const list, char *res) {
	strcpy(res, "0");

	if ('\0' == *subfield) {
		return;
	}

	if (*subfield == '-') {
		const int flag = ext_search_block(subfield + 1, list, false);
		if (flag) {
			unset(flag);
			if (plane_not_empty(flag))    // looks like an error: we should check flag, but not whole plane
			{
				strcpy(res, "");
			}
		}
	} else if (*subfield == '+') {
		const int flag = ext_search_block(subfield + 1, list, false);
		if (flag) {
			set(flag);
			if (plane_not_empty(flag))    // looks like an error: we should check flag, but not whole plane
			{
				strcpy(res, "");
			}
		}
	} else {
		const int flag = ext_search_block(subfield, list, false);
		if (flag && get(flag)) {
			strcpy(res, "1");
		} else
			strcpy(res, "0");

	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :