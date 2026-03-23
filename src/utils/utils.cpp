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

#include <algorithm>
#include <iostream>
#include "../subprojects/fmt/include/fmt/format.h"

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

char AltToKoi[] = {
		"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмноп░▒▓│┤╡+++╣║╗╝+╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨++╙╘╒++╪┘┌█▄▌▐▀рстуфхцчшщъыьэюяЁё╫╜╢╓╤╕╥╖·√??■ "
	};
char KoiToAlt[] = {
		"дЁз©юыц╢баеъэшщч╟╠╡+Ч+Ш+++Ъ+++З+м╨уЯУиВЫ╩тсх╬С╪фгл╣ПТ╧ЖЬкопйьРн+Н═║Ф╓╔ДёЕ╗╘╙╚╛╜╝╞ОЮАБЦ╕╒ЛК╖ХМИГЙ·─│√└┘■┐∙┬┴┼▀▄█▌▐÷░▒▓⌠├┌°⌡┤≤²≥≈ "
	};
char WinToKoi[] = {
		"++++++++++++++++++++++++++++++++ ++++╫++Ё©╢++++╥°+╤╕╜++·ё+╓++++╖АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя"
	};
char KoiToWin[] = {
		"++++++++++++++++++++++++++═+╟+╥++++╦╨+Ё©+++++╢+++++╗╙+╡╞+++++╔+╘ЧЮАЖДЕТЦУХИЙКЛМНОЪПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"
	};
char KoiToWin2[] = {
		"++++++++++++++++++++++++++═+╟+╥++++╦╨+Ё©+++++╢+++++╗╙+╡╞+++++╔+╘ЧЮАЖДЕТЦУХИЙКЛМНОzПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"
	};
char AltToLat[] = {
		"─│┌┐└┘├┤┬┴┼▀▄█▌▐░▒▓⌠■∙√≈≤≥ ⌡°²·÷═║╒ё╓╔╕╖╗╘╙╚╛╜╝╞╟╠╡Ё╢╣╤╥╦╧╨╩╪╫╬©0abcdefghijklmnopqrstY1v23z456780ABCDEFGHIJKLMNOPQRSTY1V23Z45678"
	};

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

void koi_to_win(char *str, int size) {
	for (; size > 0; *str = KtoW(*str), size--, str++);
}

void koi_to_alt(char *str, int size) {
	for (; size > 0; *str = KtoA(*str), size--, str++);
}

std::string koi_to_alt(const std::string &input) {
	std::string result = input;
	for (std::size_t i = 0; i < result.size(); ++i) {
		result[i] = KtoA(result[i]);
	}
	return result;
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

/*
\todo Переделать в нормальный вид с библиотекой склонений/спряжений или хотя бы полным списом падежей с индексацией через ECases.
 и перенести в engine/grammar
*/
using SomePads = std::array<std::string, 3>;
using SomePadsMap = std::unordered_map<EWhat, SomePads>;

const char *GetDeclensionInNumber(long amount, EWhat of_what) {
	static const SomePadsMap things_cases = {
		{EWhat::kDay, {"дней", "день", "дня"}},
		{EWhat::kHour, {"часов", "час", "часа"}},
		{EWhat::kYear, {"лет", "год", "года"}},
		{EWhat::kPoint, {"очков", "очко", "очка"}},
		{EWhat::kMinA, {"минут", "минута", "минуты"}},
		{EWhat::kMinU, {"минут", "минуту", "минуты"}},
		{EWhat::kMoneyA, {"кун", "куна", "куны"}},
		{EWhat::kMoneyU, {"кун", "куну", "куны"}},
		{EWhat::kThingA, {"штук", "штука", "штуки"}},
		{EWhat::kThingU, {"штук", "штуку", "штуки"}},
		{EWhat::kLvl, {"уровней", "уровень", "уровня"}},
		{EWhat::kMoveA, {"верст", "верста", "версты"}},
		{EWhat::kMoveU, {"верст", "версту", "версты"}},
		{EWhat::kOneA, {"единиц", "единица", "единицы"}},
		{EWhat::kOneU, {"единиц", "единицу", "единицы"}},
		{EWhat::kSec, {"секунд", "секунду", "секунды"}},
		{EWhat::kDegree, {"градусов", "градус", "градуса"}},
		{EWhat::kRow, {"строк", "строка", "строки"}},
		{EWhat::kObject, {"предметов", "предмет", "предмета"}},
		{EWhat::kObjU, {"предметов", "предмета", "предметов"}},
		{EWhat::kRemort, {"перевоплощений", "перевоплощение", "перевоплощения"}},
		{EWhat::kWeek, {"недель", "неделя", "недели"}},
		{EWhat::kMonth, {"месяцев", "месяц", "месяца"}},
		{EWhat::kWeekU, {"недель", "неделю", "недели"}},
		{EWhat::kGlory, {"славы", "слава", "славы"}},
		{EWhat::kGloryU, {"славы", "славу", "славы"}},
		{EWhat::kPeople, {"человек", "человек", "человека"}},
		{EWhat::kStr, {"силы", "сила", "силы"}},
		{EWhat::kGulp, {"глотков", "глоток", "глотка"}},
		{EWhat::kTorc, {"гривен", "гривна", "гривны"}},
		{EWhat::kGoldTorc, {"золотых", "золотая", "золотые"}},
		{EWhat::kSilverTorc, {"серебряных", "серебряная", "серебряные"}},
		{EWhat::kBronzeTorc, {"бронзовых", "бронзовая", "бронзовые"}},
		{EWhat::kTorcU, {"гривен", "гривну", "гривны"}},
		{EWhat::kGoldTorcU, {"золотых", "золотую", "золотые"}},
		{EWhat::kSilverTorcU, {"серебряных", "серебряную", "серебряные"}},
		{EWhat::kBronzeTorcU, {"бронзовых", "бронзовую", "бронзовые"}},
		{EWhat::kIceU, {"искристых снежинок", "искристую снежинку", "искристые снежинки"}},
		{EWhat::kNogataU, {"ногат", "ногату", "ногаты"}}
	};

	if (amount < 0) {
		amount = -amount;
	}

	if ((amount % 100 >= 11 && amount % 100 <= 14) || amount % 10 >= 5 || amount % 10 == 0) {
		return things_cases.at(of_what)[0].c_str();
	}

	if (amount % 10 == 1) {
		return things_cases.at(of_what)[1].c_str();
	} else {
		return things_cases.at(of_what)[2].c_str();
	}
}

char *rustime(const struct tm *timeptr) {
	static char mon_name[12][10] =
		{
			"Января\0", "Февраля\0", "Марта\0", "Апреля\0", "Мая\0", "Июня\0",
			"Июля\0", "Августа\0", "Сентября\0", "Октября\0", "Ноября\0", "Декабря\0"
		};
	static char result[100];

	if (timeptr) {
		sprintf(result, "%.2d:%.2d:%.2d %2d %s %d года",
				timeptr->tm_hour,
				timeptr->tm_min, timeptr->tm_sec, timeptr->tm_mday, mon_name[timeptr->tm_mon], 1900 + timeptr->tm_year);
	} else {
		sprintf(result, "Время последнего входа неизвестно");
	}

	return result;
}

int roundup(float fl) {
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
		sprintf(buffer, "%.1f", timer / 60);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kHour);
		else
			out << " " << GetDeclensionInNumber(one, EWhat::kHour);
	} else if (timer < 10080) {
		sprintf(buffer, "%.1f", timer / 1440);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kDay);
		else
			out << " " << GetDeclensionInNumber(one, EWhat::kDay);
	} else if (timer < 44640) {
		sprintf(buffer, "%.1f", timer / 10080);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two << " " << GetDeclensionInNumber(two, EWhat::kWeek);
		else
			out << " " << GetDeclensionInNumber(one, flag ? EWhat::kWeekU : EWhat::kWeek);
	} else {
		sprintf(buffer, "%.1f", timer / 44640);
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

#ifdef HAVE_ICONV
void koi_to_utf8(char *str_i, char *str_o)
{
	iconv_t cd;
	size_t len_i, len_o = kMaxSockBuf * 6;
	size_t i;

	if ((cd = iconv_open("UTF-8","KOI8-R")) == (iconv_t) - 1)
	{
		printf("koi_to_utf8: iconv_open error\n");
		return;
	}
	len_i = strlen(str_i);
	// const_cast at the next line is required for Linux, because there iconv has non-const input argument.
	if ((i = iconv(cd, &str_i, &len_i, &str_o, &len_o)) == (size_t) - 1)
	{
		printf("koi_to_utf8: iconv error\n");
		return;
	}
	*str_o = 0;
	if (iconv_close(cd) == -1)
	{
		printf("koi_to_utf8: iconv_close error\n");
		return;
	}
}

void utf8_to_koi(char *str_i, char *str_o)
{
	iconv_t cd;
	size_t len_i, len_o = kMaxSockBuf * 6;
	size_t i;

	if ((cd = iconv_open("KOI8-R", "UTF-8")) == (iconv_t) - 1)
	{
		perror("utf8_to_koi: iconv_open error");
		return;
	}
	len_i = strlen(str_i);
	if ((i=iconv(cd, &str_i, &len_i, &str_o, &len_o)) == (size_t) - 1)
	{
		perror("utf8_to_koi: iconv error");
	}
	if (iconv_close(cd) == -1)
	{
		perror("utf8_to_koi: iconv_close error");
		return;
	}
}

#else  // HAVE_ICONV

#define KOI8_UNKNOWN_CHAR  '+'  // char to use when cannot represent unicode codepoint in KOI8-R

// Simple implementation of UTF-8/KOI8-R converter, supports all codes available in KOI8-R
void utf8_to_koi(char *str_i, char *str_o) {
	unsigned char c;

	while ((c = static_cast<unsigned char>(*str_i++))) {
		if (c < 0x80) {
			*str_o++ = c;
		} else if (c < 0xC0) {
			// unexpected as a first byte in a sequence, skip
		} else if (c < 0xE0) {
			// one more byte to follow, must be b10xxxxxx
			unsigned char c1 = static_cast<unsigned char>(*str_i);
			if ((c1 & 0xC0) == 0x80) {
				// valid utf-8, but we are only interested in characters from
				// the first half of Unicode Cyrillic Block (0x400 to 0x47F)

				// init with unknown
				*str_o = KOI8_UNKNOWN_CHAR;

				if ((c & 0xFE) == 0xD0) // 0x0400 - 0x047F Cyrillic Unicode block first half
				{
					// get offset
					c1 -= (c & 1) ? 0x40 : 0x80;
					if (c1 >= 0x10 && c1 < 0x50) {
						const unsigned char Utf8ToKoiAlpha[] = {
							0xE1, 0xE2, 0xF7, 0xE7, 0xE4, 0xE5, 0xF6, 0xFA, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
							0xF0, // koi8-r АБВГДЕЖЗИЙКЛМНОП
							0xF2, 0xF3, 0xF4, 0xF5, 0xE6, 0xE8, 0xE3, 0xFE, 0xFB, 0xFD, 0xFF, 0xF9, 0xF8, 0xFC, 0xE0,
							0xF1,    // koi8-r РСТУФХЦЧШЩЪЫЬЭЮЯ
							0xC1, 0xC2, 0xD7, 0xC7, 0xC4, 0xC5, 0xD6, 0xDA, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
							0xD0, // koi8-r абвгдежзийклмноп
							0xD2, 0xD3, 0xD4, 0xD5, 0xC6, 0xC8, 0xC3, 0xDE, 0xDB, 0xDD, 0xDF, 0xD9, 0xD8, 0xDC, 0xC0,
							0xD1  // koi8-r рстуфхцчшщъыьэюя
						};
						*str_o = static_cast<char>(Utf8ToKoiAlpha[c1 - 0x10]);
					} else if (c1 == 0x01) {
						*str_o = '\xB3'; // koi8-r Ё
					} else if (c1 == 0x51) {
						*str_o = '\xA3'; // koi8-r ё
					}
				} else if (c == 0xC2) // 0x0080 - 0x00BF
				{
					// 0x00B0, 0x00B2, 0x00B7, 0x00F7
					if (c1 == 0xA0) {
						*str_o = '\x9A'; // NO-BREAK SPACE
					} else
					if (c1 == 0xA9) {
						*str_o = '\xBF';
					} else if (c1 == 0xB0) {
						*str_o = '\x9C';
					} else if (c1 == 0xB2) {
						*str_o = '\x9D';
					} else if (c1 == 0xB7) {
						*str_o = '\x9E';
					}
				} else if ((c == 0xC3) && (c1 == 0xB7)) // 0x00F7
				{
					*str_o = '\x9F';
				}
				str_o++;
				str_i++;
			}
		} else if (c < 0xF0) {
			// two more bytes to follow, must be b10xxxxxx

			if ((str_i[0] & 0xC0) == 0x80
				&& (str_i[1] & 0xC0) == 0x80) {
				// valid utf-8
				// calculate unicode codepoint
				unsigned short u = static_cast<unsigned short>(c) & 0x0F;
				u = (u << 6) | (str_i[0] & 0x3F);
				u = (u << 6) | (str_i[1] & 0x3F);
				*str_o = KOI8_UNKNOWN_CHAR;

				if (u >= 0x2550 && u <= 0x256C)// 0x2550-0x256C
				{
					const unsigned char Utf8ToKoiPg[] = {
						0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
						0xAF, // koi8-r ═║╒╓╔╕╖╗╘╙╚╛╜╝╞
						0xB0, 0xB1, 0xB2, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
						0xBE        // koi8-r ╟╠╡╢╣╤╥╦╧╨╩╪╫╬
					};
					*str_o = static_cast<char>(Utf8ToKoiPg[u - 0x2550]);
				} else // random non-sequencitial bits and pieces (other pseudographics and some math symbols)
				{
					switch (u) {
						case 0x2500: *str_o = '\x80';
							break;
						case 0x2502: *str_o = '\x81';
							break;
						case 0x250C: *str_o = '\x82';
							break;
						case 0x2510: *str_o = '\x83';
							break;
						case 0x2514: *str_o = '\x84';
							break;
						case 0x2518: *str_o = '\x85';
							break;
						case 0x251C: *str_o = '\x86';
							break;
						case 0x2524: *str_o = '\x87';
							break;
						case 0x252C: *str_o = '\x88';
							break;
						case 0x2534: *str_o = '\x89';
							break;
						case 0x253C: *str_o = '\x8A';
							break;
						case 0x2580: *str_o = '\x8B';
							break;
						case 0x2584: *str_o = '\x8C';
							break;
						case 0x2588: *str_o = '\x8D';
							break;
						case 0x258C: *str_o = '\x8E';
							break;
						case 0x2590: *str_o = '\x8F';
							break;
						case 0x2591: *str_o = '\x90';
							break;
						case 0x2592: *str_o = '\x91';
							break;
						case 0x2593: *str_o = '\x92';
							break;
						case 0x2320: *str_o = '\x93';
							break;
						case 0x25A0: *str_o = '\x94';
							break;
						case 0x2219: *str_o = '\x95';
							break;
						case 0x221A: *str_o = '\x96';
							break;
						case 0x2248: *str_o = '\x97';
							break;
						case 0x2264: *str_o = '\x98';
							break;
						case 0x2265: *str_o = '\x99';
							break;
							//   0x00A0        to 0x9A decoded elsewhere
						case 0x2321: *str_o = '\x9B';
							break;
					}
				}

				str_o++;
				str_i += 2;
			}
		} else if (c < 0xF8) {
			// three more bytes to follow, must be b10xxxxxx
			if ((str_i[0] & 0xC0) == 0x80
				&& (str_i[1] & 0xC0) == 0x80
				&& (str_i[2] & 0xC0) == 0x80) {
				// valid utf-8, but we don't handle those - not present in koi8-r
				*str_o++ = KOI8_UNKNOWN_CHAR;
				str_i += 3;
			}
		} else // c >= 0xF8
		{
			// not a valid utf-8 byte, ignore
		}
	}

	*str_o = 0;
}

void koi_to_utf8(char *str_i, char *str_o) {
	// KOI8-R to Cyrillic Unicode block 0x0400 - 0x047F
	const unsigned short KoiToUtf8[] =
		{
			0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584,
			0x2588, 0x258C, 0x2590,
			0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0,
			0x00B2, 0x00B7, 0x00F7,
			0x2550, 0x2501, 0x2502, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556, 0x2557, 0x2558, 0x2559, 0x255A, 0x255B,
			0x255C, 0x255D, 0x255E,
			0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565, 0x2566, 0x2567, 0x2568, 0x2569, 0x256A,
			0x256B, 0x256C, 0x00A9,
			0x44E, 0x430, 0x431, 0x446, 0x434, 0x435, 0x444, 0x433, 0x445, 0x438, 0x439, 0x43A, 0x43B, 0x43C, 0x43D,
			0x43E,
			0x43F, 0x44F, 0x440, 0x441, 0x442, 0x443, 0x436, 0x432, 0x44C, 0x44B, 0x437, 0x448, 0x44D, 0x449, 0x447,
			0x44A,
			0x42E, 0x410, 0x411, 0x426, 0x414, 0x415, 0x424, 0x413, 0x425, 0x418, 0x419, 0x41A, 0x41B, 0x41C, 0x41D,
			0x41E,
			0x41F, 0x42F, 0x420, 0x421, 0x422, 0x423, 0x416, 0x412, 0x42C, 0x42B, 0x417, 0x428, 0x42D, 0x429, 0x427,
			0x42A
		};
	unsigned int c;

	while ((c = static_cast<unsigned char>(*str_i++))) {
		if (c < 0x80) {
			*str_o++ = c;
		} else {
			c = KoiToUtf8[c - 0x80];
			if (c < 0x800) {
				*str_o++ = 0xC0 | (c >> 6);
				*str_o++ = 0x80 | (c & 0x3F);
			} else // (c < 0x10000) - two bytes. more than 16 bits not supported
			{
				*str_o++ = 0xE0 | (c >> 12);
				*str_o++ = 0x80 | ((c >> 6) & 0x3F);
				*str_o++ = 0x80 | (c & 0x3F);
			}
		}
	}

	*str_o = 0;
}

#endif // HAVE_ICONV

bool sprintbitwd(Bitvector bitvector, const char *names[], char *result, const char *div, const int print_flag) {

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

void MemLeakInfo() {
	static int last_pmem_used = 0;
	auto get_mem = TotalMemUse();
	int vmem_used = get_mem.first;
	int pmem_used = get_mem.second;
	char buf [256];

	if (pmem_used != last_pmem_used) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		last_pmem_used = pmem_used;
		sprintf(buf, "Memory size mismatch, last: phys (%d kb), current: virt (%d kB) phys (%d kB)", last_pmem_used, vmem_used, pmem_used);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		last_pmem_used = pmem_used;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
