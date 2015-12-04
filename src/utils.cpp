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

#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <boost/bind.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"

#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "im.h"
#include "dg_scripts.h"
#include "features.hpp"
#include "boards.h"
#include "privilege.hpp"
#include "char.hpp"
#include "room.hpp"
#include "modify.h"
#include "house.h"
#include "player_races.hpp"
#include "depot.hpp"
#include "objsave.h"
#include "fight.h"
#include "skills.h"
#include "exchange.h"

extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *mob_proto;
extern int top_of_p_table;
extern const char *weapon_class[];
extern bool check_unlimited_timer(OBJ_DATA *obj);
// local functions
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1);
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);
int valid_email(const char *address);
#define MINI_SET_ITEMS 3
// external functions
int attack_best(CHAR_DATA * ch, CHAR_DATA * victim);
void perform_drop_gold(CHAR_DATA * ch, int amount, byte mode, room_rnum RDR);
// Файл для вывода
FILE *logfile = NULL;

char AltToKoi[] =
{
	"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмноп░▒▓│┤╡+++╣║╗╝+╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨++╙╘╒++╪┘┌█▄▌▐▀рстуфхцчшщъыьэюяЁё╫╜╢╓╤╕╥╖·√??■ "
};
char KoiToAlt[] =
{
	"дЁз©юыц╢баеъэшщч╟╠╡+Ч+Ш+++Ъ+++З+м╨уЯУиВЫ╩тсх╬С╪фгл╣ПТ╧ЖЬкопйьРн+Н═║Ф╓╔ДёЕ╗╘╙╚╛╜╝╞ОЮАБЦ╕╒ЛК╖ХМИГЙ·─│√└┘■┐∙┬┴┼▀▄█▌▐÷░▒▓⌠├┌°⌡┤≤²≥≈ "
};
char WinToKoi[] =
{
	"++++++++++++++++++++++++++++++++ ++++╫++Ё©╢++++╥°+╤╕╜++·ё+╓++++╖АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя"
};
char KoiToWin[] =
{
	"++++++++++++++++++++++++++═+╟+╥++++╦╨+Ё©+++++╢+++++╗╙+╡╞+++++╔+╘ЧЮАЖДЕТЦУХИЙКЛМНОЪПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"
};
char KoiToWin2[] =
{
	"++++++++++++++++++++++++++═+╟+╥++++╦╨+Ё©+++++╢+++++╗╙+╡╞+++++╔+╘ЧЮАЖДЕТЦУХИЙКЛМНОzПЯРСФБЭШГЬЩЫВЗчюаждетцухийклмноъпярсфбэшгьщывз"
};
char AltToLat[] =
{
	"─│┌┐└┘├┤┬┴┼▀▄█▌▐░▒▓⌠■∙√≈≤≥ ⌡°²·÷═║╒ё╓╔╕╖╗╘╙╚╛╜╝╞╟╠╡Ё╢╣╤╥╦╧╨╩╪╫╬©0abcdefghijklmnopqrstY1v23z456780ABCDEFGHIJKLMNOPQRSTY1V23Z45678"
};

const char *ACTNULL = "<NULL>";


// return char with UID n
CHAR_DATA *find_char(long n)
{
	CHAR_DATA *ch;
	for (ch = character_list; ch; ch = ch->next)
		if (GET_ID(ch) == n)
			return (ch);
	return NULL;
}



int MIN(int a, int b)
{
	return (a < b ? a : b);
}


int MAX(int a, int b)
{
	return (a > b ? a : b);
}


char * CAP(char *txt)
{
	*txt = UPPER(*txt);
	return (txt);
}

// Create and append to dinamyc length string - Alez
char *str_add(char *dst, const char *src)
{
	if (dst == NULL)
	{
		dst = (char *) malloc(strlen(src) + 1);
		strcpy(dst, src);
	}
	else
	{
		dst = (char *) realloc(dst, strlen(dst) + strlen(src) + 1);
		strcat(dst, src);
	};
	return dst;
}

// Create a duplicate of a string
char *str_dup(const char *source)
{
	char *new_z = NULL;
	if (source)
	{
		CREATE(new_z, char, strlen(source) + 1);
		return (strcpy(new_z, source));
	}
	CREATE(new_z, char, 1);
	return (strcpy(new_z, ""));
}

// * Strips \r\n from end of string.
void prune_crlf(char *txt)
{
	int i = strlen(txt) - 1;

	while (txt[i] == '\n' || txt[i] == '\r')
		txt[i--] = '\0';
}


/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
	int chk, i;

	if (arg1 == NULL || arg2 == NULL)
	{
		log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; arg1[i] || arg2[i]; i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);	// not equal

	return (0);
}
int str_cmp(const std::string &arg1, const char *arg2)
{
	int chk;
	std::string::size_type i;

	if (arg2 == NULL)
	{
		log("SYSERR: str_cmp() passed a NULL pointer, %p.", arg2);
		return (0);
	}

	for (i = 0; i != arg1.length() && *arg2; i++, arg2++)
		if ((chk = LOWER(arg1[i]) - LOWER(*arg2)) != 0)
			return (chk);	// not equal

	if (i == arg1.length() && !*arg2)
		return (0);

	if (*arg2)
		return (LOWER('\0') - LOWER(*arg2));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}
int str_cmp(const char *arg1, const std::string &arg2)
{
	int chk;
	std::string::size_type i;

	if (arg1 == NULL)
	{
		log("SYSERR: str_cmp() passed a NULL pointer, %p.", arg1);
		return (0);
	}

	for (i = 0; *arg1 && i != arg2.length(); i++, arg1++)
		if ((chk = LOWER(*arg1) - LOWER(arg2[i])) != 0)
			return (chk);	// not equal

	if (!*arg1 && i == arg2.length())
		return (0);

	if (*arg1)
		return (LOWER(*arg1) - LOWER('\0'));
	else
		return (LOWER('\0') - LOWER(arg2[i]));
}
int str_cmp(const std::string &arg1, const std::string &arg2)
{
	int chk;
	std::string::size_type i;

	for (i = 0; i != arg1.length() && i != arg2.length(); i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);	// not equal

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
int strn_cmp(const char *arg1, const char *arg2, int n)
{
	int chk, i;

	if (arg1 == NULL || arg2 == NULL)
	{
		log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);	// not equal

	return (0);
}
int strn_cmp(const std::string &arg1, const char *arg2, int n)
{
	int chk;
	std::string::size_type i;

	if (arg2 == NULL)
	{
		log("SYSERR: strn_cmp() passed a NULL pointer, %p.", arg2);
		return (0);
	}

	for (i = 0; i != arg1.length() && *arg2 && (n > 0); i++, arg2++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(*arg2)) != 0)
			return (chk);	// not equal

	if (i == arg1.length() && (!*arg2 || n == 0))
		return (0);

	if (*arg2)
		return (LOWER('\0') - LOWER(*arg2));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}
int strn_cmp(const char *arg1, const std::string &arg2, int n)
{
	int chk;
	std::string::size_type i;

	if (arg1 == NULL)
	{
		log("SYSERR: strn_cmp() passed a NULL pointer, %p.", arg1);
		return (0);
	}

	for (i = 0; *arg1 && i != arg2.length() && (n > 0); i++, arg1++, n--)
		if ((chk = LOWER(*arg1) - LOWER(arg2[i])) != 0)
			return (chk);	// not equal

	if (!*arg1 && (i == arg2.length() || n == 0))
		return (0);

	if (*arg1)
		return (LOWER(*arg1) - LOWER('\0'));
	else
		return (LOWER('\0') - LOWER(arg2[i]));
}
int strn_cmp(const std::string &arg1, const std::string &arg2, int n)
{
	int chk;
	std::string::size_type i;

	for (i = 0; i != arg1.length() && i != arg2.length() && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);	// not equal

	if (arg1.length() == arg2.length() || (n == 0))
		return (0);

	if (i == arg1.length())
		return (LOWER('\0') - LOWER(arg2[i]));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

// дескрипторы открытых файлов логов для сброса буфера при креше
std::list<FILE *> opened_files;

// * Чтобы не дублировать создание даты в каждом виде лога.
void write_time(FILE *file)
{
	char time_buf[20];
	time_t ct = time(0);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	fprintf(file, "%s :: ", time_buf);
}

// * Так, потестить...
void write_test_time(FILE *file)
{
	char time_buf[20];
	struct timeval tv;

	gettimeofday(&tv, 0);
	time_t ct = tv.tv_sec;

	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	fprintf(file, "%s.%ld :: ", time_buf, tv.tv_usec);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void log(const char *format, ...)
{
	if (logfile == NULL)
		puts("SYSERR: Using log() before stream was initialized!");
	if (format == NULL)
		format = "SYSERR: log() received a NULL format.";

	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	time_s[strlen(time_s) - 1] = '\0';
	fprintf(logfile, "%-15.15s :: ", time_s + 4);

//	write_test_time(logfile);
	va_list args;
	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);
	fprintf(logfile, "\n");

// shapirus: для дебаггинга
#ifdef LOG_AUTOFLUSH
	fflush(logfile);
#endif
}

void shop_log(const char *format, ...)
{
	const char *filename = "../log/shop.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: olc_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

void olc_log(const char *format, ...)
{
	const char *filename = "../log/olc.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: olc_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

void imm_log(const char *format, ...)
{
	const char *filename = "../log/imm.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: imm_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

void temp_log(const char *format, ...)
{
	const char *filename = "../log/trig.log";
	static FILE *file = 0;
	if (!file)
	{
		file = fopen(filename, "a");
		if (!file)
		{
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	}

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
}

void err_log(const char *format, ...)
{
	static char buf_[MAX_RAW_INPUT_LENGTH];
	int cnt = snprintf(buf_, sizeof(buf_), "SYSERROR: ");

	va_list args;
	va_start(args, format);
	vsnprintf(buf_ + cnt, sizeof(buf_) - cnt, format, args);
	va_end(args);

	mudlog(buf_, DEF, LVL_IMMORT, SYSLOG, TRUE);
}

/**
* Файл персонального лога терь открывается один раз за каждый вход плеера в игру.
* Дескриптор открытого файла у плеера же и хранится (закрывает при con_close).
*/
void pers_log(CHAR_DATA *ch, const char *format, ...)
{
	if (!ch)
	{
		log("NULL character resieved! (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (!format)
		format = "SYSERR: pers_log received a NULL format.";

	if (!ch->desc->pers_log)
	{
		char filename[128], name[64], *ptr;
		strcpy(name, GET_NAME(ch));
		for (ptr = name; *ptr; ptr++)
			*ptr = LOWER(AtoL(*ptr));
		sprintf(filename, "../log/perslog/%s.log", name);
		ch->desc->pers_log = fopen(filename, "a");
		if (!ch->desc->pers_log)
		{
			log("SYSERR: error open %s (%s %s %d)", filename, __FILE__, __func__, __LINE__);
			return;
		}
		opened_files.push_back(ch->desc->pers_log);
	}

	write_time(ch->desc->pers_log);
	va_list args;
	va_start(args, format);
	vfprintf(ch->desc->pers_log, format, args);
	va_end(args);
	fprintf(ch->desc->pers_log, "\n");
}


void ip_log(const char *ip)
{
	FILE *iplog;

	if (!(iplog = fopen("../log/ip.log", "a")))
	{
		log("SYSERR: ../log/ip.log");
		return;
	}

	fprintf(iplog, "%s\n", ip);
	fclose(iplog);
}

// the "touch" command, essentially.
int touch(const char *path)
{
	FILE *fl;

	if (!(fl = fopen(path, "a")))
	{
		log("SYSERR: %s: %s", path, strerror(errno));
		return (-1);
	}
	else
	{
		fclose(fl);
		return (0);
	}
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 * file - номер файла для вывода (0..NLOG), -1 не выводить в файл
 */
void mudlog(const char *str, int type, int level, int channel, int file)
{
	char tmpbuf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *i;

	if (str == NULL)
		return;		// eh, oh well.
	if (channel < 0 || channel >= NLOG)
		return;
	if (file)
	{
		logfile = logs[channel].logfile;
		log("%s", str);
		logfile = logs[SYSLOG].logfile;
	}
	if (level < 0)
		return;
	char time_buf[20];
        time_t ct = time(0);
        strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	sprintf(tmpbuf, "[%s][ %s ]\r\n", time_buf, str);
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING || IS_NPC(i->character))	// switch
			continue;
		if (GET_LOGS(i->character)[channel] < type && type != DEF)
			continue;
		if (type == DEF && GET_LEVEL(i->character) < LVL_IMMORT && !PRF_FLAGGED(i->character, PRF_CODERINFO))
			continue;
		if (GET_LEVEL(i->character) < level && !PRF_FLAGGED(i->character, PRF_CODERINFO))
			continue;
		if (PLR_FLAGGED(i->character, PLR_WRITING) || PLR_FLAGGED(i->character, PLR_FROZEN))
			continue;

		send_to_char(CCGRN(i->character, C_NRM), i->character);
		send_to_char(tmpbuf, i->character);
		send_to_char(CCNRM(i->character, C_NRM), i->character);
	}
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
const char *empty_string = "ничего";

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag)
{
	long nr = 0, fail = 0, divider = FALSE;
	int plane = 0;
	char c='a';

	*result = '\0';

	if (bitvector < static_cast<uint32_t>(INT_ONE));
	else if (bitvector < static_cast<uint32_t>(INT_TWO))
		fail = 1;
	else if (bitvector < static_cast<uint32_t>(INT_THREE))
		fail = 2;
	else
		fail = 3;
	bitvector &= 0x3FFFFFFF;
	while (fail)
	{
		if (*names[nr] == '\n')
		{
			fail--;
			plane++;
		}
		nr++;
	}

	for (; bitvector; bitvector >>= 1)
	{
		if (IS_SET(bitvector, 1))
		{
			if (*names[nr] != '\n')
			{
				if (print_flag == 1)
					sprintf(result + strlen(result), "%c%d:", c, plane);
				if ((print_flag == 2) && (!strcmp(names[nr], "UNUSED")))
					sprintf(result + strlen(result), "%ld:", nr + 1);
				strcat(result, names[nr]);
				strcat(result, div);
				divider = TRUE;
			}
			else
			{
				if (print_flag == 2)
					sprintf(result + strlen(result), "%ld:", nr + 1);
				else if (print_flag == 1)
					sprintf(result + strlen(result), "%c%d:", c, plane);
				strcat(result, "UNDEF");
				strcat(result, div);
				divider = TRUE;
			}
		}
		if (print_flag == 1)
		{
			c++;
			if(c > 'z')
				c = 'A';
		}
		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result)
	{
		strcat(result, empty_string);
		return false;
	}
	else if (divider)
		*(result + strlen(result) - 1) = '\0';

	return true;
}

bool sprintbit(bitvector_t bitvector, const char *names[], char *result, const int print_flag)
{
	return sprintbitwd(bitvector, names, result, ",", print_flag);
}

bool sprintbits(FLAG_DATA flags, const char *names[], char *result, const char *div, const int print_flag)
{
	bool have_flags = false;
	char buffer[MAX_STRING_LENGTH];
	int i;
	*result = '\0';
	for (i = 0; i < 4; i++)
	{
		if (sprintbitwd(flags.flags[i] | (i << 30), names, buffer, div, print_flag))
		{
			if (strlen(result))
				strcat(result, div);
			strcat(result, buffer);
			have_flags = true;
		}
	}
	if (!strlen(result))
		strcat(result, buffer);

	return have_flags;
}

void sprinttype(int type, const char *names[], char *result)
{
	int nr = 0;

	while (type && *names[nr] != '\n')
	{
		type--;
		nr++;
	}

	if (*names[nr] != '\n')
		strcpy(result, names[nr]);
	else
		strcpy(result, "UNDEF");
}

// * Calculate the REAL time passed over the last t2-t1 centuries (secs)
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1)
{
	long secs;
	static TIME_INFO_DATA now;

	secs = (long)(t2 - t1);

	now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	// 0..23 hours //
	secs -= SECS_PER_REAL_HOUR * now.hours;

	now.day = (secs / SECS_PER_REAL_DAY);	// 0..34 days  //
	// secs -= SECS_PER_REAL_DAY * now.day; - Not used. //

	now.month = -1;
	now.year = -1;

	return (&now);
}



// Calculate the MUD time passed over the last t2-t1 centuries (secs) //
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1)
{
	long secs;
	static TIME_INFO_DATA now;

	secs = (long)(t2 - t1);

	now.hours = (secs / (SECS_PER_MUD_HOUR * TIME_KOEFF)) % HOURS_PER_DAY;	// 0..23 hours //
	secs -= SECS_PER_MUD_HOUR * TIME_KOEFF * now.hours;

	now.day = (secs / (SECS_PER_MUD_DAY * TIME_KOEFF)) % DAYS_PER_MONTH;	// 0..29 days  //
	secs -= SECS_PER_MUD_DAY * TIME_KOEFF * now.day;

	now.month = (secs / (SECS_PER_MUD_MONTH * TIME_KOEFF)) % MONTHS_PER_YEAR;	// 0..11 months //
	secs -= SECS_PER_MUD_MONTH * TIME_KOEFF * now.month;

	now.year = (secs / (SECS_PER_MUD_YEAR * TIME_KOEFF));	// 0..XX? years //

	return (&now);
}



TIME_INFO_DATA *age(CHAR_DATA * ch)
{
	static TIME_INFO_DATA player_age;

	player_age = *mud_time_passed(time(0), ch->player_data.time.birth);

	player_age.year += 17;	// All players start at 17 //

	return (&player_age);
}


// Check if making CH follow VICTIM will create an illegal //
// Follow "Loop/circle"                                    //
bool circle_follow(CHAR_DATA * ch, CHAR_DATA * victim)
{
	CHAR_DATA *k;

	for (k = victim; k; k = k->master)
	{
		if (k->master == k)
		{
			k->master = NULL;
			return (FALSE);
		}
		if (k == ch)
			return (TRUE);
	}

	return (FALSE);
}

void make_horse(CHAR_DATA * horse, CHAR_DATA * ch)
{
	SET_BIT(AFF_FLAGS(horse, AFF_HORSE), AFF_HORSE);
	add_follower(horse, ch);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_WIMPY), MOB_WIMPY);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_SENTINEL), MOB_SENTINEL);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_HELPER), MOB_HELPER);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_MOUNTING), MOB_MOUNTING);
	REMOVE_BIT(AFF_FLAGS(horse, AFF_TETHERED), AFF_TETHERED);
}

int on_horse(CHAR_DATA * ch)
{
	return (AFF_FLAGGED(ch, AFF_HORSE) && has_horse(ch, TRUE));
}

int has_horse(CHAR_DATA * ch, int same_room)
{
	struct follow_type *f;

	if (IS_NPC(ch))
		return (FALSE);

	for (f = ch->followers; f; f = f->next)
	{
		if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_HORSE) &&
				(!same_room || IN_ROOM(ch) == IN_ROOM(f->follower)))
			return (TRUE);
	}
	return (FALSE);
}

CHAR_DATA *get_horse(CHAR_DATA * ch)
{
	struct follow_type *f;

	if (IS_NPC(ch))
		return (NULL);

	for (f = ch->followers; f; f = f->next)
	{
		if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_HORSE))
			return (f->follower);
	}
	return (NULL);
}

void horse_drop(CHAR_DATA * ch)
{
	if (ch->master)
	{
		act("$N сбросил$G вас со своей спины.", FALSE, ch->master, 0, ch, TO_CHAR);
		REMOVE_BIT(AFF_FLAGS(ch->master, AFF_HORSE), AFF_HORSE);
		WAIT_STATE(ch->master, 3 * PULSE_VIOLENCE);
		if (GET_POS(ch->master) > POS_SITTING)
			GET_POS(ch->master) = POS_SITTING;
	}
}

void check_horse(CHAR_DATA * ch)
{
	if (!IS_NPC(ch) && !has_horse(ch, TRUE))
		REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
}


// Called when stop following persons, or stopping charm //
// This will NOT do if a character quits/dies!!          //
// При возврате 1 использовать ch нельзя, т.к. прошли через extract_char
// TODO: по всем вызовам не проходил, может еще где-то коряво вызывается, кроме передачи скакунов -- Krodo
// при персонаже на входе - пуржить не должно полюбому, если начнет, как минимум в change_leader будут глюки
bool stop_follower(CHAR_DATA * ch, int mode)
{
	CHAR_DATA *master;
	struct follow_type *j, *k;
	int i;

	//log("[Stop follower] Start function(%s->%s)",ch ? GET_NAME(ch) : "none",
	//      ch->master ? GET_NAME(ch->master) : "none");

	if (!ch->master)
	{
		log("SYSERR: stop_follower(%s) without master", GET_NAME(ch));
		// core_dump();
		return (FALSE);
	}

	// для смены лидера без лишнего спама
	if (!IS_SET(mode, SF_SILENCE))
	{
		act("Вы прекратили следовать за $N4.", FALSE, ch, 0, ch->master, TO_CHAR);
		act("$n прекратил$g следовать за $N4.", TRUE, ch, 0, ch->master, TO_NOTVICT | TO_ARENA_LISTEN);
	}

	//log("[Stop follower] Stop horse");
	if (get_horse(ch->master) == ch && on_horse(ch->master))
		horse_drop(ch);
	else
		act("$n прекратил$g следовать за вами.", TRUE, ch, 0, ch->master, TO_VICT);

	//log("[Stop follower] Remove from followers list");
	if (!ch->master->followers)
		log("[Stop follower] SYSERR: Followers absent for %s (master %s).", GET_NAME(ch), GET_NAME(ch->master));
	else if (ch->master->followers->follower == ch)  	// Head of follower-list?
	{
		k = ch->master->followers;
		if (!(ch->master->followers = k->next) && !ch->master->master)
			REMOVE_BIT(AFF_FLAGS(ch->master, AFF_GROUP), AFF_GROUP);
		free(k);
	}
	else  		// locate follower who is not head of list
	{
		for (k = ch->master->followers; k->next && k->next->follower != ch; k = k->next);
		if (!k->next)
			log("[Stop follower] SYSERR: Undefined %s in %s followers list.",
				GET_NAME(ch), GET_NAME(ch->master));
		else
		{
			j = k->next;
			k->next = j->next;
			free(j);
		}
	}
	master = ch->master;
	ch->master = NULL;

	REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
	if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_HORSE))
		REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);

	//log("[Stop follower] Free charmee");
	if (AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HELPER) || IS_SET(mode, SF_CHARMLOST))
	{
		if (affected_by_spell(ch, SPELL_CHARM))
			affect_from_char(ch, SPELL_CHARM);
		EXTRACT_TIMER(ch) = 5;
		REMOVE_BIT(AFF_FLAGS(ch, AFF_CHARM), AFF_CHARM);
		// log("[Stop follower] Stop fight charmee");
		if (ch->get_fighting())
			stop_fighting(ch, TRUE);
		/*
		   log("[Stop follower] Stop fight charmee opponee");
		   for (vict = world[IN_ROOM(ch)]->people; vict; vict = vict->next)
		   {if (vict->get_fighting() &&
		   vict->get_fighting() == ch &&
		   ch->get_fighting() != vict)
		   stop_vict->get_fighting();
		   }
		 */
		//log("[Stop follower] Charmee MOB reaction");
		if (IS_NPC(ch))
		{
			if (MOB_FLAGGED(ch, MOB_CORPSE))
			{
				act("Налетевший ветер развеял $n3, не оставив и следа.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				GET_LASTROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
				perform_drop_gold(ch, ch->get_gold(), SCMD_DROP, 0);
				ch->set_gold(0);
				extract_char(ch, FALSE);
				return (TRUE);
			}
			else if (AFF_FLAGGED(ch, AFF_HELPER))
				REMOVE_BIT(AFF_FLAGS(ch, AFF_HELPER), AFF_HELPER);
			else
			{
				if (master &&
						!IS_SET(mode, SF_MASTERDIE) &&
						IN_ROOM(ch) == IN_ROOM(master) &&
						CAN_SEE(ch, master) && !ch->get_fighting() &&
						!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))   //Polud - ну не надо агрить в мирках, незачем это
				{
					if (number(1, GET_REAL_INT(ch) * 2) > GET_REAL_CHA(master))
					{
						act("$n посчитал$g, что вы заслуживаете смерти!",
							FALSE, ch, 0, master, TO_VICT | CHECK_DEAF);
						act("$n заорал$g : \"Ты долго водил$G меня за нос, но дальше так не пойдет!\""
						    "              \"Теперь только твоя смерть может искупить твой обман!!!\"",
						    TRUE, ch, 0, master, TO_NOTVICT | CHECK_DEAF);
						set_fighting(ch, master);
					}
				}
				else
					if (master &&
							!IS_SET(mode, SF_MASTERDIE) &&
							CAN_SEE(ch, master) && MOB_FLAGGED(ch, MOB_MEMORY))
						remember(ch, master);
			}
		}
	}
	//log("[Stop follower] Restore mob flags");
	if (IS_NPC(ch) && (i = GET_MOB_RNUM(ch)) >= 0)
	{
		MOB_FLAGS(ch, INT_ZERRO) = MOB_FLAGS(mob_proto + i, INT_ZERRO);
		MOB_FLAGS(ch, INT_ONE) = MOB_FLAGS(mob_proto + i, INT_ONE);
		MOB_FLAGS(ch, INT_TWO) = MOB_FLAGS(mob_proto + i, INT_TWO);
		MOB_FLAGS(ch, INT_THREE) = MOB_FLAGS(mob_proto + i, INT_THREE);
	}
	//log("[Stop follower] Stop function");
	return (FALSE);
}



// * Called when a character that follows/is followed dies
bool die_follower(CHAR_DATA * ch)
{
	struct follow_type *j, *k = ch->followers;

	if (ch->master && stop_follower(ch, SF_FOLLOWERDIE))
	{
		//  чармиса спуржили в stop_follower
		return true;
	}

	if (on_horse(ch))
		REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);

	for (k = ch->followers; k; k = j)
	{
		j = k->next;
		stop_follower(k->follower, SF_MASTERDIE);
	}
	return false;
}



/** Do NOT call this before having checked if a circle of followers
* will arise. CH will follow leader
* \param silence - для смены лидера группы без лишнего спама (по дефолту 0)
*/
void add_follower(CHAR_DATA * ch, CHAR_DATA * leader, bool silence)
{
	struct follow_type *k;

	if (ch->master)
	{
		log("SYSERR: add_follower(%s->%s) when master existing(%s)...",
			GET_NAME(ch), leader ? GET_NAME(leader) : "", GET_NAME(ch->master));
		// core_dump();
		return;
	}

	if (ch == leader)
		return;

	ch->master = leader;

	CREATE(k, struct follow_type, 1);

	k->follower = ch;
	k->next = leader->followers;
	leader->followers = k;

	if (!IS_HORSE(ch) && !silence)
	{
		act("Вы начали следовать за $N4.", FALSE, ch, 0, leader, TO_CHAR);
		//if (CAN_SEE(leader, ch))
		act("$n начал$g следовать за вами.", TRUE, ch, 0, leader, TO_VICT);
		act("$n начал$g следовать за $N4.", TRUE, ch, 0, leader, TO_NOTVICT | TO_ARENA_LISTEN);
	}
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
	char temp[256];
	int lines = 0;

	do
	{
		fgets(temp, 256, fl);
		if (feof(fl))
			return (0);
		lines++;
	}
	while (*temp == '*' || *temp == '\n');

	temp[strlen(temp) - 1] = '\0';
	strcpy(buf, temp);
	return (lines);
}


int get_filename(const char *orig_name, char *filename, int mode)
{
	const char *prefix, *middle, *suffix;
	char name[64], *ptr;

	if (orig_name == NULL || *orig_name == '\0' || filename == NULL)
	{
		log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.", orig_name, filename);
		return (0);
	}

	switch (mode)
	{
	case ALIAS_FILE:
		prefix = LIB_PLRALIAS;
		suffix = SUF_ALIAS;
		break;
	case SCRIPT_VARS_FILE:
		prefix = LIB_PLRVARS;
		suffix = SUF_MEM;
		break;
	case PLAYERS_FILE:
		prefix = LIB_PLRS;
		suffix = SUF_PLAYER;
		break;
	case TEXT_CRASH_FILE:
		prefix = LIB_PLROBJS;
		suffix = TEXT_SUF_OBJS;
		break;
	case TIME_CRASH_FILE:
		prefix = LIB_PLROBJS;
		suffix = TIME_SUF_OBJS;
		break;
	case PERS_DEPOT_FILE:
		prefix = LIB_DEPOT;
		suffix = SUF_PERS_DEPOT;
		break;
	case SHARE_DEPOT_FILE:
		prefix = LIB_DEPOT;
		suffix = SUF_SHARE_DEPOT;
		break;
	case PURGE_DEPOT_FILE:
		prefix = LIB_DEPOT;
		suffix = SUF_PURGE_DEPOT;
		break;
	default:
		return (0);
	}

	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++)
	{
		if (*ptr == 'Ё' || *ptr == 'ё')
			*ptr = '9';
		else
			*ptr = LOWER(AtoL(*ptr));
	}

	switch (LOWER(*name))
	{
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
		middle = LIB_A;
		break;
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
		middle = LIB_F;
		break;
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
		middle = LIB_K;
		break;
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
		middle = LIB_P;
		break;
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
		middle = LIB_U;
		break;
	default:
		middle = LIB_Z;
		break;
	}

	sprintf(filename, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
	return (1);
}


int num_pc_in_room(ROOM_DATA * room)
{
	int i = 0;
	CHAR_DATA *ch;

	for (ch = room->people; ch != NULL; ch = ch->next_in_room)
		if (!IS_NPC(ch))
			i++;

	return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_real(const char *who, int line)
{
	log("SYSERR: Assertion failed at %s:%d!", who, line);

#if defined(CIRCLE_UNIX)
	// These would be duplicated otherwise...
	fflush(stdout);
	fflush(stderr);
	for (int i = 0; i < NLOG; ++i)
		fflush(logs[i].logfile);

	/*
	 * Kill the child so the debugger or script doesn't think the MUD
	 * crashed.  The 'autorun' script would otherwise run it again.
	 */
	if (fork() == 0)
		abort();
#endif
}

void to_koi(char *str, int from)
{
	switch (from)
	{
	case KT_ALT:
		for (; *str; *str = AtoK(*str), str++);
		break;
	case KT_WINZ:
	case KT_WIN:
		for (; *str; *str = WtoK(*str), str++);
		break;
	}
}

void from_koi(char *str, int from)
{
	switch (from)
	{
	case KT_ALT:
		for (; *str; *str = KtoA(*str), str++);
		break;
	case KT_WIN:
		for (; *str; *str = KtoW(*str), str++);
	case KT_WINZ:
		for (; *str; *str = KtoW2(*str), str++);
		break;
	}
}

void koi_to_win(char *str, int size)
{
	for (; size > 0; *str = KtoW(*str), size--, str++);
}

void koi_to_winz(char *str, int size)
{
	for (; size > 0; *str = KtoW2(*str), size--, str++);
}

void koi_to_alt(char *str, int size)
{
	for (; size > 0; *str = KtoA(*str), size--, str++);
}

// string manipulation fucntion originally by Darren Wilson //
// (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) //
// completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), //
// substitute appearances of 'pattern' with 'replacement' in string //
// and return the # of replacements //
int replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size)
{
	char *replace_buffer = NULL;
	char *flow, *jetsam, temp;
	int len, i;

	if ((signed)((strlen(*string) - strlen(pattern)) + strlen(replacement))
			> max_size)
		return -1;

	CREATE(replace_buffer, char, max_size);
	i = 0;
	jetsam = *string;
	flow = *string;
	*replace_buffer = '\0';
	if (rep_all)
	{
		while ((flow = (char *) strstr(flow, pattern)) != NULL)
		{
			i++;
			temp = *flow;
			*flow = '\0';
			if ((signed)(strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size)
			{
				i = -1;
				break;
			}
			strcat(replace_buffer, jetsam);
			strcat(replace_buffer, replacement);
			*flow = temp;
			flow += strlen(pattern);
			jetsam = flow;
		}
		strcat(replace_buffer, jetsam);
	}
	else
	{
		if ((flow = (char *) strstr(*string, pattern)) != NULL)
		{
			i++;
			flow += strlen(pattern);
			len = ((char *) flow - (char *) * string) - strlen(pattern);

			strncpy(replace_buffer, *string, len);
			strcat(replace_buffer, replacement);
			strcat(replace_buffer, flow);
		}
	}
	if (i == 0)
		return 0;
	if (i > 0)
	{
		RECREATE(*string, char, strlen(replace_buffer) + 3);
		strcpy(*string, replace_buffer);
	}
	free(replace_buffer);
	return i;
}


// re-formats message type formatted char * //
// (for strings edited with d->str) (mostly olc and mail)     //
void format_text(char **ptr_string, int mode, DESCRIPTOR_DATA * d, int maxlen)
{
	int total_chars, cap_next = TRUE, cap_next_next = FALSE;
	char *flow, *start = NULL, temp;
	// warning: do not edit messages with max_str's of over this value //
	char formated[MAX_STRING_LENGTH];

	flow = *ptr_string;
	if (!flow)
		return;

	if (IS_SET(mode, FORMAT_INDENT))
	{
		strcpy(formated, "   ");
		total_chars = 3;
	}
	else
	{
		*formated = '\0';
		total_chars = 0;
	}

	while (*flow != '\0')
	{
		while ((*flow == '\n') ||
				(*flow == '\r') || (*flow == '\f') || (*flow == '\t') || (*flow == '\v') || (*flow == ' '))
			flow++;

		if (*flow != '\0')
		{
			start = flow++;
			while ((*flow != '\0') &&
					(*flow != '\n') &&
					(*flow != '\r') &&
					(*flow != '\f') &&
					(*flow != '\t') &&
					(*flow != '\v') && (*flow != ' ') && (*flow != '.') && (*flow != '?') && (*flow != '!'))
				flow++;

			if (cap_next_next)
			{
				cap_next_next = FALSE;
				cap_next = TRUE;
			}

			// this is so that if we stopped on a sentance .. we move off the sentance delim. //
			while ((*flow == '.') || (*flow == '!') || (*flow == '?'))
			{
				cap_next_next = TRUE;
				flow++;
			}

			temp = *flow;
			*flow = '\0';

			if ((total_chars + strlen(start) + 1) > 79)
			{
				strcat(formated, "\r\n");
				total_chars = 0;
			}

			if (!cap_next)
			{
				if (total_chars > 0)
				{
					strcat(formated, " ");
					total_chars++;
				}
			}
			else
			{
				cap_next = FALSE;
				*start = UPPER(*start);
			}

			total_chars += strlen(start);
			strcat(formated, start);

			*flow = temp;
		}

		if (cap_next_next)
		{
			if ((total_chars + 3) > 79)
			{
				strcat(formated, "\r\n");
				total_chars = 0;
			}
			else
			{
				strcat(formated, " ");
				total_chars += 2;
			}
		}
	}
	strcat(formated, "\r\n");

	if ((signed) strlen(formated) > maxlen)
		formated[maxlen] = '\0';
	RECREATE(*ptr_string, char, MIN(maxlen, strlen(formated) + 3));
	strcpy(*ptr_string, formated);
}


const char *some_pads[3][37] =
{
	{"дней", "часов", "лет", "очков", "минут", "минут", "кун", "кун", "штук", "штук", "уровней", "верст", "верст", "единиц", "единиц", "секунд", "градусов", "строк", "предметов", "предметов", "перевоплощений", "недель", "месяцев", "недель", "славы", "славы", "человек", "силы", "глотков", "гривен", "золотых", "серебряных", "бронзовых", "гривен", "золотых", "серебряных", "бронзовых"},
	{"день", "час", "год", "очко", "минута", "минуту", "куна", "куну", "штука", "штуку", "уровень", "верста", "версту", "единица", "единицу", "секунду", "градус", "строка", "предмет", "предмета", "перевоплощение", "неделя", "месяц", "неделю", "слава", "славу", "человек", "сила", "глоток", "гривна", "золотая", "серебряная", "бронзовая", "гривну", "золотую", "серебряную", "бронзовую"},
	{"дня", "часа", "года", "очка", "минуты", "минуты", "куны", "куны", "штуки", "штуки", "уровня", "версты", "версты", "единицы", "единицы", "секунды", "градуса", "строки", "предмета", "предметов", "перевоплощения", "недели", "месяца", "недели", "славы", "славы", "человека", "силы", "глотка", "гривны", "золотые", "серебряные", "бронзовые", "гривны", "золотые", "серебряные", "бронзовые"}
};

const char * desc_count(long how_many, int of_what)
{
	if (how_many < 0)
		how_many = -how_many;
	if ((how_many % 100 >= 11 && how_many % 100 <= 14) || how_many % 10 >= 5 || how_many % 10 == 0)
		return some_pads[0][of_what];
	if (how_many % 10 == 1)
		return some_pads[1][of_what];
	else
		return some_pads[2][of_what];
}

int check_moves(CHAR_DATA * ch, int how_moves)
{
	if (IS_IMMORTAL(ch) || IS_NPC(ch))
		return (TRUE);
	if (GET_MOVE(ch) < how_moves)
	{
		send_to_char("Вы слишком устали.\r\n", ch);
		return (FALSE);
	}
	GET_MOVE(ch) -= how_moves;
	return (TRUE);
}

int real_sector(int room)
{
	int sector = SECT(room);

	if (ROOM_FLAGGED(room, ROOM_NOWEATHER))
		return sector;
	switch (sector)
	{
	case SECT_INSIDE:
	case SECT_CITY:
	case SECT_FLYING:
	case SECT_UNDERWATER:
	case SECT_SECRET:
	case SECT_STONEROAD:
	case SECT_ROAD:
	case SECT_WILDROAD:
		return sector;
		break;
	case SECT_FIELD:
		if (world[room]->weather.snowlevel > 20)
			return SECT_FIELD_SNOW;
		else if (world[room]->weather.rainlevel > 20)
			return SECT_FIELD_RAIN;
		else
			return SECT_FIELD;
		break;
	case SECT_FOREST:
		if (world[room]->weather.snowlevel > 20)
			return SECT_FOREST_SNOW;
		else if (world[room]->weather.rainlevel > 20)
			return SECT_FOREST_RAIN;
		else
			return SECT_FOREST;
		break;
	case SECT_HILLS:
		if (world[room]->weather.snowlevel > 20)
			return SECT_HILLS_SNOW;
		else if (world[room]->weather.rainlevel > 20)
			return SECT_HILLS_RAIN;
		else
			return SECT_HILLS;
		break;
	case SECT_MOUNTAIN:
		if (world[room]->weather.snowlevel > 20)
			return SECT_MOUNTAIN_SNOW;
		else
			return SECT_MOUNTAIN;
		break;
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
		if (world[room]->weather.icelevel > 30)
			return SECT_THICK_ICE;
		else if (world[room]->weather.icelevel > 20)
			return SECT_NORMAL_ICE;
		else if (world[room]->weather.icelevel > 10)
			return SECT_THIN_ICE;
		else
			return sector;
		break;
	}
	return SECT_INSIDE;
}

bool same_group(CHAR_DATA * ch, CHAR_DATA * tch)
{
	if (!ch || !tch)
		return false;

	// Добавлены проверки чтобы не любой заследовавшийся моб считался согруппником (Купала)
	if (IS_NPC(ch) && ch->master && !IS_NPC(ch->master)
		&& (IS_HORSE(ch) || AFF_FLAGGED(ch, AFF_CHARM) || MOB_FLAGGED(ch, MOB_ANGEL)))
		ch = ch->master;
	if (IS_NPC(tch) && tch->master && !IS_NPC(tch->master)
		&& (IS_HORSE(tch) || AFF_FLAGGED(tch, AFF_CHARM) || MOB_FLAGGED(tch, MOB_ANGEL)))
		tch = tch->master;

	// NPC's always in same group
	if ((IS_NPC(ch) && IS_NPC(tch)) || ch == tch)
		return true;

	if (!AFF_FLAGGED(ch, AFF_GROUP) || !AFF_FLAGGED(tch, AFF_GROUP))
		return false;

	if (ch->master == tch || tch->master == ch || (ch->master && ch->master == tch->master))
		return true;

	return false;
}

// Проверка является комната рентой.
bool is_rent(room_rnum room)
{
	// комната с флагом замок, но клан мертвый
	if (ROOM_FLAGGED(room, ROOM_HOUSE))
	{
		ClanListType::const_iterator it = Clan::IsClanRoom(room);
		if (Clan::ClanList.end() == it)
		{
			return false;
		}
	}
	// комната без рентера в ней
	for (CHAR_DATA *ch = world[room]->people; ch; ch = ch->next_in_room)
	{
		if (IS_NPC(ch) && IS_RENTKEEPER(ch))
		{
			return true;
		}
	}
	return false;
}

// Проверка является комната почтой.
int is_post(room_rnum room)
{
	CHAR_DATA *ch;
	for (ch = world[room]->people; ch; ch = ch->next_in_room)
		if (IS_NPC(ch) && IS_POSTKEEPER(ch))
			return (TRUE);
	return (FALSE);

}

// Форматирование вывода в соответствии с форматом act-a
// output act format//
char *format_act(const char *orig, CHAR_DATA * ch, OBJ_DATA * obj, const void *vict_obj)
{
	const char *i = NULL;
	char *buf, *lbuf;
	ubyte padis;
	int stopbyte;
//	CHAR_DATA *dg_victim = NULL;

	buf = (char *) malloc(MAX_STRING_LENGTH);
	lbuf = buf;

	for (stopbyte = 0; stopbyte < MAX_STRING_LENGTH; stopbyte++)
	{
		if (*orig == '$')
		{
			switch (*(++orig))
			{
			case 'n':
				if (*(orig + 1) < '0' || *(orig + 1) > '5')
					i = GET_PAD(ch, 0);
				else
				{
					padis = *(++orig) - '0';
					i = GET_PAD(ch, padis);
				}
				break;
			case 'N':
				if (*(orig + 1) < '0' || *(orig + 1) > '5')
				{
					CHECK_NULL(vict_obj, GET_PAD((const CHAR_DATA *) vict_obj, 0));
				}
				else
				{
					padis = *(++orig) - '0';
					CHECK_NULL(vict_obj, GET_PAD((const CHAR_DATA *) vict_obj, padis));
				}
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'm':
				i = HMHR(ch);
				break;
			case 'M':
				if (vict_obj)
					i = HMHR((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OMHR(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 's':
				i = HSHR(ch);
				break;
			case 'S':
				if (vict_obj)
					i = HSHR((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OSHR(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'e':
				i = HSSH(ch);
				break;
			case 'E':
				if (vict_obj)
					i = HSSH((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OSSH(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'o':
				if (*(orig + 1) < '0' || *(orig + 1) > '5')
				{
					CHECK_NULL(obj, OBJ_PAD(obj, 0));
				}
				else
				{
					padis = *(++orig) - '0';
					CHECK_NULL(obj, OBJ_PAD(obj, padis > 5 ? 0 : padis));
				}
				break;
			case 'O':
				if (*(orig + 1) < '0' || *(orig + 1) > '5')
				{
					CHECK_NULL(vict_obj, OBJ_PAD((const OBJ_DATA *) vict_obj, 0));
				}
				else
				{
					padis = *(++orig) - '0';
					CHECK_NULL(vict_obj,
							   OBJ_PAD((const OBJ_DATA *) vict_obj, padis > 5 ? 0 : padis));
				}
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

				/*            case 'p':
				                 CHECK_NULL(obj, OBJS(obj, to));
				                 break;
				            case 'P':
				                 CHECK_NULL(vict_obj, OBJS((const OBJ_DATA *) vict_obj, to));
				                 dg_victim = (CHAR_DATA *) vict_obj;
				                 break;
				*/
			case 't':
				CHECK_NULL(obj, (const char *) obj);
				break;

			case 'T':
				CHECK_NULL(vict_obj, (const char *) vict_obj);
				break;

			case 'F':
				CHECK_NULL(vict_obj, (const char *) vict_obj);
				break;

			case '$':
				i = "$";
				break;

			case 'a':
				i = GET_CH_SUF_6(ch);
				break;
			case 'A':
				if (vict_obj)
					i = GET_CH_SUF_6((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_6(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'g':
				i = GET_CH_SUF_1(ch);
				break;
			case 'G':
				if (vict_obj)
					i = GET_CH_SUF_1((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_1(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'y':
				i = GET_CH_SUF_5(ch);
				break;
			case 'Y':
				if (vict_obj)
					i = GET_CH_SUF_5((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_5(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'u':
				i = GET_CH_SUF_2(ch);
				break;
			case 'U':
				if (vict_obj)
					i = GET_CH_SUF_2((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_2(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'w':
				i = GET_CH_SUF_3(ch);
				break;
			case 'W':
				if (vict_obj)
					i = GET_CH_SUF_3((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_3(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'q':
				i = GET_CH_SUF_4(ch);
				break;
			case 'Q':
				if (vict_obj)
					i = GET_CH_SUF_4((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_4(obj));
				//dg_victim = (CHAR_DATA *) vict_obj;
				break;
//Polud Добавил склонение местоимения ваш(е,а,и)
			case 'z':
				if (obj)
					i = OYOU(obj);
				else
					CHECK_NULL(obj, OYOU(obj));
				break;
			case 'Z':
				if (vict_obj)
					i = HYOU((const CHAR_DATA *)vict_obj);
				else
					CHECK_NULL(vict_obj, HYOU((const CHAR_DATA *)vict_obj));
				break;
//-Polud
			default:
				log("SYSERR: Illegal $-code to act(): %c", *orig);
				log("SYSERR: %s", orig);
				i = "";
				break;
			}
			while ((*buf = *(i++)))
				buf++;
			orig++;
		}
		else if (*orig == '\\')
		{
			if (*(orig + 1) == 'r')
			{
				*(buf++) = '\r';
				orig += 2;
			}
			else if (*(orig + 1) == 'n')
			{
				*(buf++) = '\n';
				orig += 2;
			}
			else
				*(buf++) = *(orig++);
		}
		else if (!(*(buf++) = *(orig++)))
			break;
	}

	*(--buf) = '\r';
	*(++buf) = '\n';
	*(++buf) = '\0';
	return (lbuf);
}

char *rustime(const struct tm *timeptr)
{
	static char mon_name[12][10] =
	{
		"Января\0", "Февраля\0", "Марта\0", "Апреля\0", "Мая\0", "Июня\0",
		"Июля\0", "Августа\0", "Сентября\0", "Октября\0", "Ноября\0", "Декабря\0"
	};
	static char result[100];

	sprintf(result, "%.2d:%.2d:%.2d %2d %s %d года",
			timeptr->tm_hour,
			timeptr->tm_min, timeptr->tm_sec, timeptr->tm_mday, mon_name[timeptr->tm_mon], 1900 + timeptr->tm_year);
	return result;
}

int roundup(float fl)
{
	if ((int) fl < fl)
		return (int) fl + 1;
	else
		return (int)fl;
}

// Функция проверяет может ли ch нести предмет obj и загружает предмет
// в инвентарь игрока или в комнату, где игрок находится
void can_carry_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
	{
		send_to_char("Вы не можете нести столько предметов.", ch);
		obj_to_room(obj, IN_ROOM(ch));
		obj_decay(obj);
	}
	else
	{
		if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(ch) > CAN_CARRY_W(ch))
		{
			sprintf(buf, "Вам слишком тяжело нести еще и %s.", obj->PNames[3]);
			send_to_char(buf, ch);
			obj_to_room(obj, IN_ROOM(ch));
			// obj_decay(obj);
		}
		else
			obj_to_char(obj, ch);
	}
}

/**
 * Бывшее #define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
 */
bool CAN_CARRY_OBJ(CHAR_DATA *ch, OBJ_DATA *obj)
{
	// для анлимного лута мобами из трупов
	if (IS_NPC(ch) && !IS_CHARMICE(ch))
	{
		return true;
	}

	if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)
		&& IS_CARRYING_N(ch) + 1 <= CAN_CARRY_N(ch))
	{
		return true;
	}

	return false;
}

// shapirus: проверка, игнорирет ли чар who чара whom
bool ignores(CHAR_DATA * who, CHAR_DATA * whom, unsigned int flag)
{
	if (IS_NPC(who)) return false;

	long ign_id;
	struct ignore_data *ignore = IGNORE_LIST(who);

// имморталов не игнорит никто
	if (IS_IMMORTAL(whom))
		return FALSE;

// чармисы игнорируемого хозяина тоже должны быть проигнорированы
	if (IS_NPC(whom) && AFF_FLAGGED(whom, AFF_CHARM))
		return ignores(who, whom->master, flag);

	ign_id = GET_IDNUM(whom);
	for (; ignore; ignore = ignore->next)
	{
		if ((ignore->id == ign_id || ignore->id == -1) && IS_SET(ignore->mode, flag))
			return TRUE;
	}
	return FALSE;
}

//Gorrah
int valid_email(const char *address)
{
	int i, size, count = 0;
	static string special_symbols("\r\n ()<>,;:\\\"[]|/&'`$");
	string addr = address;
	string::size_type dog_pos = 0, pos = 0;

	// Наличие запрещенных символов или кириллицы //
	if (addr.find_first_of(special_symbols) != string::npos)
		return 0;
	size = 	addr.size();
	for (i = 0; i < size; i++)
		if (addr[i] <= ' ' || addr[i] >= 127)
			return 0;
	// Собака должна быть только одна и на второй и далее позиции //
	while ((pos = addr.find_first_of('@', pos)) != string::npos)
	{
		dog_pos = pos;
		++count;
		++pos;
	}
	if (count != 1 || dog_pos == 0)
		return 0;
	// Проверяем правильность синтаксиса домена //
	// В доменной части должно быть как минимум 4 символа, считая собаку //
	if (size - dog_pos <= 3)
		return 0;
	// Точка отсутствует, расположена сразу после собаки, или на последнем месте //
	if (addr[dog_pos + 1] == '.' || addr[size - 1] == '.' || addr.find('.', dog_pos) == string::npos)
		return 0;

	return 1;
}

/**
* Вывод времени (минут) в зависимости от их кол-ва с округлением вплоть до месяцев.
* Для таймеров славы по 'glory имя' и 'слава информация'.
* \param flag - по дефолту 0 - 1 минута, 1 неделя;
*                          1 - 1 минуту, 1 неделю.
*/
std::string time_format(int in_timer, int flag)
{
	char buffer[256];
	std::ostringstream out;

	double timer = in_timer;
	int one = 0, two = 0;

	if (timer < 60)
		out << timer << " " << desc_count(in_timer, flag ? WHAT_MINu : WHAT_MINa);
	else if (timer < 1440)
	{
		sprintf(buffer, "%.1f", timer / 60);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two  << " " << desc_count(two, WHAT_HOUR);
		else
			out << " " << desc_count(one, WHAT_HOUR);
	}
	else if (timer < 10080)
	{
		sprintf(buffer, "%.1f", timer / 1440);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two  << " " << desc_count(two, WHAT_DAY);
		else
			out << " " << desc_count(one, WHAT_DAY);
	}
	else if (timer < 44640)
	{
		sprintf(buffer, "%.1f", timer / 10080);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two  << " " << desc_count(two, WHAT_WEEK);
		else
			out << " " << desc_count(one, flag ? WHAT_WEEKu : WHAT_WEEK);
	}
	else
	{
		sprintf(buffer, "%.1f", timer / 44640);
		sscanf(buffer, "%d.%d", &one, &two);
		out << one;
		if (two)
			out << "." << two  << " " << desc_count(two, WHAT_MONTH);
		else
			out << " " << desc_count(one, WHAT_MONTH);
	}
	return out.str();
}

// * Для обрезания точек в карме при сете славы.
void skip_dots(char **string)
{
	for (; **string && (strchr(" .", **string) != NULL); (*string)++);
}

// Return pointer to first occurrence in string ct in
// cs, or NULL if not present.  Case insensitive
char *str_str(char *cs, const char *ct)
{
	char *s;
	const char *t;

	if (!cs || !ct)
		return NULL;

	while (*cs)
	{
		t = ct;

		while (*cs && (LOWER(*cs) != LOWER(*t)))
			cs++;

		s = cs;

		while (*t && *cs && (LOWER(*cs) == LOWER(*t)))
		{
			t++;
			cs++;
		}

		if (!*t)
			return s;

	}
	return NULL;
}

// remove ^M's from file output
void kill_ems(char *str)
{
	char *ptr1, *ptr2;

	ptr1 = str;
	ptr2 = str;

	while (*ptr1)
	{
		if ((*(ptr2++) = *(ptr1++)) == '\r')
			if (*ptr1 == '\r')
				ptr1++;
	}
	*ptr2 = '\0';
}

// * Вырезание и перемещение в word одного слова из str (a_isalnum).
void cut_one_word(std::string &str, std::string &word)
{
	if (str.empty())
	{
		word.clear();
		return;
	}

	bool process = false;
	unsigned begin = 0, end = 0;
	for (unsigned i = 0; i < str.size(); ++i)
	{
		if (!process && a_isalnum(str.at(i)))
		{
			process = true;
			begin = i;
			continue;
		}
		if (process && !a_isalnum(str.at(i)))
		{
			end = i;
			break;
		}
	}

	if (process)
	{
		if (!end || end >= str.size())
		{
			word = str.substr(begin);
			str.clear();
		}
		else
		{
			word = str.substr(begin, end - begin);
			str.erase(0, end);
		}
		return;
	}

	str.clear();
	word.clear();
}

/**
 * взято с http://www.openbsd.org/cgi-bin/cvsweb/src/lib/libc/string/strlcpy.c?rev=1.11&content-type=text/x-cvsweb-markup
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strl_cpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	// Copy as many bytes as will fit
	if (n != 0)
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	// Not enough room in dst, add NUL and traverse rest of src
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';		// NUL-terminate dst
		while (*s++)
			;
	}

	return(s - src - 1);	// count does not include NUL
}

/**
* Аналог старого GET_REAL_DR(ch)
* для мобов ограничение в 50 дамролов убрано
* +еще есть рандом дамролы, в данный момент максимум 30d127
*/
int get_real_dr(CHAR_DATA *ch)
{
	int dd = 0;
// Инициализация массива для дальнейшего бонуса попаданий/повреждений от количества перевоплощений	
	int dam[31] = {0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7};
        if (IS_SMITH(ch) || IS_GUARD(ch) || IS_RANGER(ch))  // Бонус имеют только Дружинники, охотники, кузнецы
           dd = dam[GET_REMORT(ch)];
           GET_HR_ADD(ch) += dd;           // Бонус попаданий от количества перевоплощений

        int level = GET_LEVEL(ch);

	int bonus = 0;
	if (IS_NPC(ch) && !IS_CHARMICE(ch))
	{
		if (level > STRONG_MOB_LEVEL)
		    bonus += level + number(0, level);
		return MAX(0, GET_DR(ch) + GET_DR_ADD(ch) + bonus);
	}
	if (can_use_feat(ch, BOWS_FOCUS_FEAT) && ch->get_skill(SKILL_ADDSHOT))
	    return  MAX(0, GET_DR(ch) + GET_DR_ADD(ch) + dd * 2);
        else
            return (VPOSI(GET_DR(ch)+GET_DR_ADD(ch), -50, 50) + dd * 2);
}

// без ограничений
int get_real_dr_new(CHAR_DATA *ch)
{
    return MAX(0, GET_DR(ch) + GET_DR_ADD(ch));
}


////////////////////////////////////////////////////////////////////////////////
// show money

namespace MoneyDropStat
{

typedef std::map<int /* zone vnum */, long /* money count */> ZoneListType;
ZoneListType zone_list;

void add(int zone_vnum, long money)
{
	ZoneListType::iterator i = zone_list.find(zone_vnum);
	if (i != zone_list.end())
	{
		i->second += money;
	}
	else
	{
		zone_list[zone_vnum] = money;
	}
}

void print(CHAR_DATA *ch)
{
	if (!PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char(ch, "Пока в разработке.\r\n");
		return;
	}

	std::map<long, int> tmp_list;
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i)
	{
		if (i->second > 0)
		{
			tmp_list[i->second] = i->first;
		}
	}
	if (!tmp_list.empty())
	{
		send_to_char(ch,
				"Money drop stats:\r\n"
				"Total zones: %d\r\n"
				"  vnum - money\r\n"
				"================\r\n", tmp_list.size());
	}
	else
	{
		send_to_char(ch, "Empty.\r\n");
		return;
	}
	std::stringstream out;
	for (std::map<long, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend; ++i)
	{
		out << "  " << std::setw(4) << i->second << " - " << i->first << "\r\n";
//		send_to_char(ch, "  %4d - %ld\r\n", i->second, i->first);
	}
	page_string(ch->desc, out.str());
}

void print_log()
{
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i)
	{
		if (i->second > 0)
		{
			log("ZoneDrop: %d %ld", i->first, i->second);
		}
	}
}

} // MoneyDropStat

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// show exp
namespace ZoneExpStat
{

struct zone_data
{
	zone_data() : gain(0), lose(0) {};

	unsigned long gain;
	unsigned long lose;
};

typedef std::map<int /* zone vnum */, zone_data> ZoneListType;
ZoneListType zone_list;

void add(int zone_vnum, long exp)
{
	if (!exp)
	{
		return;
	}

	ZoneListType::iterator i = zone_list.find(zone_vnum);
	if (i != zone_list.end())
	{
		if (exp > 0)
		{
			i->second.gain += exp;
		}
		else
		{
			i->second.lose += -exp;
		}
	}
	else
	{
		zone_data tmp_zone;
		if (exp > 0)
		{
			tmp_zone.gain = exp;
		}
		else
		{
			tmp_zone.lose = -exp;
		}
		zone_list[zone_vnum] = tmp_zone;
	}
}

void print_gain(CHAR_DATA *ch)
{
	if (!PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char(ch, "Пока в разработке.\r\n");
		return;
	}

	std::map<unsigned long, int> tmp_list;
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i)
	{
		if (i->second.gain > 0)
		{
			tmp_list[i->second.gain] = i->first;
		}
	}
	if (!tmp_list.empty())
	{
		send_to_char(ch,
				"Gain exp stats:\r\n"
				"Total zones: %d\r\n"
				"  vnum - exp\r\n"
				"================\r\n", tmp_list.size());
	}
	else
	{
		send_to_char(ch, "Empty.\r\n");
		return;
	}
	std::stringstream out;
	for (std::map<unsigned long, int>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend(); i != iend; ++i)
	{
		out << "  " << std::setw(4) << i->second << " - " << i->first << "\r\n";
	}
	page_string(ch->desc, out.str());
}

void print_log()
{
	for (ZoneListType::const_iterator i = zone_list.begin(), iend = zone_list.end(); i != iend; ++i)
	{
		if (i->second.gain > 0 || i->second.lose > 0)
		{
			log("ZoneExp: %d %lu %lu", i->first, i->second.gain, i->second.lose);
		}
	}
}

} // ZoneExpStat
////////////////////////////////////////////////////////////////////////////////

std::string thousands_sep(long long n)
{
	bool negative = false;
	if (n < 0)
	{
		n = -n;
		negative = true;
	}

	int size = 50;
	int curr_pos = size - 1;
	const int comma = ',';
	std::string buffer;
	buffer.resize(size);
	int i = 0;

	try
	{
		do
		{
			if (i % 3 == 0 && i != 0)
			{
				buffer.at(--curr_pos) = comma;
			}
			buffer.at(--curr_pos) = '0' + n % 10;
			n /= 10;
			i++;
		}
		while(n != 0);

		if (negative)
		{
			buffer.at(--curr_pos) = '-';
		}
	}
	catch (...)
	{
		log("SYSERROR : string.at() (%s:%d)", __FILE__, __LINE__);
		return "<OutOfRange>";
	}

	buffer = buffer.substr(curr_pos, size - 1);
	return buffer;
}

int str_bonus(int str, int type)
{
	int bonus = 0;
	str = MAX(1, str);

	switch (type)
	{
	case STR_TO_HIT:
		// -5 ... 10
		if (str < 10)
		{
			bonus = (str - 11) / 2;
		}
		else
		{
			bonus = (str - 10) / 4;
		}
		break;
	case STR_TO_DAM:
		// -5 ... 36
		if (str < 15)
		{
			bonus = (str - 11) / 2;
		}
		else
		{
			bonus = str - 14;
		}
		break;
	case STR_CARRY_W:
		// 50 ... 2500
		bonus = str * 50;
		break;
	case STR_WIELD_W:
		if (str <= 20)
		{
			// w 1 .. 20
			bonus = str;
		}
		else if (str < 30)
		{
			// w 20,5 .. 24,5
			bonus = 20 + (str - 20) / 2;
		}
		else
		{
			// w >= 25
			bonus = 25 + (str - 30) / 4;
		}
		break;
	case STR_HOLD_W:
		if (str <= 29)
		{
			// w 1 .. 15
			bonus = (str + 1) / 2;
		}
		else
		{
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

int dex_bonus(int dex)
{
	int bonus = 0;
	dex = MAX(1, dex);
	// -5 ... 40
	if (dex < 10)
	{
		bonus = (dex - 11) / 2;
	}
	else
	{
		bonus = dex - 10;
	}
	return bonus;
}

int dex_ac_bonus(int dex)
{
	int bonus = 0;
	dex = MAX(1, dex);
	// -3 ... 35
	if (dex <= 15)
	{
		bonus = (dex - 10) / 3;
	}
	else
	{
		bonus = dex - 15;
	}
	return bonus;
}

int wis_bonus(int stat, int type)
{
	int bonus = 0;
	stat = MAX(1, stat);

	switch (type)
	{
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
		if (stat <= 15)
		{
			bonus = stat;
		}
		// 15 .. 32
		else
		{
			bonus = 15 + (stat - 15) / 2;
		}
		break;
	case WIS_FAILS:
		// 34 .. 66
		if (stat <= 9)
		{
			bonus = 30 + stat * 4;
		}
		// 140 .. 940
		else
		{
			bonus = 120 + (stat - 9) * 20;
		}
		break;
	default:
		log("SYSERROR: stat=%d, type=%d (%s %s %d)",
				stat, type, __FILE__, __func__, __LINE__);
	}

	return bonus;
}

int calc_str_req(int weight, int type)
{
	int str = 0;
	weight = MAX(1, weight);

	switch (type)
	{
	case STR_WIELD_W:
		if (weight <= 20)
		{
			str = weight;
		}
		else if (weight <= 24)
		{
			str = 2 * weight - 20;
		}
		else
		{
			str = 4 * weight - 70;
		}
		break;
	case STR_HOLD_W:
		if (weight <= 15)
		{
			str = 2 * weight - 1;
		}
		else
		{
			str = 4 * weight - 31;
		}
		break;
	case STR_BOTH_W:
		if (weight <= 31)
		{
			str = weight - (weight + 1) / 3;
		}
		else if (weight <= 40)
		{
			str = weight - 10;
		}
		else
		{
			str = (weight - 39) * 2 + 29 - (weight + 1) % 2;
		}
		break;
	default:
		log("SYSERROR: weight=%d, type=%d (%s %s %d)",
				weight, type, __FILE__, __func__, __LINE__);
	}

	return str;
}

void message_str_need(CHAR_DATA *ch, OBJ_DATA *obj, int type)
{
	if (GET_POS(ch) == POS_DEAD)
		return;
	int need_str = 0;
	switch (type)
	{
	case STR_WIELD_W:
		need_str = calc_str_req(GET_OBJ_WEIGHT(obj), STR_WIELD_W);
		break;
	case STR_HOLD_W:
		need_str = calc_str_req(GET_OBJ_WEIGHT(obj), STR_HOLD_W);
		break;
	case STR_BOTH_W:
		need_str = calc_str_req(GET_OBJ_WEIGHT(obj), STR_BOTH_W);
		break;
	case STR_SHIELD_W:
		need_str = calc_str_req((GET_OBJ_WEIGHT(obj)+1)/2, STR_HOLD_W);
		break;
	default:
		log("SYSERROR: ch=%s, weight=%d, type=%d (%s %s %d)",
				GET_NAME(ch), GET_OBJ_WEIGHT(obj), type,
				__FILE__, __func__, __LINE__);
		return;
	}
	send_to_char(ch, "Для этого требуется %d %s.\r\n",
		need_str, desc_count(need_str, WHAT_STR));
}

long GetAffectNumByName(std::string affName)
{
	int base = 0, offset = 0, counter = 0;
	bool endOfArray = false;
	while (!endOfArray)
	{
		if (affName == string(affected_bits[counter]))
			return ((base << 30) | (1 << offset));
		offset++;
		if (*affected_bits[counter] == '\n')
		{
			base++;
			offset = 0;
			if (*affected_bits[counter+1] == '\n')
				endOfArray = true;
		}
		counter++;
	}
	return -1;
}

/// считает кол-во цветов &R и т.п. в строке
/// size_t len = 0 - по дефолту считает strlen(str)
int count_colors(const char * str, size_t len)
{
	unsigned int c, cc = 0;
	len = len ? len : strlen(str);

	for (c = 0; c < len - 1; c++)
	{
		if (*(str+c)=='&' && *(str+c+1)!='&')
			cc++;
	}
	return cc;
}

//возвращает строку длины len + кол-во цветов*2 для того чтоб в табличке все было ровненько
//left_align выравнивание строки влево
char* colored_name(const char * str, int len, const bool left_align)
{
	static char cstr[128];
	static char fmt[6];
	unsigned int cc = len + count_colors(str) * 2;

	if (strlen(str)<cc)
	{
		snprintf(fmt, sizeof(fmt), "%%%s%ds", (left_align?"-":""), cc);
		snprintf(cstr, sizeof(cstr), fmt, str);
	}
	else
	{
		snprintf(cstr, sizeof(cstr), "%s", str);
	}
	return cstr;
}

/// длина строки без символов цвета из count_colors()
size_t strlen_no_colors(const char *str)
{
	const size_t len = strlen(str);
	return len - count_colors(str, len) * 2;
}

////////////////////////////////////////////////////////////////////////////////
namespace SetSystem {

struct SetNode
{
	// список шмоток по конкретному сету для сверки
	// инится один раз при ребуте и больше не меняется
	std::set<int> set_vnum;
	// список шмоток из данного сета у текущего чара
	// если после заполнения в списке только 1 предмет
	// значит удаляем его как единственный у чара
	std::vector<int> obj_vnum;
};

std::vector<SetNode> set_list;
const unsigned BIG_SET_ITEMS = 9;
// для проверок при попытке ренты
std::set<int> vnum_list;

// * Заполнение списка фулл-сетов для последующих сверок.
void init_set_list()
{
	for (id_to_set_info_map::const_iterator i = obj_data::set_table.begin(),
		iend = obj_data::set_table.end(); i != iend; ++i)
	{
		if (i->second.size() > BIG_SET_ITEMS)
		{
			SetNode node;
			for (set_info::const_iterator k = i->second.begin(),
				kend = i->second.end(); k != kend; ++k)
			{
				node.set_vnum.insert(k->first);
			}
			set_list.push_back(node);
		}
	}
}

// * Удаление инфы от последнего сверявшегося чара.
void reset_set_list()
{
	for (std::vector<SetNode>::iterator i = set_list.begin(),
		iend = set_list.end(); i != iend; ++i)
	{
		i->obj_vnum.clear();
	}
}

// * Проверка шмотки на принадлежность к сетам из set_list.
void check_item(int vnum)
{
	for (std::vector<SetNode>::iterator i = set_list.begin(),
		iend = set_list.end(); i != iend; ++i)
	{
		std::set<int>::const_iterator k = i->set_vnum.find(vnum);
		if (k != i->set_vnum.end())
		{
			i->obj_vnum.push_back(vnum);
		}
	}
}

// * Обнуление таймера шмотки в ренте или перс.хране.
void delete_item(int pt_num, int vnum)
{
	bool need_save = false;
	// рента
	if (player_table[pt_num].timer)
	{
		for (std::vector<save_time_info>::iterator i = player_table[pt_num].timer->time.begin(),
			iend = player_table[pt_num].timer->time.end(); i != iend; ++i)
		{
			if (i->vnum == vnum)
			{
				log("[TO] Player %s : set-item %d deleted",
					player_table[pt_num].name, i->vnum);
				i->timer = -1;
				int rnum = real_object(i->vnum);
				if (rnum >= 0)
				{
					obj_index[rnum].stored--;
				}
				need_save = true;
			}
		}
	}
	if (need_save)
	{
		if (!Crash_write_timer(pt_num))
		{
			log("SYSERROR: [TO] Error writing timer file for %s",
				player_table[pt_num].name);
		}
		return;
	}
	// перс.хран
	Depot::delete_set_item(player_table[pt_num].unique, vnum);
}

// * Проверка при ребуте всех рент и перс.хранилищ чаров.
void check_rented()
{
	init_set_list();

	for (int i = 0; i <= top_of_p_table; i++)
	{
		reset_set_list();
		// рента
		if (player_table[i].timer)
		{
			for (std::vector<save_time_info>::iterator it = player_table[i].timer->time.begin(),
				it_end = player_table[i].timer->time.end(); it != it_end; ++it)
			{
				if (it->timer >= 0)
				{
					check_item(it->vnum);
				}
			}
		}
		// перс.хран
		Depot::check_rented(player_table[i].unique);
		// проверка итогового списка
		for (std::vector<SetNode>::iterator it = set_list.begin(),
			iend = set_list.end(); it != iend; ++it)
		{
			if (it->obj_vnum.size() == 1)
			{
				delete_item(i, it->obj_vnum[0]);
			}
		}
	}
}




/**
 * Почта, базар.
 * Предметы сетов из BIG_SET_ITEMS и более предметов не принимаются.
 */
bool is_big_set(const OBJ_DATA *obj,bool is_mini)
{
	unsigned int sets_items = is_mini ? MINI_SET_ITEMS : BIG_SET_ITEMS;
	if (!OBJ_FLAGGED(obj, ITEM_SETSTUFF))
	{
		return false;
	}
	for (id_to_set_info_map::const_iterator i = obj_data::set_table.begin(),
		iend = obj_data::set_table.end(); i != iend; ++i)
	{
		if (i->second.find(GET_OBJ_VNUM(obj)) != i->second.end()
			&& i->second.size() > sets_items)
		{
			return true;
		}
	}
	return false;
}



bool find_set_item(OBJ_DATA *obj)
{
	for (; obj; obj = obj->next_content)
	{
		std::set<int>::const_iterator i = vnum_list.find(GET_OBJ_VNUM(obj));
		if (i != vnum_list.end())
		{
			return true;
		}
		if (find_set_item(obj->contains))
		{
			return true;
		}
	}
	return false;
}

// * Генерация списка сетин из того же набора, что и vnum (исключая ее саму).
void init_vnum_list(int vnum)
{
	vnum_list.clear();
	for (id_to_set_info_map::const_iterator i = obj_data::set_table.begin(),
		iend = obj_data::set_table.end(); i != iend; ++i)
	{
		if (i->second.find(vnum) != i->second.end())
			//&& i->second.size() > BIG_SET_ITEMS)
		{
			for (set_info::const_iterator k = i->second.begin(),
				kend = i->second.end(); k != kend; ++k)
			{
				if (k->first != vnum)
				{
					vnum_list.insert(k->first);
				}
			}
		}
	}

	if (vnum_list.empty())
	{
		vnum_list = obj_sets::vnum_list_add(vnum);
	} 
}

/**
 * Экипировка, инвентарь, чармисы, перс. хран.
 * Требуется наличие двух и более предметов, если сетина из большого сета.
 * Перс. хран, рента.
 */
bool is_norent_set(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (!OBJ_FLAGGED(obj, ITEM_SETSTUFF))
	{
		return false;
	}

	init_vnum_list(GET_OBJ_VNUM(obj));

	if (vnum_list.empty())
	{
		return false;
	}

	// экипировка
	for (int i = 0; i < NUM_WEARS; ++i)
	{
		if (find_set_item(GET_EQ(ch, i)))
		{
			return false;
		}
	}
	// инвентарь
	if (find_set_item(ch->carrying))
	{
		return false;
	}
	// чармисы
	if (ch->followers)
	{
		for (struct follow_type *k = ch->followers; k; k = k->next)
		{
			if (!IS_CHARMICE(k->follower) || !k->follower->master)
			{
				continue;
			}
			for (int j = 0; j < NUM_WEARS; j++)
			{
				if (find_set_item(GET_EQ(k->follower, j)))
				{
					return false;
				}
			}
			if (find_set_item(k->follower->carrying))
			{
				return false;
			}
		}
	}
	// перс. хранилище
	if (Depot::find_set_item(ch, vnum_list))
	{
		return false;
	}

	return true;
}

} // namespace SetSystem

////////////////////////////////////////////////////////////////////////////////

// Симуляция телла от моба
void tell_to_char(CHAR_DATA *keeper, CHAR_DATA *ch, const char *arg)
{
	if (AFF_FLAGGED(ch, AFF_DEAFNESS) || PRF_FLAGGED(ch, PRF_NOTELL))
		return;
	char local_buf[MAX_INPUT_LENGTH];
	snprintf(local_buf, MAX_INPUT_LENGTH,
		"%s сказал%s вам : '%s'", GET_NAME(keeper), GET_CH_SUF_1(keeper), arg);
	send_to_char(ch, "%s%s%s\r\n",
		CCICYN(ch, C_NRM), CAP(local_buf), CCNRM(ch, C_NRM));
}

int can_carry_n(CHAR_DATA* ch)
{
	int n = 5 + GET_REAL_DEX(ch) / 2 + GET_LEVEL(ch) / 2;
	if (HAVE_FEAT(ch, JUGGLER_FEAT))
	{
		n += GET_LEVEL(ch) / 2;
		if (GET_CLASS(ch) == CLASS_DRUID)
		{
			n += 5;
		}
	}
	if (GET_CLASS(ch) == CLASS_DRUID)
	{
		n += 5;
	}
	return std::max(n, 1);
}

void tascii(const uint32_t* pointer, int num_planes, char* ascii)
{
	int i, c, found;

	for (i = 0, found = FALSE; i < num_planes; i++)
	{
		for (c = 0; c < 30; c++)
		{
			if (*(pointer + i) & (1 << c))
			{
				found = TRUE;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
		}
	}
	if (!found)
		strcat(ascii, "0 ");
	else
		strcat(ascii, " ");
}

bool ParseFilter::init_type(const char *str)
{
	if (is_abbrev(str, "свет") || is_abbrev(str, "light"))
		type = ITEM_LIGHT;
	else if (is_abbrev(str, "свиток") || is_abbrev(str, "scroll"))
		type = ITEM_SCROLL;
	else if (is_abbrev(str, "палочка") || is_abbrev(str, "wand"))
		type = ITEM_WAND;
	else if (is_abbrev(str, "посох") || is_abbrev(str, "staff"))
		type = ITEM_STAFF;
	else if (is_abbrev(str, "оружие") || is_abbrev(str, "weapon"))
		type = ITEM_WEAPON;
	else if (is_abbrev(str, "броня") || is_abbrev(str, "armor"))
		type = ITEM_ARMOR;
	else if (is_abbrev(str, "напиток") || is_abbrev(str, "potion"))
		type = ITEM_POTION;
	else if (is_abbrev(str, "прочее") || is_abbrev(str, "другое") || is_abbrev(str, "other"))
		type = ITEM_OTHER;
	else if (is_abbrev(str, "контейнер") || is_abbrev(str, "container"))
		type = ITEM_CONTAINER;
	else if (is_abbrev(str, "материал") || is_abbrev(str, "material"))
		type = ITEM_MATERIAL;
	else if (is_abbrev(str, "зачарованный")  | is_abbrev(str, "enchant"))
		type = ITEM_ENCHANT;
	else if (is_abbrev(str, "емкость") || is_abbrev(str, "tank"))
		type = ITEM_DRINKCON;
	else if (is_abbrev(str, "книга") || is_abbrev(str, "book"))
		type = ITEM_BOOK;
	else if (is_abbrev(str, "руна") || is_abbrev(str, "rune"))
		type = ITEM_INGRADIENT;
	else if (is_abbrev(str, "ингредиент") || is_abbrev(str, "ingradient"))
		type = ITEM_MING;
	else if (is_abbrev(str, "легкие") || is_abbrev(str, "легкая"))
		type = ITEM_ARMOR_LIGHT;
	else if (is_abbrev(str, "средние") || is_abbrev(str, "средняя"))
		type = ITEM_ARMOR_MEDIAN;
	else if (is_abbrev(str, "тяжелые") || is_abbrev(str, "тяжелая"))
		type = ITEM_ARMOR_HEAVY;
	else
		return false;

	return true;
}

bool ParseFilter::init_state(const char *str)
{
	if (is_abbrev(str, "ужасно"))
		state = 0;
	else if (is_abbrev(str, "скоро испортится"))
		state = 20;
	else if (is_abbrev(str, "плоховато"))
		state = 40;
	else if (is_abbrev(str, "средне"))
		state = 60;
	else if (is_abbrev(str, "идеально"))
		state = 80;
	else if (is_abbrev(str, "нерушимо"))
		state = 1000;
	else return false;

	return true;
}

bool ParseFilter::init_wear(const char *str)
{
	if (is_abbrev(str, "палец"))
	{
		wear = ITEM_WEAR_FINGER;
		wear_message = 1;
	}
	else if (is_abbrev(str, "шея") || is_abbrev(str, "грудь"))
	{
		wear = ITEM_WEAR_NECK;
		wear_message = 2;
	}
	else if (is_abbrev(str, "тело"))
	{
		wear = ITEM_WEAR_BODY;
		wear_message = 3;
	}
	else if (is_abbrev(str, "голова"))
	{
		wear = ITEM_WEAR_HEAD;
		wear_message = 4;
	}
	else if (is_abbrev(str, "ноги"))
	{
		wear = ITEM_WEAR_LEGS;
		wear_message = 5;
	}
	else if (is_abbrev(str, "ступни"))
	{
		wear = ITEM_WEAR_FEET;
		wear_message = 6;
	}
	else if (is_abbrev(str, "кисти"))
	{
		wear = ITEM_WEAR_HANDS;
		wear_message = 7;
	}
	else if (is_abbrev(str, "руки"))
	{
		wear = ITEM_WEAR_ARMS;
		wear_message = 8;
	}
	else if (is_abbrev(str, "щит"))
	{
		wear = ITEM_WEAR_SHIELD;
		wear_message = 9;
	}
	else if (is_abbrev(str, "плечи"))
	{
		wear = ITEM_WEAR_ABOUT;
		wear_message = 10;
	}
	else if (is_abbrev(str, "пояс"))
	{
		wear = ITEM_WEAR_WAIST;
		wear_message = 11;
	}
	else if (is_abbrev(str, "запястья"))
	{
		wear = ITEM_WEAR_WRIST;
		wear_message = 12;
	}
	else if (is_abbrev(str, "правая"))
	{
		wear = ITEM_WEAR_WIELD;
		wear_message = 13;
	}
	else if (is_abbrev(str, "левая"))
	{
		wear = ITEM_WEAR_HOLD;
		wear_message = 14;
	}
	else if (is_abbrev(str, "обе"))
	{
		wear = ITEM_WEAR_BOTHS;
		wear_message = 15;
	}
	else
	{
		return false;
	}

	return true;
}

bool ParseFilter::init_cost(const char *str)
{
	if (sscanf(str, "%d%[-+]", &cost, &cost_sign) != 2)
	{
		return false;
	}
	if (cost_sign == '-')
	{
		cost = -cost;
	}

	return true;
}

bool ParseFilter::init_weap_class(const char *str)
{
	if (is_abbrev(str, "луки"))
	{
		weap_class = SKILL_BOWS;
		weap_message = 0;
	}
	else if (is_abbrev(str, "короткие"))
	{
		weap_class = SKILL_SHORTS;
		weap_message = 1;
	}
	else if (is_abbrev(str, "длинные"))
	{
		weap_class = SKILL_LONGS;
		weap_message = 2;
	}
	else if (is_abbrev(str, "секиры"))
	{
		weap_class = SKILL_AXES;
		weap_message = 3;
	}
	else if (is_abbrev(str, "палицы"))
	{
		weap_class = SKILL_CLUBS;
		weap_message = 4;
	}
	else if (is_abbrev(str, "иное"))
	{
		weap_class = SKILL_NONSTANDART;
		weap_message = 5;
	}
	else if (is_abbrev(str, "двуручники"))
	{
		weap_class = SKILL_BOTHHANDS;
		weap_message = 6;
	}
	else if (is_abbrev(str, "проникающее"))
	{
		weap_class = SKILL_PICK;
		weap_message = 7;
	}
	else if (is_abbrev(str, "копья"))
	{
		weap_class = SKILL_SPADES;
		weap_message = 8;
	}
	else
	{
		return false;
	}

	type = ITEM_WEAPON;
	return true;
}

size_t ParseFilter::affects_cnt() const
{
	return affect_weap.size() + affect_apply.size() + affect_extra.size();
}

bool ParseFilter::init_affect(char *str, size_t str_len)
{
	// Аимя!
	bool strong = false;
	if (str_len > 1 && str[str_len - 1] == '!')
	{
		strong = true;
		str[str_len - 1] = '\0';
	}
	// А1, А2, А3
	if (str_len == 1)
	{
		switch (*str)
		{
			case '1':
				sprintf(str, "можно вплавить 1 камень");
			break;
			case '2':
				sprintf(str, "можно вплавить 2 камня");
			break;
			case '3':
				sprintf(str, "можно вплавить 3 камня");
			break;
		}
	}

	lower_convert(str);
	str_len = strlen(str);

	for (int num = 0; *apply_types[num] != '\n'; ++num)
	{
		if (strong && !strcmp(str, apply_types[num]))
		{
			affect_apply.push_back(num);
			return true;
		}
		else if (!strong && isname(str, apply_types[num]))
		{
			affect_apply.push_back(num);
			return true;
		}
	}

	int num = 0;
	for (int flag = 0; flag < 4; ++flag)
	{
		for (/* тут ничего не надо */; *weapon_affects[num] != '\n'; ++num)
		{
			if (strong && !strcmp(str, weapon_affects[num]))
			{
				affect_weap.push_back(num);
				return true;
			}
			else if (!strong && isname(str, weapon_affects[num]))
			{
				affect_weap.push_back(num);
				return true;
			}
		}
		++num;
	}

	num = 0;
	for (int flag = 0; flag < 4; ++flag)
	{
		for (/* тут ничего не надо */; *extra_bits[num] != '\n'; ++num)
		{
			if (strong && !strcmp(str, extra_bits[num]))
			{
				affect_extra.push_back(num);
				return true;
			}
			else if (!strong && isname(str, extra_bits[num]))
			{
				affect_extra.push_back(num);
				return true;
			}
		}
		num++;
	}

	return false;
}

/// имя, метка для клан-хранов
bool ParseFilter::check_name(OBJ_DATA *obj, CHAR_DATA *ch) const
{
	bool result = false;
	if (name.empty() || isname(name, GET_OBJ_PNAME(obj, 0)))
	{
		result = true;
	}
	else if (((GET_OBJ_TYPE(obj) == ITEM_MING)
			|| (GET_OBJ_TYPE(obj) == ITEM_INGRADIENT))
		&& GET_OBJ_RNUM(obj) >= 0
		&& isname(name, obj_proto[GET_OBJ_RNUM(obj)]->aliases))
	{
		result = true;
	}
	else if (ch && filter_type == CLAN
		&& CHECK_CUSTOM_LABEL(name.c_str(), obj, ch))
	{
		result = true;
	}
	return result;
}

bool ParseFilter::check_type(OBJ_DATA *obj) const
{
	if (type < 0 || type == GET_OBJ_TYPE(obj))
	{
		return true;
	}
	return false;
}

bool ParseFilter::check_state(OBJ_DATA *obj) const
{
	bool result = false;
	if (state < 0)
	{
		result = true;
	}
	else if (GET_OBJ_RNUM(obj) >= 0)
	{
		int proto_tm = obj_proto.at(GET_OBJ_RNUM(obj))->get_timer();
		if (proto_tm <= 0)
		{
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_),
				"SYSERROR: wrong obj-proto timer %d, vnum=%d (%s %s:%d)",
				proto_tm, obj_proto.at(GET_OBJ_RNUM(obj))->item_number,
				__func__, __FILE__, __LINE__);
			mudlog(buf_, CMP, LVL_IMMORT, SYSLOG, TRUE);
		}
		else
		{
			int tm_pct;
			if (check_unlimited_timer(obj))  // если шмотка нерушима, физически проставляем текст нерушимо
				tm_pct = 1000;
			else
				tm_pct = obj->get_timer() * 100 / proto_tm;
			if (filter_type == CLAN
				&& tm_pct >= state
				&& tm_pct < state + 20)
			{
				result = true;
			}
			else if (filter_type == EXCHANGE && tm_pct >= state)
			{
				result = true;
			}
		}
	}
	return result;
}

bool ParseFilter::check_wear(OBJ_DATA *obj) const
{
	if (wear < 0 || CAN_WEAR(obj, wear))
	{
		return true;
	}
	return false;
}

bool ParseFilter::check_weap_class(OBJ_DATA *obj) const
{
	if (weap_class < 0 || weap_class == GET_OBJ_SKILL(obj))
	{
		return true;
	}
	return false;
}

bool ParseFilter::check_cost(int obj_price) const
{
	bool result = false;

	if (cost_sign == '\0')
	{
		result = true;
	}
	else if (cost >= 0 && obj_price >= cost)
	{
		result = true;
	}
	else if (cost < 0 && obj_price <= -cost)
	{
		result = true;
	}
	return result;
}

// заколебали эти флаги... сравниваем num и все поля в flags
bool CompareBits(FLAG_DATA flags, const char *names[], int affect)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		int nr = 0, fail = 0;
		bitvector_t bitvector = flags.flags[i] | (i << 30);
		if (bitvector < static_cast<uint32_t>(INT_ONE));
		else if (bitvector < static_cast<uint32_t>(INT_TWO))
			fail = 1;
		else if (bitvector < static_cast<uint32_t>(INT_THREE))
			fail = 2;
		else
			fail = 3;
		bitvector &= 0x3FFFFFFF;
		while (fail)
		{
			if (*names[nr] == '\n')
				fail--;
			nr++;
		}

		for (; bitvector; bitvector >>= 1)
		{
			if (IS_SET(bitvector, 1))
				if (*names[nr] != '\n')
					if (nr == affect)
						return 1;
			if (*names[nr] != '\n')
				nr++;
		}
	}
	return 0;
}

bool ParseFilter::check_affect_weap(OBJ_DATA *obj) const
{
	if (!affect_weap.empty())
	{
		for (auto it = affect_weap.begin(); it != affect_weap.end(); ++it)
		{
			if (!CompareBits(obj->obj_flags.affects, weapon_affects, *it))
			{
				return false;
			}
		}
	}
	return true;
}

bool ParseFilter::check_affect_apply(OBJ_DATA *obj) const
{
	bool result = true;
	if (!affect_apply.empty())
	{
		for (auto it = affect_apply.begin(); it != affect_apply.end() && result; ++it)
		{
			result = false;
			for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
			{
				if (obj->affected[i].location == *it)
				{
					int mod = obj->affected[i].modifier;
					char buf_[MAX_INPUT_LENGTH];
					sprinttype(obj->affected[i].location, apply_types, buf_);
					for (int j = 0; *apply_negative[j] != '\n'; j++)
					{
						if (!str_cmp(buf_, apply_negative[j]))
						{
							mod = -mod;
							break;
						}
					}
					if (mod > 0)
					{
						result = true;
						break;
					}
				}
			}
		}
	}
	return result;
}

bool ParseFilter::check_affect_extra(OBJ_DATA *obj) const
{
	if (!affect_extra.empty())
	{
		for (auto it = affect_extra.begin(); it != affect_extra.end(); ++it)
		{
			if (!CompareBits(obj->obj_flags.extra_flags, extra_bits, *it))
			{
				return false;
			}
		}
	}
	return true;
}

bool ParseFilter::check_owner(exchange_item_data *exch_obj) const
{
	if (owner.empty()
		|| isname(owner, get_name_by_id(GET_EXCHANGE_ITEM_SELLERID(exch_obj))))
	{
		return true;
	}
	return false;
}

bool ParseFilter::check(OBJ_DATA *obj, CHAR_DATA *ch)
{
	if (check_name(obj, ch)
		&& check_type(obj)
		&& check_state(obj)
		&& check_wear(obj)
		&& check_weap_class(obj)
		&& check_cost(GET_OBJ_COST(obj))
		&& check_affect_apply(obj)
		&& check_affect_weap(obj)
		&& check_affect_extra(obj))
	{
		return true;
	}
	return false;
}

bool ParseFilter::check(exchange_item_data *exch_obj)
{
	OBJ_DATA *obj = GET_EXCHANGE_ITEM(exch_obj);
	if (check_name(obj)
		&& check_owner(exch_obj)
		//&& (owner_id == -1 || owner_id == GET_EXCHANGE_ITEM_SELLERID(exch_obj))
		&& check_type(obj)
		&& check_state(obj)
		&& check_wear(obj)
		&& check_weap_class(obj)
		&& check_cost(GET_EXCHANGE_ITEM_COST(exch_obj))
		&& check_affect_apply(obj)
		&& check_affect_weap(obj)
		&& check_affect_extra(obj))
	{
		return true;
	}
	return false;
}

const char *print_obj_state(int tm_pct)
{
	if (tm_pct < 20)
		return "ужасно";
	else if (tm_pct < 40)
		return "скоро испортится";
	else if (tm_pct < 60)
		return "плоховато";
	else if (tm_pct < 80)
		return "средне";
	//else if (tm_pct <=100) // у только что созданной шмотки значение 100% первый тик, потому <=
	//	return "идеально";
	else if (tm_pct <1000) // проблема крафта, на хаймортах таймер больще прототипа
		return "идеально";
	else return "нерушимо";
}

std::string ParseFilter::print() const
{
	std::string buffer = "Выборка: ";
	if (!name.empty())
	{
		buffer += name + " ";
	}
	/*
	if (owner_id >= 0)
	{
		const char *name = get_name_by_id(owner_id);
		if (name && name[0] != '\0')
		{
			buffer += name;
			buffer += " ";
		}
	}
	*/
	if (!owner.empty())
	{
		buffer += owner + " ";
	}
	if (type >= 0)
	{
		buffer += item_types[type];
		buffer += " ";
	}
	if (state >= 0)
	{
		buffer += print_obj_state(state);
		buffer += " ";
	}
	if (wear >= 0)
	{
		buffer += wear_bits[wear_message];
		buffer += " ";
	}
	if (weap_class >= 0)
	{
		buffer += weapon_class[weap_message];
		buffer += " ";
	}
	if (cost >= 0)
	{
		sprintf(buf, "%d%c ", cost, cost_sign);
		buffer += buf;
	}
	if (!affect_weap.empty())
	{
		for (auto it = affect_weap.begin(); it != affect_weap.end(); ++it)
		{
			buffer += weapon_affects[*it];
			buffer += " ";
		}
	}
	if (!affect_apply.empty())
	{
		for (auto it = affect_apply.begin(); it != affect_apply.end(); ++it)
		{
			buffer += apply_types[*it];
			buffer += " ";
		}
	}
	if (!affect_extra.empty())
	{
		for (auto it = affect_extra.begin(); it != affect_extra.end(); ++it)
		{
			buffer += extra_bits[*it];
			buffer += " ";
		}
	}
	buffer += "\r\n";

	return buffer;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
