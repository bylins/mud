#include "structs.h"

#include "utils/utils.h"

void tascii(const uint32_t *pointer, int num_planes, char *ascii) {
	bool found = false;

	for (int i = 0; i < num_planes; i++) {
		for (int c = 0; c < 30; c++) {
			if (pointer[i] & (1 << c)) {
				found = true;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}

	strcat(ascii, found ? " " : "0 ");
}

const char* nothing_string = "ничего";

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag) {

	long nr = 0;
	bitvector_t fail;
	int plane = 0;
	char c = 'a';

	*result = '\0';

	fail = bitvector >> 30;
	bitvector &= 0x3FFFFFFF;
	while (fail) {
		if (*names[nr] == '\n') {
			fail--;
			plane++;
		}
		nr++;
	}

	bool can_show;
	for (; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			can_show = ((*names[nr] != '*') || (print_flag & 4));

			if (*result != '\0' && can_show)
				strcat(result, div);

			if (*names[nr] != '\n') {
				if (print_flag & 1) {
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				if ((print_flag & 2) && (!strcmp(names[nr], "UNUSED"))) {
					sprintf(result + strlen(result), "%ld:", nr + 1);
				}
				if (can_show)
					strcat(result, (*names[nr] != '*' ? names[nr] : names[nr] + 1));
			} else {
				if (print_flag & 2) {
					sprintf(result + strlen(result), "%ld:", nr + 1);
				} else if (print_flag & 1) {
					sprintf(result + strlen(result), "%c%d:", c, plane);
				}
				strcat(result, "UNDEF");
			}
		}

		if (print_flag & 1) {
			c++;
			if (c > 'z') {
				c = 'A';
			}
		}
		if (*names[nr] != '\n') {
			nr++;
		}
	}

	if ('\0' == *result) {
		strcat(result, nothing_string);
		return false;
	}

	return true;
}


void DelegatedStringWriter::set_string(const char *string) {
	const size_t l = strlen(string);
	if (nullptr == m_delegated_string) {
		CREATE(m_delegated_string, l + 1);
	} else {
		RECREATE(m_delegated_string, l + 1);
	}
	strcpy(m_delegated_string, string);
}

void DelegatedStringWriter::append_string(const char *string) {
	const size_t l = length() + strlen(string);
	if (nullptr == m_delegated_string) {
		CREATE(m_delegated_string, l + 1);
		*m_delegated_string = '\0';
	} else {
		RECREATE(m_delegated_string, l + 1);
	}
	strcat(m_delegated_string, string);
}

void DelegatedStringWriter::clear() {
	if (m_delegated_string) {
		free(m_delegated_string);
	}
	m_delegated_string = nullptr;
}

punish_data::punish_data() : duration(0), reason(nullptr), level(0), godid(0) {
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
