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
