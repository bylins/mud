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
#include <third_party_libs/fmt/include/fmt/format.h>

#include "entities/world_characters.h"
#include "obj_prototypes.h"
#include "logger.h"
#include "entities/obj_data.h"
#include "db.h"
#include "comm.h"
#include "color.h"
#include "game_magic/spells.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "game_crafts/im.h"
#include "dg_script/dg_scripts.h"
#include "administration/privilege.h"
#include "entities/char_data.h"
#include "entities/room_data.h"
#include "modify.h"
#include "house.h"
#include "entities/player_races.h"
#include "depot.h"
#include "obj_save.h"
#include "game_fight/fight.h"
#include "game_skills/skills.h"
#include "game_economics/exchange.h"
#include "game_mechanics/sets_drop.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "conf.h"
#include "game_mechanics/obj_sets.h"
#include "utils_string.h"


#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include <boost/algorithm/string/trim.hpp>

#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <sstream>

extern DescriptorData *descriptor_list;
extern CharData *mob_proto;
extern const char *weapon_class[];
// local functions
TimeInfoData *real_time_passed(time_t t2, time_t t1);
TimeInfoData *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);
bool IsValidEmail(const char *address);

// external functions
void do_echo(CharData *ch, char *argument, int cmd, int subcmd);

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

// return char with UID n
CharData *find_char(long n) {
	static long last_uid{0};
	static CharData *last_ch{nullptr};
	if (n == last_uid)
		return last_ch;
	CharData *mob = mob_by_uid[n];
	if (mob) {
		last_uid = n;
		last_ch = mob;
		return mob;
	}
	return find_pc(n);
}

// return pc online with UID n
CharData *find_pc(long n) {
	for (auto d = descriptor_list; d; d = d->next) {
		if (STATE(d) == CON_PLAYING && GET_ID(d->character) == n) {
			return d->character.get();
		}
	}
	return nullptr;
}

int MIN(int a, int b) {
	return (a < b ? a : b);
}

int MAX(int a, int b) {
	return (a > b ? a : b);
}

const char *first_letter(const char *txt) {
	if (txt) {
		while (*txt && !a_isalpha(*txt)) {
			//Предполагается, что для отправки клиенту используется только управляющий код с цветом
			//На данный момент в коде присутствует только еще один управляющий код для очистки экрана,
			//но он не используется (см. CLEAR_SCREEN)
			if ('\x1B' == *txt) {
				while (*txt && 'm' != *txt) {
					++txt;
				}
				if (!*txt) {
					return txt;
				}
			} else if ('&' == *txt) {
				++txt;
				if (!*txt) {
					return txt;
				}
			}
			++txt;
		}
	}
	return txt;
}

char *colorCAP(char *txt) {
	char *letter = const_cast<char *>(first_letter(txt));
	if (letter && *letter) {
		*letter = UPPER(*letter);
	}
	return txt;
}

std::string &colorCAP(std::string &txt) {
	size_t pos = first_letter(txt.c_str()) - txt.c_str();
	txt[pos] = UPPER(txt[pos]);
	return txt;
}

// rvalue variant
std::string &colorCAP(std::string &&txt) {
	return colorCAP(txt);
}

char *colorLOW(char *txt) {
	char *letter = const_cast<char *>(first_letter(txt));
	if (letter && *letter) {
		*letter = LOWER(*letter);
	}
	return txt;
}

std::string &colorLOW(std::string &txt) {
	size_t pos = first_letter(txt.c_str()) - txt.c_str();
	txt[pos] = LOWER(txt[pos]);
	return txt;
}

// rvalue variant
std::string &colorLOW(std::string &&txt) {
	return colorLOW(txt);
}

char *CAP(char *txt) {
	*txt = UPPER(*txt);
	return (txt);
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

// * Strips \r\n from end of string.
void prune_crlf(char *txt) {
	size_t i = strlen(txt) - 1;

	while (txt[i] == '\n' || txt[i] == '\r') {
		txt[i--] = '\0';
	}
}

bool is_head(std::string name) {
	if ((name == "Стрибог") || (name == "стрибог"))
		return true;
	return false;
}

int get_virtual_race(CharData *mob) {
	if (mob->get_role(MOB_ROLE_BOSS)) {
		return kNpcBoss;
	}
	std::map<int, int>::iterator it;
	std::map<int, int> unique_mobs = SetsDrop::get_unique_mob();
	for (it = unique_mobs.begin(); it != unique_mobs.end(); it++) {
		if (GET_MOB_VNUM(mob) == it->first)
			return kNpcUnique;
	}
	return -1;
}

/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */

int str_cmp(const char *arg1, const char *arg2) {
	int chk, i;

	if (arg1 == nullptr || arg2 == nullptr) {
		log("SYSERR: str_cmp() passed a nullptr pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; arg1[i] || arg2[i]; i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);    // not equal

	return (0);
}

int str_cmp(const std::string &arg1, const char *arg2) {
	int chk;
	std::string::size_type i;

	if (arg2 == nullptr) {
		log("SYSERR: str_cmp() passed a NULL pointer, %p.", arg2);
		return (0);
	}

	for (i = 0; i != arg1.length() && *arg2; i++, arg2++)
		if ((chk = LOWER(arg1[i]) - LOWER(*arg2)) != 0)
			return (chk);    // not equal

	if (i == arg1.length() && !*arg2)
		return (0);

	if (*arg2)
		return (LOWER('\0') - LOWER(*arg2));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}
int str_cmp(const char *arg1, const std::string &arg2) {
	int chk;
	std::string::size_type i;

	if (arg1 == nullptr) {
		log("SYSERR: str_cmp() passed a NULL pointer, %p.", arg1);
		return (0);
	}

	for (i = 0; *arg1 && i != arg2.length(); i++, arg1++)
		if ((chk = LOWER(*arg1) - LOWER(arg2[i])) != 0)
			return (chk);    // not equal

	if (!*arg1 && i == arg2.length())
		return (0);

	if (*arg1)
		return (LOWER(*arg1) - LOWER('\0'));
	else
		return (LOWER('\0') - LOWER(arg2[i]));
}
int str_cmp(const std::string &arg1, const std::string &arg2) {
	int chk;
	std::string::size_type i;

	for (i = 0; i != arg1.length() && i != arg2.length(); i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);    // not equal

	if (arg1.length() == arg2.length())
		return (0);

	if (i == arg1.length())
		return (LOWER('\0') - LOWER(arg2[i]));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, size_t n) {
	int chk, i;

	if (arg1 == nullptr || arg2 == nullptr) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);    // not equal

	return (0);
}

int strn_cmp(const std::string &arg1, const char *arg2, size_t n) {
	int chk;
	std::string::size_type i;

	if (arg2 == nullptr) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p.", arg2);
		return (0);
	}

	for (i = 0; i != arg1.length() && *arg2 && (n > 0); i++, arg2++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(*arg2)) != 0)
			return (chk);    // not equal

	if (i == arg1.length() && (!*arg2 || n == 0))
		return (0);

	if (*arg2)
		return (LOWER('\0') - LOWER(*arg2));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

int strn_cmp(const char *arg1, const std::string &arg2, size_t n) {
	int chk;
	std::string::size_type i;

	if (arg1 == nullptr) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p.", arg1);
		return (0);
	}

	for (i = 0; *arg1 && i != arg2.length() && (n > 0); i++, arg1++, n--)
		if ((chk = LOWER(*arg1) - LOWER(arg2[i])) != 0)
			return (chk);    // not equal

	if (!*arg1 && (i == arg2.length() || n == 0))
		return (0);

	if (*arg1)
		return (LOWER(*arg1) - LOWER('\0'));
	else
		return (LOWER('\0') - LOWER(arg2[i]));
}

int strn_cmp(const std::string &arg1, const std::string &arg2, size_t n) {
	int chk;
	std::string::size_type i;

	for (i = 0; i != arg1.length() && i != arg2.length() && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);    // not equal

	if (arg1.length() == arg2.length() || (n == 0))
		return (0);

	if (i == arg1.length())
		return (LOWER('\0') - LOWER(arg2[i]));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

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

// * Calculate the REAL time passed over the last t2-t1 centuries (secs)
TimeInfoData *real_time_passed(time_t t2, time_t t1) {
	long secs;
	static TimeInfoData now;

	secs = (long) (t2 - t1);

	now.hours = (secs / kSecsPerRealHour) % 24;    // 0..23 hours //
	secs -= kSecsPerRealHour * now.hours;

	now.day = (secs / kSecsPerRealDay);    // 0..34 days  //
	// secs -= SECS_PER_REAL_DAY * now.day; - Not used. //

	now.month = -1;
	now.year = -1;

	return (&now);
}

// Calculate the MUD time passed over the last t2-t1 centuries (secs) //
TimeInfoData *mud_time_passed(time_t t2, time_t t1) {
	long secs;
	static TimeInfoData now;

	secs = (long) (t2 - t1);

	now.hours = (secs / (kSecsPerMudHour * kTimeKoeff)) % kHoursPerDay;    // 0..23 hours //
	secs -= kSecsPerMudHour * kTimeKoeff * now.hours;

	now.day = (secs / (kSecsPerMudDay * kTimeKoeff)) % kDaysPerMonth;    // 0..29 days  //
	secs -= kSecsPerMudDay * kTimeKoeff * now.day;

	now.month = (secs / (kSecsPerMudMonth * kTimeKoeff)) % kMonthsPerYear;    // 0..11 months //
	secs -= kSecsPerMudMonth * kTimeKoeff * now.month;

	now.year = (secs / (kSecsPerMudYear * kTimeKoeff));    // 0..XX? years //

	return (&now);
}

TimeInfoData *age(const CharData *ch) {
	static TimeInfoData player_age;

	player_age = *mud_time_passed(time(nullptr), ch->player_data.time.birth);

	player_age.year += 17;    // All players start at 17 //

	return (&player_age);
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
		fgets(temp, 256, fl);

		if (feof(fl)) {
			return 0;
		}
		lines++;
	} while (*temp == '*' || *temp == '\n');

	temp[strlen(temp) - 1] = '\0';
	strcpy(buf, temp);

	return lines;
}

int get_filename(const char *orig_name, char *filename, int mode) {
	const char *prefix, *middle, *suffix;
	char name[64], *ptr;

	if (orig_name == nullptr || *orig_name == '\0' || filename == nullptr) {
		log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.", orig_name, filename);
		return (0);
	}

	switch (mode) {
		case kAliasFile: prefix = LIB_PLRALIAS;
			suffix = SUF_ALIAS;
			break;
		case kScriptVarsFile: prefix = LIB_PLRVARS;
			suffix = SUF_MEM;
			break;
		case kPlayersFile: prefix = LIB_PLRS;
			suffix = SUF_PLAYER;
			break;
		case kTextCrashFile: prefix = LIB_PLROBJS;
			suffix = TEXT_SUF_OBJS;
			break;
		case kTimeCrashFile: prefix = LIB_PLROBJS;
			suffix = TIME_SUF_OBJS;
			break;
		case kPersDepotFile: prefix = LIB_DEPOT;
			suffix = SUF_PERS_DEPOT;
			break;
		case kShareDepotFile: prefix = LIB_DEPOT;
			suffix = SUF_SHARE_DEPOT;
			break;
		case kPurgeDepotFile: prefix = LIB_DEPOT;
			suffix = SUF_PURGE_DEPOT;
			break;
		default: return (0);
	}

	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++) {
		if (*ptr == 'Ё' || *ptr == 'ё')
			*ptr = '9';
		else
			*ptr = LOWER(AtoL(*ptr));
	}

	switch (LOWER(*name)) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e': middle = LIB_A;
			break;
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j': middle = LIB_F;
			break;
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o': middle = LIB_K;
			break;
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't': middle = LIB_P;
			break;
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z': middle = LIB_U;
			break;
		default: middle = LIB_Z;
			break;
	}

	sprintf(filename, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
	return (1);
}

int num_pc_in_room(RoomData *room) {
	int i = 0;
	for (const auto ch : room->people) {
		if (!ch->IsNpc()) {
			i++;
		}
	}

	return i;
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

			strncpy(replace_buffer, replacement, replacement_length);
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
	}
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

int check_moves(CharData *ch, int how_moves) {
	if (IS_IMMORTAL(ch) || ch->IsNpc())
		return (true);
	if (GET_MOVE(ch) < how_moves) {
		SendMsgToChar("Вы слишком устали.\r\n", ch);
		return (false);
	}
	GET_MOVE(ch) -= how_moves;
	return (true);
}

int real_sector(int room) {
	int sector = SECT(room);

	if (ROOM_FLAGGED(room, ERoomFlag::kNoWeather))
		return sector;
	switch (sector) {
		case ESector::kInside:
		case ESector::kCity:
		case ESector::kOnlyFlying:
		case ESector::kUnderwater:
		case ESector::kSecret:
		case ESector::kStoneroad:
		case ESector::kRoad:
		case ESector::kWildroad: return sector;
			break;
		case ESector::kField:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kFieldSnow;
			else if (world[room]->weather.rainlevel > 20)
				return ESector::kFieldRain;
			else
				return ESector::kField;
			break;
		case ESector::kForest:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kForestSnow;
			else if (world[room]->weather.rainlevel > 20)
				return ESector::kForestRain;
			else
				return ESector::kForest;
			break;
		case ESector::kHills:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kHillsSnow;
			else if (world[room]->weather.rainlevel > 20)
				return ESector::kHillsRain;
			else
				return ESector::kHills;
			break;
		case ESector::kMountain:
			if (world[room]->weather.snowlevel > 20)
				return ESector::kMountainSnow;
			else
				return ESector::kMountain;
			break;
		case ESector::kWaterSwim:
		case ESector::kWaterNoswim:
			if (world[room]->weather.icelevel > 30)
				return ESector::kThickIce;
			else if (world[room]->weather.icelevel > 20)
				return ESector::kNormalIce;
			else if (world[room]->weather.icelevel > 10)
				return ESector::kThinIce;
			else
				return sector;
			break;
	}
	return ESector::kInside;
}

bool same_group(CharData *ch, CharData *tch) {
	if (!ch || !tch)
		return false;

	// Добавлены проверки чтобы не любой заследовавшийся моб считался согруппником (Купала)
	if (ch->IsNpc()
		&& ch->has_master()
		&& !ch->get_master()->IsNpc()
		&& (IS_HORSE(ch)
			|| AFF_FLAGGED(ch, EAffect::kCharmed)
			|| MOB_FLAGGED(ch, EMobFlag::kTutelar)
			|| MOB_FLAGGED(ch, EMobFlag::kMentalShadow))) {
		ch = ch->get_master();
	}

	if (tch->IsNpc()
		&& tch->has_master()
		&& !tch->get_master()->IsNpc()
		&& (IS_HORSE(tch)
			|| AFF_FLAGGED(tch, EAffect::kCharmed)
			|| MOB_FLAGGED(tch, EMobFlag::kTutelar)
			|| MOB_FLAGGED(tch, EMobFlag::kMentalShadow))) {
		tch = tch->get_master();
	}

	// NPC's always in same group
	if ((ch->IsNpc() && tch->IsNpc())
		|| ch == tch) {
		return true;
	}

	if (!AFF_FLAGGED(ch, EAffect::kGroup)
		|| !AFF_FLAGGED(tch, EAffect::kGroup)) {
		return false;
	}

	if (ch->get_master() == tch
		|| tch->get_master() == ch
		|| (ch->has_master()
			&& ch->get_master() == tch->get_master())) {
		return true;
	}

	return false;
}

// Проверка является комната рентой.
bool is_rent(RoomRnum room) {
	// комната с флагом замок, но клан мертвый
	if (ROOM_FLAGGED(room, ERoomFlag::kHouse)) {
		const auto clan = Clan::GetClanByRoom(room);
		if (!clan) {
			return false;
		}
	}
	// комната без рентера в ней
	for (const auto ch : world[room]->people) {
		if (ch->IsNpc()
			&& IS_RENTKEEPER(ch)) {
			return true;
		}
	}
	return false;
}

// Проверка является комната почтой.
int is_post(RoomRnum room) {
	for (const auto ch : world[room]->people) {
		if (ch->IsNpc()
			&& IS_POSTKEEPER(ch)) {
			return (true);
		}
	}
	return (false);

}

// Форматирование вывода в соответствии с форматом act-a
// output act format//
char *format_act(const char *orig, CharData *ch, ObjData *obj, const void *vict_obj) {
	const char *i = nullptr;
	char *buf, *lbuf;
	ubyte padis;
	int stopbyte;
//	CharacterData *dg_victim = nullptr;

	buf = (char *) malloc(kMaxStringLength);
	lbuf = buf;

	for (stopbyte = 0; stopbyte < kMaxStringLength; stopbyte++) {
		if (*orig == '$') {
			switch (*(++orig)) {
				case 'n':
					if (*(orig + 1) < '0' || *(orig + 1) > '5')
						i = ch->get_name().c_str();
					else {
						padis = *(++orig) - '0';
						i = GET_PAD(ch, padis);
					}
					break;
				case 'N':
					if (*(orig + 1) < '0' || *(orig + 1) > '5') {
						CHECK_NULL(vict_obj, GET_PAD((const CharData *) vict_obj, 0));
					} else {
						padis = *(++orig) - '0';
						CHECK_NULL(vict_obj, GET_PAD((const CharData *) vict_obj, padis));
					}
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'm': i = HMHR(ch);
					break;
				case 'M':
					if (vict_obj)
						i = HMHR((const CharData *) vict_obj);
					else CHECK_NULL(obj, OMHR(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 's': i = HSHR(ch);
					break;
				case 'S':
					if (vict_obj)
						i = HSHR((const CharData *) vict_obj);
					else CHECK_NULL(obj, OSHR(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'e': i = HSSH(ch);
					break;
				case 'E':
					if (vict_obj)
						i = HSSH((const CharData *) vict_obj);
					else CHECK_NULL(obj, OSSH(obj));
					break;

				case 'o':
					if (*(orig + 1) < '0' || *(orig + 1) > '5') {
						CHECK_NULL(obj, obj->get_PName(0).c_str());
					} else {
						padis = *(++orig) - '0';
						CHECK_NULL(obj, obj->get_PName(padis > 5 ? 0 : padis).c_str());
					}
					break;
				case 'O':
					if (*(orig + 1) < '0' || *(orig + 1) > '5') {
						CHECK_NULL(vict_obj, ((const ObjData *) vict_obj)->get_PName(0).c_str());
					} else {
						padis = *(++orig) - '0';
						CHECK_NULL(vict_obj, ((const ObjData *) vict_obj)->get_PName(padis > 5 ? 0 : padis).c_str());
					}
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 't': CHECK_NULL(obj, (const char *) obj);
					break;

				case 'T': CHECK_NULL(vict_obj, (const char *) vict_obj);
					break;

				case 'F': CHECK_NULL(vict_obj, (const char *) vict_obj);
					break;

				case '$': i = "$";
					break;

				case 'a': i = GET_CH_SUF_6(ch);
					break;
				case 'A':
					if (vict_obj)
						i = GET_CH_SUF_6((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_6(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'g': i = GET_CH_SUF_1(ch);
					break;
				case 'G':
					if (vict_obj)
						i = GET_CH_SUF_1((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_1(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'y': i = GET_CH_SUF_5(ch);
					break;
				case 'Y':
					if (vict_obj)
						i = GET_CH_SUF_5((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_5(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'u': i = GET_CH_SUF_2(ch);
					break;
				case 'U':
					if (vict_obj)
						i = GET_CH_SUF_2((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_2(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'w': i = GET_CH_SUF_3(ch);
					break;
				case 'W':
					if (vict_obj)
						i = GET_CH_SUF_3((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_3(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;

				case 'q': i = GET_CH_SUF_4(ch);
					break;
				case 'Q':
					if (vict_obj)
						i = GET_CH_SUF_4((const CharData *) vict_obj);
					else CHECK_NULL(obj, GET_OBJ_SUF_4(obj));
					//dg_victim = (CharacterData *) vict_obj;
					break;
//Polud Добавил склонение местоимения ваш(е,а,и)
				case 'z':
					if (obj)
						i = OYOU(obj);
					else CHECK_NULL(obj, OYOU(obj));
					break;
				case 'Z':
					if (vict_obj)
						i = HYOU((const CharData *) vict_obj);
					else CHECK_NULL(vict_obj, HYOU((const CharData *) vict_obj));
					break;
//-Polud
				default: log("SYSERR: Illegal $-code to act(): %c", *orig);
					log("SYSERR: %s", orig);
					i = "";
					break;
			}
			while ((*buf = *(i++)))
				buf++;
			orig++;
		} else if (*orig == '\\') {
			if (*(orig + 1) == 'r') {
				*(buf++) = '\r';
				orig += 2;
			} else if (*(orig + 1) == 'n') {
				*(buf++) = '\n';
				orig += 2;
			} else
				*(buf++) = *(orig++);
		} else if (!(*(buf++) = *(orig++)))
			break;
	}

	*(--buf) = '\r';
	*(++buf) = '\n';
	*(++buf) = '\0';
	return (lbuf);
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

// Функция проверяет может ли ch нести предмет obj и загружает предмет
// в инвентарь игрока или в комнату, где игрок находится
void can_carry_obj(CharData *ch, ObjData *obj) {
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не можете нести столько предметов.", ch);
		PlaceObjToRoom(obj, ch->in_room);
		CheckObjDecay(obj);
	} else {
		if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(ch) > CAN_CARRY_W(ch)) {
			sprintf(buf, "Вам слишком тяжело нести еще и %s.", obj->get_PName(3).c_str());
			SendMsgToChar(buf, ch);
			PlaceObjToRoom(obj, ch->in_room);
			// obj_decay(obj);
		} else {
			PlaceObjToInventory(obj, ch);
		}
	}
}

/**
 * Бывшее #define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
 */
bool CAN_CARRY_OBJ(const CharData *ch, const ObjData *obj) {
	// для анлимного лута мобами из трупов
	if (ch->IsNpc() && !IS_CHARMICE(ch)) {
		return true;
	}

	if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)
		&& IS_CARRYING_N(ch) + 1 <= CAN_CARRY_N(ch)) {
		return true;
	}

	return false;
}

// shapirus: проверка, игнорирет ли чар who чара whom
bool ignores(CharData *who, CharData *whom, unsigned int flag) {
	if (who->IsNpc()) return false;

	long ign_id;

// имморталов не игнорит никто
	if (IS_IMMORTAL(whom)) {
		return false;
	}

// чармисы игнорируемого хозяина тоже должны быть проигнорированы
	if (whom->IsNpc()
		&& AFF_FLAGGED(whom, EAffect::kCharmed)) {
		return ignores(who, whom->get_master(), flag);
	}

	ign_id = GET_IDNUM(whom);
	for (const auto &ignore : who->get_ignores()) {
		if ((ignore->id == ign_id || ignore->id == -1)
			&& IS_SET(ignore->mode, flag)) {
			return true;
		}
	}
	return false;
}

bool IsValidEmail(const char *address) {
	int count = 0;
	static std::string special_symbols("\r\n ()<>,;:\\\"[]|/&'`$");
	std::string addr = address;
	std::string::size_type dog_pos = 0, pos = 0;

	// Наличие запрещенных символов или кириллицы //
	if (addr.find_first_of(special_symbols) != std::string::npos) {
		return false;
	}
	size_t size = addr.size();
	for (size_t i = 0; i < size; i++) {
		if (addr[i] <= ' ' || addr[i] >= 127) {
			return false;
		}
	}
	// Собака должна быть только одна и на второй и далее позиции //
	while ((pos = addr.find_first_of('@', pos)) != std::string::npos) {
		dog_pos = pos;
		++count;
		++pos;
	}
	if (count != 1 || dog_pos == 0) {
		return false;
	}
	// Проверяем правильность синтаксиса домена //
	// В доменной части должно быть как минимум 4 символа, считая собаку //
	if (size - dog_pos <= 3) {
		return false;
	}
	// Точка отсутствует, расположена сразу после собаки, или на последнем месте //
	if (addr[dog_pos + 1] == '.' || addr[size - 1] == '.' || addr.find('.', dog_pos) == std::string::npos) {
		return false;
	}

	return true;
}

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

// * Для обрезания точек в карме при сете славы.
void skip_dots(char **string) {
	for (; **string && (strchr(" .", **string) != nullptr); (*string)++);
}

int RealZoneNum(ZoneVnum zvn) {
	for (int zrn = 0; zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
		if (zone_table[zrn].vnum == zvn)
			return zrn;
	}
	return -1;
}

// Return pointer to first occurrence in string ct in
// cs, or nullptr if not present.  Case insensitive
char *str_str(const char *cs, const char *ct) {
	if (!cs || !ct) {
		return nullptr;
	}

	while (*cs) {
		const char *t = ct;

		while (*cs && (LOWER(*cs) != LOWER(*t))) {
			cs++;
		}

		char *s = (char*)cs;
		while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
			t++;
			cs++;
		}

		if (!*t) {
			return s;
		}
	}

	return nullptr;
}

// remove ^M's from file output
void kill_ems(char *str) {
	char *ptr1, *ptr2;
	ptr1 = str;
	ptr2 = str;
	while (*ptr1) {
		if ('\r' != *ptr1) {
			*ptr2 = *ptr1;
			++ptr2;
		}
		++ptr1;
	}
	*ptr2 = '\0';
}

// * Вырезание и перемещение в word одного слова из str (a_isalnum).
void cut_one_word(std::string &str, std::string &word) {
	if (str.empty()) {
		word.clear();
		return;
	}

	bool process = false;
	unsigned begin = 0, end = 0;
	for (unsigned i = 0; i < str.size(); ++i) {
		if (!process && a_isalnum(str.at(i))) {
			process = true;
			begin = i;
			continue;
		}
		if (process && !a_isalnum(str.at(i))) {
			end = i;
			break;
		}
	}

	if (process) {
		if (!end || end >= str.size()) {
			word = str.substr(begin);
			str.clear();
		} else {
			word = str.substr(begin, end - begin);
			str.erase(0, end);
		}
		return;
	}

	str.clear();
	word.clear();
}

void ReadEndString(std::ifstream &file) {
	char c;
	while (file.get(c)) {
		if (c == '\n') {
			return;
		}
	}
}

void StringReplace(std::string &buffer, char s, const std::string &d) {
	for (size_t index = 0; index = buffer.find(s, index), index != std::string::npos;) {
		buffer.replace(index, 1, d);
		index += d.length();
	}
}

std::string &format_news_message(std::string &text) {
	StringReplace(text, '\n', "\n   ");
	utils::Trim(text);
	text.insert(0, "   ");
	text += '\n';
	return text;
}

/**
 * взято с http://www.openbsd.org/cgi-bin/cvsweb/src/lib/libc/string/strlcpy.c?rev=1.11&content-type=text/x-cvsweb-markup
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strl_cpy(char *dst, const char *src, size_t siz) {
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	// Copy as many bytes as will fit
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	// Not enough room in dst, add NUL and traverse rest of src
	if (n == 0) {
		if (siz != 0)
			*d = '\0';        // NUL-terminate dst
		while (*s++);
	}

	return (s - src - 1);    // count does not include NUL
}

////////////////////////////////////////////////////////////////////////////////
// show money

namespace MoneyDropStat {

typedef std::map<int /* zone vnum */, long /* money count */> ZoneListType;
ZoneListType zone_list;

void add(int zone_vnum, long money) {
	ZoneListType::iterator i = zone_list.find(zone_vnum);
	if (i != zone_list.end()) {
		i->second += money;
	} else {
		zone_list[zone_vnum] = money;
	}
}

void print(CharData *ch) {
	if (!IS_GRGOD(ch)) {
		SendMsgToChar(ch, "Только для иммов 33+.\r\n");
		return;
	}

	std::map<long, int> tmp_list;
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second > 0) {
			tmp_list[i->second] = i->first;
		}
	}
	if (!tmp_list.empty()) {
		SendMsgToChar(ch,
					  "Money drop stats:\r\n"
					  "Total zones: %lu\r\n"
					  "  vnum - money\r\n"
					  "================\r\n", tmp_list.size());
	} else {
		SendMsgToChar(ch, "Empty.\r\n");
		return;
	}
	std::stringstream out;
	for (std::map<long, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend; ++i) {
		out << "  " << std::setw(4) << i->second << " - " << i->first << "\r\n";
//		SendMsgToChar(ch, "  %4d - %ld\r\n", i->second, i->first);
	}
	page_string(ch->desc, out.str());
}

void print_log() {
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second > 0) {
			log("ZoneDrop: %d %ld", i->first, i->second);
		}
	}
}

} // MoneyDropStat

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// show exp
namespace ZoneExpStat {

struct zone_data {
	zone_data() : gain(0), lose(0) {};

	unsigned long gain;
	unsigned long lose;
};

typedef std::map<int /* zone vnum */, zone_data> ZoneListType;
ZoneListType zone_list;

void add(int zone_vnum, long exp) {
	if (!exp) {
		return;
	}

	ZoneListType::iterator i = zone_list.find(zone_vnum);
	if (i != zone_list.end()) {
		if (exp > 0) {
			i->second.gain += exp;
		} else {
			i->second.lose += -exp;
		}
	} else {
		zone_data tmp_zone;
		if (exp > 0) {
			tmp_zone.gain = exp;
		} else {
			tmp_zone.lose = -exp;
		}
		zone_list[zone_vnum] = tmp_zone;
	}
}

void print_gain(CharData *ch) {
	if (!PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		SendMsgToChar(ch, "Пока в разработке.\r\n");
		return;
	}

	std::map<unsigned long, int> tmp_list;
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second.gain > 0) {
			tmp_list[i->second.gain] = i->first;
		}
	}
	if (!tmp_list.empty()) {
		SendMsgToChar(ch,
					  "Gain exp stats:\r\n"
					  "Total zones: %lu\r\n"
					  "  vnum - exp\r\n"
					  "================\r\n", tmp_list.size());
	} else {
		SendMsgToChar(ch, "Empty.\r\n");
		return;
	}
	std::stringstream out;
	for (std::map<unsigned long, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend;
		 ++i) {
		out << "  " << std::setw(4) << i->second << " - " << i->first << "\r\n";
	}
	page_string(ch->desc, out.str());
}

void print_log() {
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i) {
		if (i->second.gain > 0 || i->second.lose > 0) {
			log("ZoneExp: %d %lu %lu", i->first, i->second.gain, i->second.lose);
		}
	}
}

} // ZoneExpStat
////////////////////////////////////////////////////////////////////////////////

std::string PrintNumberByDigits(long long num, const char separator) {
	const int digits_num{3};

	bool negative{false};
	if (num < 0) {
		num = -num;
		negative = true;
	}

	std::string buffer;
	try {
		buffer = std::to_string(num);
	} catch (std::bad_alloc &) {
		log("SYSERROR : string.at() (%s:%d)", __FILE__, __LINE__);
		return "<Out Of Range>";
	}

	fmt::memory_buffer out;
	if (negative) {
		out.push_back('-');
	}

	if (digits_num >= buffer.size()) {
		format_to(std::back_inserter(out), "{}", buffer);
	} else {
		auto modulo = buffer.size() % digits_num;
		if (modulo != 0) {
			format_to(std::back_inserter(out), "{}{}", buffer.substr(0, modulo), separator);
		}

		unsigned pos = modulo;
		while (pos < buffer.size() - digits_num) {
			format_to(std::back_inserter(out), "{}{}", buffer.substr(pos, digits_num), separator);
			pos += digits_num;
		}

		format_to(std::back_inserter(out), "{}", buffer.substr(pos, digits_num));
	}

	return to_string(out);
}

std::string thousands_sep(long long n) {
	bool negative = false;
	if (n < 0) {
		n = -n;
		negative = true;
	}

	int size = 50;
	int curr_pos = size - 1;
	const int comma = ',';
	std::string buffer;
	buffer.resize(size);
	int i = 0;

	try {
		do {
			if (i % 3 == 0 && i != 0) {
				buffer.at(--curr_pos) = comma;
			}
			buffer.at(--curr_pos) = '0' + n % 10;
			n /= 10;
			i++;
		} while (n != 0);

		if (negative) {
			buffer.at(--curr_pos) = '-';
		}
	}
	catch (...) {
		log("SYSERROR : string.at() (%s:%d)", __FILE__, __LINE__);
		return "<OutOfRange>";
	}

	buffer = buffer.substr(curr_pos, size - 1);
	return buffer;
}

int str_bonus(int str, int type) {
	int bonus = 0;
	str = MAX(1, str);

	switch (type) {
		case STR_TO_HIT:
			// -5 ... 10
			if (str < 10) {
				bonus = (str - 11) / 2;
			} else {
				bonus = (str - 10) / 4;
			}
			break;
		case STR_TO_DAM:
			// -5 ... 36
			if (str < 15) {
				bonus = (str - 11) / 2;
			} else {
				bonus = str - 14;
			}
			break;
		case STR_CARRY_W:
			// 50 ... 2500
			bonus = str * 50;
			break;
		case STR_WIELD_W:
			if (str <= 20) {
				// w 1 .. 20
				bonus = str;
			} else if (str < 30) {
				// w 20,5 .. 24,5
				bonus = 20 + (str - 20) / 2;
			} else {
				// w >= 25
				bonus = 25 + (str - 30) / 4;
			}
			break;
		case STR_HOLD_W:
			if (str <= 29) {
				// w 1 .. 15
				bonus = (str + 1) / 2;
			} else {
				// w >= 16
				bonus = 15 + (str - 29) / 4;
			}
			break;
		default:
			log("SYSERROR: str=%d, type=%d (%s %s %d)",
				str, type, __FILE__, __func__, __LINE__);
	}

	return bonus;
}

int dex_bonus(int dex) {
	int bonus = 0;
	dex = MAX(1, dex);
	// -5 ... 40
	if (dex < 10) {
		bonus = (dex - 11) / 2;
	} else {
		bonus = dex - 10;
	}
	return bonus;
}

int dex_ac_bonus(int dex) {
	int bonus = 0;
	dex = MAX(1, dex);
	// -3 ... 35
	if (dex <= 15) {
		bonus = (dex - 10) / 3;
	} else {
		bonus = dex - 15;
	}
	return bonus;
}

int wis_bonus(int stat, int type) {
	int bonus = 0;
	stat = MAX(1, stat);

	switch (type) {
		case WIS_MAX_LEARN_L20:
			// 28 .. 175
			bonus = 28 + (stat - 1) * 3;
			break;
		case WIS_SPELL_SUCCESS:
			// -80 .. 116
			bonus = stat * 4 - 84;
			break;
		case WIS_MAX_SKILLS:
			// 1 .. 15
			if (stat <= 15) {
				bonus = stat;
			}
				// 15 .. 32
			else {
				bonus = 15 + (stat - 15) / 2;
			}
			break;
		case WIS_FAILS:
			// 34 .. 66
			if (stat <= 9) {
				bonus = 30 + stat * 4;
			}
				// 140 .. 940
			else {
				bonus = 120 + (stat - 9) * 20;
			}
			break;
		default:
			log("SYSERROR: stat=%d, type=%d (%s %s %d)",
				stat, type, __FILE__, __func__, __LINE__);
	}

	return bonus;
}

int calc_str_req(int weight, int type) {
	int str = 0;
	weight = MAX(1, weight);

	switch (type) {
		case STR_WIELD_W:
			if (weight <= 20) {
				str = weight;
			} else if (weight <= 24) {
				str = 2 * weight - 20;
			} else {
				str = 4 * weight - 70;
			}
			break;
		case STR_HOLD_W:
			if (weight <= 15) {
				str = 2 * weight - 1;
			} else {
				str = 4 * weight - 31;
			}
			break;
		case STR_BOTH_W:
			if (weight <= 31) {
				str = weight - (weight + 1) / 3;
			} else if (weight <= 40) {
				str = weight - 10;
			} else {
				str = (weight - 39) * 2 + 29 - (weight + 1) % 2;
			}
			break;
		default:
			log("SYSERROR: weight=%d, type=%d (%s %s %d)",
				weight, type, __FILE__, __func__, __LINE__);
	}

	return str;
}

void message_str_need(CharData *ch, ObjData *obj, int type) {
	if (GET_POS(ch) == EPosition::kDead)
		return;
	int need_str = 0;
	switch (type) {
		case STR_WIELD_W: need_str = calc_str_req(GET_OBJ_WEIGHT(obj), STR_WIELD_W);
			break;
		case STR_HOLD_W: need_str = calc_str_req(GET_OBJ_WEIGHT(obj), STR_HOLD_W);
			break;
		case STR_BOTH_W: need_str = calc_str_req(GET_OBJ_WEIGHT(obj), STR_BOTH_W);
			break;
		case STR_SHIELD_W: need_str = calc_str_req((GET_OBJ_WEIGHT(obj) + 1) / 2, STR_HOLD_W);
			break;
		default:
			log("SYSERROR: ch=%s, weight=%d, type=%d (%s %s %d)",
				GET_NAME(ch), GET_OBJ_WEIGHT(obj), type,
				__FILE__, __func__, __LINE__);
			return;
	}
	SendMsgToChar(ch, "Для этого требуется %d %s.\r\n",
				  need_str, GetDeclensionInNumber(need_str, EWhat::kStr));
}

/// считает кол-во цветов &R и т.п. в строке
/// size_t len = 0 - по дефолту считает strlen(str)
size_t count_colors(const char *str, size_t len) {
	unsigned int c, cc = 0;
	len = len ? len : strlen(str);

	for (c = 0; c < len - 1; c++) {
		if (*(str + c) == '&' && *(str + c + 1) != '&')
			cc++;
	}
	return cc;
}

//возвращает строку длины len + кол-во цветов*2 для того чтоб в табличке все было ровненько
//left_align выравнивание строки влево
char *colored_name(const char *str, size_t len, const bool left_align) {
	static char cstr[128];
	static char fmt[6];
	size_t cc = len + count_colors(str) * 2;

	if (strlen(str) < cc) {
		snprintf(fmt, sizeof(fmt), "%%%s%ds", (left_align ? "-" : ""), static_cast<int>(cc));
		snprintf(cstr, sizeof(cstr), fmt, str);
	} else {
		snprintf(cstr, sizeof(cstr), "%s", str);
	}

	return cstr;
}

/// длина строки без символов цвета из count_colors()
size_t strlen_no_colors(const char *str) {
	const size_t len = strlen(str);
	return len - count_colors(str, len) * 2;
}

// Симуляция телла от моба
void tell_to_char(CharData *keeper, CharData *ch, const char *arg) {
	char local_buf[kMaxInputLength];
	if (AFF_FLAGGED(ch, EAffect::kDeafness) || PRF_FLAGGED(ch, EPrf::kNoTell)) {
		sprintf(local_buf, "жестами показал$g на свой рот и уши. Ну его, болезного ..");
		do_echo(keeper, local_buf, 0, SCMD_EMOTE);
		return;
	}
	snprintf(local_buf, kMaxInputLength,
			 "%s сказал%s вам : '%s'", GET_NAME(keeper), GET_CH_SUF_1(keeper), arg);
	SendMsgToChar(ch, "%s%s%s\r\n",
				  CCICYN(ch, C_NRM), CAP(local_buf), CCNRM(ch, C_NRM));
}

int CAN_CARRY_N(const CharData *ch) {
	int n = 5 + GetRealDex(ch) / 2 + GetRealLevel(ch) / 2;
	if (ch->HaveFeat(EFeat::kJuggler)) {
		n += GetRealLevel(ch) / 2;
		if (CanUseFeat(ch, EFeat::kThrifty)) {
			n += 5;
		}
	}
	if (CanUseFeat(ch, EFeat::kThrifty)) {
		n += 5;
	}
	return std::max(n, 1);
}

const char *print_obj_state(int tm_pct) {
	if (tm_pct < 20)
		return "ужасно";
	else if (tm_pct < 40)
		return "скоро сломается";
	else if (tm_pct < 60)
		return "плоховато";
	else if (tm_pct < 80)
		return "средне";
		//else if (tm_pct <=100) // у только что созданной шмотки значение 100% первый тик, потому <=
		//	return "идеально";
	else if (tm_pct < 1000) // проблема крафта, на хаймортах таймер больще прототипа
		return "идеально";
	else return "нерушимо";
}

void sanity_check(void) {
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

int GetRealLevel(const CharData *ch) {

	if (ch->IsNpc()) {
		return std::clamp(ch->GetLevel() + ch->get_level_add(), 0, kMaxMobLevel);
	}

	if (IS_IMMORTAL(ch)) {
		return ch->GetLevel();
	}

	return std::clamp(ch->GetLevel() + ch->get_level_add(), 1, kLvlImmortal - 1);
}

int GetRealLevel(const std::shared_ptr<CharData> &ch) {
	return GetRealLevel(ch.get());
}

short GetRealRemort(const CharData *ch) {
	return std::clamp(ch->get_remort() + ch->get_remort_add(), 0, kMaxRemort);
}

short GetRealRemort(const std::shared_ptr<CharData> &ch) {
	return GetRealRemort(ch.get());
}

/*short GetRealRemort(const std::shared_ptr<CharData> &ch) {
	return GetRealRemort(ch.get());
}*/

bool IsNegativeApply(EApply location) {
	for (auto elem : apply_negative) {
		if (location == elem.location)
			return true;
	}
	return false;
}

bool isname(const char *str, const char *namelist) {
	bool once_ok = false;
	const char *curname, *curstr, *laststr;

	if (!namelist || !*namelist || !str) {
		return false;
	}

	for (curstr = str; !a_isalnum(*curstr); curstr++) {
		if (!*curstr) {
			return once_ok;
		}
	}

	laststr = curstr;
	curname = namelist;
	for (;;) {
		once_ok = false;
		for (;; curstr++, curname++) {
			if (!*curstr) {
				return once_ok;
			}

			if (*curstr == '!') {
				if (a_isalnum(*curname)) {
					curstr = laststr;
					break;
				}
			}

			if (!a_isalnum(*curstr)) {
				for (; !a_isalnum(*curstr); curstr++) {
					if (!*curstr) {
						return once_ok;
					}
				}
				laststr = curstr;
				break;
			}

			if (!*curname) {
				return false;
			}

			if (!a_isalnum(*curname)) {
				curstr = laststr;
				break;
			}

			if (LOWER(*curstr) != LOWER(*curname)) {
				curstr = laststr;
				break;
			} else {
				once_ok = true;
			}
		}

		// skip to next name
		for (; a_isalnum(*curname); curname++);
		for (; !a_isalnum(*curname); curname++) {
			if (!*curname) {
				return false;
			}
		}
	}
}

const char *one_word(const char *argument, char *first_arg) {
	char *begin = first_arg;

	skip_spaces(&argument);
	first_arg = begin;

	if (*argument == '\"') {
		argument++;
		while (*argument && *argument != '\"') {
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}
		argument++;
	} else {
		while (*argument && !a_isspace(*argument)) {
			*(first_arg++) = a_lcc(*argument);
			argument++;
		}
	}
	*first_arg = '\0';

	return argument;
}

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

#include <iostream>

void build_char_table(int(*func)(int c)) {
	for (int c = 0; c < 256; c++) {
		std::cout << (func(c) ? " true" : "false") << (255 > c ? ", " : "");
		if (0 < c && 0 == (1 + c) % 16) {
			std::cout << std::endl;
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
			bool result = 0 != m_original(i % 256);
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
	std::cout << "Validity... " << std::endl;
	std::cout << (test_values() ? "passed" : "failed") << std::endl;
	std::cout << "Performance... " << std::endl;
	std::cout << std::setprecision(2) << std::fixed << test_time() * 100 << "%" << std::endl;
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
							0xF2, 0xF3, 0xF4, 0xF5, 0xE6, 0xE8, 0xF3, 0xFE, 0xFB, 0xFD, 0xFF, 0xF9, 0xF8, 0xFC, 0xE0,
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
					*str_o = static_cast<char>(Utf8ToKoiPg[u - 0x2500]);
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

ZoneVnum GetZoneVnumByCharPlace(CharData *ch) {
		return zone_table[world[ch->in_room]->zone_rn].vnum;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
