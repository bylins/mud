/**************************************************************************
*   File: utils.cpp                                     Part of Bylins    *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "utils.h"
#include "utils/grammar/declensions.h"

#include <algorithm>
#include <iostream>
#include <fmt/format.h>

#include "engine/core/sysdep.h"
#include "engine/core/conf.h"
#include "backtrace.h"


#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <sstream>

extern std::pair<int, int> TotalMemUse();


const char *ACTNULL = "<NULL>";

int MIN(int a, int b) {
	return (a < b ? a : b);
}

int MAX(int a, int b) {
	return (a > b ? a : b);
}

// Create and append to dynamic length string - Alez
char *str_add(char *dst, const char *src) {
	if (dst == nullptr) {
		dst = (char *) malloc(strlen(src) + 1);
		strcpy(dst, src);
	} else {
		dst = (char *) realloc(dst, strlen(dst) + strlen(src) + 1);
		strcat(dst, src);
	};
	return dst;
}

// Create a duplicate of a string
char *str_dup(const char *source) {
	char *new_z = nullptr;
	if (source) {
		CREATE(new_z, strlen(source) + 1);
		return (strcpy(new_z, source));
	}
	CREATE(new_z, 1);
	return (strcpy(new_z, ""));
}

/*char* strdup(const char *s) {
	size_t slen = strlen(s);
	char* result = malloc(slen + 1);
	if(result == nullptr) {
		return nullptr;
	}
	memcpy(result, s, slen+1);
return result;
}
*/


bool is_head(std::string name) {
	if ((name == "Стрибог") || (name == "стрибог"))
		return true;
	return false;
}

// str_cmp, strn_cmp moved to utils_string.cpp

// the "touch" command, essentially.
int touch(const char *path) {
	FILE *fl;

	if (!(fl = fopen(path, "a"))) {
		log("SYSERR: %s: %s", path, strerror(errno));
		return (-1);
	} else {
		fclose(fl);
		return (0);
	}
}

void sprinttype(int type, const char *names[], char *result) {
	int nr = 0;

	while (type && *names[nr] != '\n') {
		type--;
		nr++;
	}

	if (*names[nr] != '\n')
		strcpy(result, names[nr]);
	else
		strcpy(result, "UNDEF");
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE *fl, char *buf) {
	char temp[256];
	auto lines = 0;

	do {
	if (fgets(temp, 256, fl)) {}
		if (feof(fl)) {
			return 0;
		}
		lines++;
	} while (*temp == '*' || *temp == '\n' || *temp == '\r');
	temp[strlen(temp) - 1] = '\0';
	strcpy(buf, temp);
	return lines;
}

// string manipulation fucntion originally by Darren Wilson //
// (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) //
// completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), //
// completely rewritten by Anton Gorev 05/08/2016 (kvirund@gmail.com) //
// substitute appearances of 'pattern' with 'replacement' in string //
// and return the # of replacements //
int replace_str(const utils::AbstractStringWriter::shared_ptr &writer, const char *pattern,
				const char *replacement, int rep_all, int max_size) {
	char *replace_buffer = nullptr;
	CREATE(replace_buffer, max_size);
	std::shared_ptr<char> guard(replace_buffer, free);
	size_t source_remained = writer->length();
	const size_t pattern_length = strlen(pattern);
	const size_t replacement_length = strlen(replacement);

	int count = 0;
	const char *from = writer->get_string();
	size_t remains = max_size;
	do {
		if (remains < source_remained) {
			return -1;    // destination does not have enough space.
		}

		const char *pos = strstr(from, pattern);
		if (nullptr != pos) {
			if (remains < source_remained - pattern_length + replacement_length) {
				return -1;    // destination does not have enough space.
			}

			strncpy(replace_buffer, from, pos - from);
			replace_buffer += pos - from;

			memcpy(replace_buffer, replacement, replacement_length);
			replace_buffer += replacement_length;
			remains -= replacement_length;

			const size_t processed = pos - from + pattern_length;
			source_remained -= processed;
			from += processed;

			++count;
		} else {
			strncpy(replace_buffer, from, source_remained);
			replace_buffer += source_remained;
			source_remained = 0;
		}
	} while (0 != rep_all && 0 < source_remained);

	if (count == 0) {
		return 0;
	}

	if (0 < source_remained) {
		strncpy(replace_buffer, from, std::min(remains, source_remained));
		replace_buffer += std::min(remains, source_remained);
	}
	*replace_buffer = '\0';
	writer->set_string(guard.get());

	return count;
}

// re-formats message type formatted char * //
// (for strings edited with d->str) (mostly olc and mail)     //
void format_text(const utils::AbstractStringWriter::shared_ptr &writer,
				 int mode, DescriptorData * /*d*/, size_t maxlen) {
	size_t total_chars = 0;
	int cap_next = true, cap_next_next = false;
	const char *flow;
	const char *start = nullptr;
	// warning: do not edit messages with max_str's of over this value //
	char formatted[kMaxStringLength];
	char *pos = formatted;

	flow = writer->get_string();
	if (!flow) {
		return;
	}

	if (IS_SET(mode, kFormatIndent)) {
		strcpy(pos, "   ");
		total_chars = 3;
		pos += 3;
	} else {
		*pos = '\0';
		total_chars = 0;
	}

	while (*flow != '\0') {
		while ((*flow == '\n')
			|| (*flow == '\r')
			|| (*flow == '\f')
			|| (*flow == '\t')
			|| (*flow == '\v')
			|| (*flow == ' ')) {
			flow++;
		}

		if (*flow != '\0') {
			start = flow++;
			while ((*flow != '\0')
				&& (*flow != '\n')
				&& (*flow != '\r')
				&& (*flow != '\f')
				&& (*flow != '\t')
				&& (*flow != '\v')
				&& (*flow != ' ')
				&& (*flow != '.')
				&& (*flow != '?')
				&& (*flow != '!')) {
				flow++;
			}

			if (cap_next_next) {
				cap_next_next = false;
				cap_next = true;
			}

			// this is so that if we stopped on a sentance .. we move off the sentance delim. //
			while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
				cap_next_next = true;
				flow++;
			}

			if ((total_chars + (flow - start) + 1) > 79) {
				strcpy(pos, "\r\n");
				total_chars = 0;
				pos += 2;
			}

			if (!cap_next) {
				if (total_chars > 0) {
					strcpy(pos, " ");
					++total_chars;
					++pos;
				}
			}

			total_chars += flow - start;
			strncpy(pos, start, flow - start);
			if (cap_next) {
				cap_next = false;
				*pos = UPPER(*pos);
			}
			pos += flow - start;
		}

		if (cap_next_next) {
			if ((total_chars + 3) > 79) {
				strcpy(pos, "\r\n");
				total_chars = 0;
				pos += 2;
			} else {
				strcpy(pos, " ");
				total_chars += 2;
				pos += 1;
			}
		}
	}
	strcpy(pos, "\r\n");

	if (static_cast<size_t>(pos - formatted) > maxlen) {
		formatted[maxlen] = '\0';
	}
	writer->set_string(formatted);
}


char *rustime(const struct tm *timeptr) {
	static char mon_name[12][10] =
		{
			"Января\0", "Февраля\0", "Марта\0", "Апреля\0", "Мая\0", "Июня\0",
			"Июля\0", "Августа\0", "Сентября\0", "Октября\0", "Ноября\0", "Декабря\0"
		};
	static char result[100];

	if (timeptr) {
		snprintf(result, sizeof(result), "%.2d:%.2d:%.2d %2d %s %d года",
				timeptr->tm_hour,
				timeptr->tm_min, timeptr->tm_sec, timeptr->tm_mday, mon_name[timeptr->tm_mon], 1900 + timeptr->tm_year);
	} else {
		snprintf(result, sizeof(result), "Время последнего входа неизвестно");
	}

	return result;
}

int round_up(float fl) {
	if ((int) fl < fl)
		return (int) fl + 1;
	else
		return (int) fl;
}

// IsValidEmail moved to utils_string.cpp

/**
* Вывод времени (в минутах) в зависимости от их кол-ва с округлением вплоть до месяцев.
* Для таймеров славы по 'glory имя' и 'слава информация'.
* \param flag - по дефолту 0 - 1 минута, 1 неделя;
*                          1 - 1 минуту, 1 неделю.
*/
std::string FormatTimeToStr(long in_timer, bool flag) {
	char buffer[256];
	std::ostringstream out;

	double timer = in_timer;
	int one = 0, two = 0;

	if (timer < 60)
		out << timer << " " << GetDeclensionInNumber(in_timer, flag ? EWhat::kMinU : EWhat::kMinA);
	else if (timer < 1440) {
		snprintf(buffer, sizeof(buffer), "%.1f", timer / 60);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kHour);
		else
			out << " " << GetDeclensionInNumber(one, EWhat::kHour);
	} else if (timer < 10080) {
		snprintf(buffer, sizeof(buffer), "%.1f", timer / 1440);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kDay);
		else
			out << " " << GetDeclensionInNumber(one, EWhat::kDay);
	} else if (timer < 44640) {
		snprintf(buffer, sizeof(buffer), "%.1f", timer / 10080);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kWeek);
		else
			out << " " << GetDeclensionInNumber(one, flag ? EWhat::kWeekU : EWhat::kWeek);
	} else {
		snprintf(buffer, sizeof(buffer), "%.1f", timer / 44640);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kMonth);
		else
			out << " " << GetDeclensionInNumber(one, EWhat::kMonth);
	}
	return out.str();
}

// skip_dots, str_str, kill_ems, cut_one_word, ReadEndString moved to utils_string.cpp
// StringReplace, format_news_message moved to utils_string.cpp

// strl_cpy, PrintNumberByDigits, thousands_sep moved to utils_string.cpp

void sanity_check() {
	int ok = true;

	// * If any line is false, 'ok' will become false also.
	ok &= (test_magic(buf) == kMagicNumber || test_magic(buf) == '\0');
	ok &= (test_magic(buf1) == kMagicNumber || test_magic(buf1) == '\0');
	ok &= (test_magic(buf2) == kMagicNumber || test_magic(buf2) == '\0');
	ok &= (test_magic(arg) == kMagicNumber || test_magic(arg) == '\0');

	/*
	* This isn't exactly the safest thing to do (referencing known bad memory)
	* but we're doomed to crash eventually, might as well try to get something
	* useful before we go down. -gg
	* However, lets fix the problem so we don't spam the logs. -gg 11/24/98
	*/
	if (!ok) {
		log("SYSERR: *** Buffer overflow! ***\n" "buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);

		plant_magic(buf);
		plant_magic(buf1);
		plant_magic(buf2);
		plant_magic(arg);
	}
}

/*short GetRealRemort(const std::shared_ptr<CharData> &ch) {
	return GetRealRemort(ch.get());
}*/

// isname, one_word moved to utils_string.cpp

const char a_ucc_table[256] = {
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d',
	'\x0e', '\x0f',    //15
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d',
	'\x1e', '\x1f',    //31
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d',
	'\x2e', '\x2f',    //47
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d',
	'\x3e', '\x3f',    //63
	'\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d',
	'\x4e', '\x4f',    //79
	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d',
	'\x5e', '\x5f',    //95
	'\x60', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d',
	'\x4e', '\x4f',    //111
	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x7b', '\x7c', '\x7d',
	'\x7e', '\x7f',    //127
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d',
	'\x8e', '\x8f',    //143
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d',
	'\x9e', '\x9f',    //159
	'\xa0', '\xa1', '\xa2', '\xb3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad',
	'\xae', '\xaf',    //175
	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd',
	'\xbe', '\xbf',    //191
	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed',
	'\xee', '\xef',    //207
	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd',
	'\xfe', '\xff',    //223
	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed',
	'\xee', '\xef',    //239
	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd',
	'\xfe', '\xff'    //255
};

const char a_lcc_table[256] = {
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d',
	'\x0e', '\x0f',
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d',
	'\x1e', '\x1f',
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d',
	'\x2e', '\x2f',
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d',
	'\x3e', '\x3f',
	'\x40', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d',
	'\x6e', '\x6f',
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7a', '\x5b', '\x5c', '\x5d',
	'\x5e', '\x5f',
	'\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d',
	'\x6e', '\x6f',
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d',
	'\x7e', '\x7f',
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d',
	'\x8e', '\x8f',
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d',
	'\x9e', '\x9f',
	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad',
	'\xae', '\xaf',
	'\xb0', '\xb1', '\xb2', '\xa3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd',
	'\xbe', '\xbf',
	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd',
	'\xce', '\xcf',
	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd',
	'\xde', '\xdf',
	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd',
	'\xce', '\xcf',
	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd',
	'\xde', '\xdf'
};

const bool a_isalnum_table[256] = {
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false,
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, true, false, false, false, false, false, false, false, false, false, false, false, false,
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true
};

const bool a_isdigit_table[256] = {
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
};

const bool a_isupper_table[256] = {
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //15
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //31
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //47
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //63
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //79
	true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false,    //95
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //111
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //127
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //159
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //175
	false, false, false, true, false, false, false, false, false, false, false, false, false, false, false,
	false,    //191
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //207
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //223
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //239
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true    //255
};

const bool a_isxdigit_table[256] = {
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //15
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //31
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //47
	true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,    //63
	false, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false,    //79
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //95
	false, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false,    //111
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //127
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //159
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //175
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //191
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //207
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //223
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //239
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false    //255
};

const bool a_isalpha_table[256] = {
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //15
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //31
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //47
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //63
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //79
	true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false,    //95
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //111
	true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false,    //127
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //159
	false, false, false, true, false, false, false, false, false, false, false, false, false, false, false,
	false,    //175
	false, false, false, true, false, false, false, false, false, false, false, false, false, false, false,
	false,    //191
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //207
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //223
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //239
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true    //255
};

const bool a_islower_table[256] = {
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //15
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //31
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //47
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //63
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //79
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //95
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //111
	true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false,    //127
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //159
	false, false, false, true, false, false, false, false, false, false, false, false, false, false, false,
	false,    //175
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //191
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //207
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,    //223
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //239
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false    //255
};

const bool a_isspace_table[256] = {
	true, false, false, false, false, false, false, false, false, true, true, true, true, true, false, false,    //15
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //31
	true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //47
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //63
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //79
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //95
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //111
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //127
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //159
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //175
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //191
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //207
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //223
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false,    //239
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
	false    //255
};

void build_char_table(int(*func)(int c)) {
	for (int c = 0; c < 256; c++) {
		std::cout << (func(c) ? " true" : "false") << (255 > c ? ", " : "");
		if (0 < c && 0 == (1 + c) % 16) {
			std::cout << "\r\n";
		}
	}
}

#ifdef WIN32
bool CCheckTable::test_values() const
{
	unsigned i = 0;
	do
	{
		if ((0 != m_original(i % 256)) != m_table(i % 256))
		{
			std::cout << static_cast<unsigned>(i % 256) << ": " << m_original(i % 256) << "; " << m_table(i % 256);
			return false;
		}
		++i;
	} while (0 < i);

	return true;
}

double CCheckTable::test_time() const
{
	class CMeasurer
	{
	public:
		CMeasurer(DWORD& duration) : m_start(GetTickCount()), m_output(duration) {}
		~CMeasurer() { m_output = GetTickCount() - m_start; }

	private:
		DWORD m_start;
		DWORD& m_output;
	};

	DWORD original_time = 0;
	{
		CMeasurer m(original_time);
		unsigned i = 0;
		do
		{
			(void)(0 != m_original(i % 256));
			++i;
		} while (0 < i);
	}

	DWORD table_time = 0;
	{
		CMeasurer m(table_time);
		unsigned i = 0;
		do
		{
			m_table(i % 256);
			++i;
		} while (0 < i);
	}

	return static_cast<long double>(original_time) / table_time;
}

void CCheckTable::check() const
{
	std::cout << "Validity... " << "\r\n";
	std::cout << (test_values() ? "passed" : "failed") << "\r\n";
	std::cout << "Performance... " << "\r\n";
	std::cout << std::setprecision(2) << std::fixed << test_time() * 100 << "%" << "\r\n";
}
#endif    // WIN32


bool sprintbitwd(Bitvector bitvector, const char *names[], char *result, size_t result_size, const char *div, const int print_flag) {

	long nr = 0;
	Bitvector fail;
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
				strncat(result, div, result_size - strlen(result) - 1);

			if (*names[nr] != '\n') {
				if (print_flag & 1) {
					size_t result_len = strlen(result);
					snprintf(result + result_len, result_size - result_len, "%c%d:", c, plane);
				}
				if ((print_flag & 2) && (!strcmp(names[nr], "UNUSED"))) {
					size_t result_len = strlen(result);
					snprintf(result + result_len, result_size - result_len, "%ld:", nr + 1);
				}
				if (can_show)
					strncat(result, (*names[nr] != '*' ? names[nr] : names[nr] + 1), result_size - strlen(result) - 1);
			} else {
				if (print_flag & 2) {
					size_t result_len = strlen(result);
					snprintf(result + result_len, result_size - result_len, "%ld:", nr + 1);
				} else if (print_flag & 1) {
					size_t result_len = strlen(result);
					snprintf(result + result_len, result_size - result_len, "%c%d:", c, plane);
				}
				strncat(result, "UNDEF", result_size - strlen(result) - 1);
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

	// issue.common-msg: sprintbitwd stays a pure formatter -- on an empty bitvector it leaves an empty
	// result and returns false. The "nothing" word (CommonMsg(kNothing)) is substituted by the wrappers
	// above it (FlagData::sprintbits, and the few direct callers that show the result to a player).
	if ('\0' == *result) {
		return false;
	}

	return true;
}

void MemLeakInfo() {
	static int last_pmem_used = 0;
	auto get_mem = TotalMemUse();
	int vmem_used = get_mem.first;
	int pmem_used = get_mem.second;
	char buf [256];

	if (pmem_used != last_pmem_used) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		last_pmem_used = pmem_used;
		snprintf(buf, sizeof(buf), "Memory size mismatch, last: phys (%d kb), current: virt (%d kB) phys (%d kB)", last_pmem_used, vmem_used, pmem_used);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		last_pmem_used = pmem_used;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
