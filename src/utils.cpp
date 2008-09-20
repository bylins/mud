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

#include "conf.h"
#include <fstream>
#include <cmath>
#include <boost/lexical_cast.hpp>

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

extern DESCRIPTOR_DATA *descriptor_list;

extern CHAR_DATA *mob_proto;

/* local functions */
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1);
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
void die_follower(CHAR_DATA * ch);
void prune_crlf(char *txt);
int valid_email(const char *address);

/* external functions */
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


/* return char with UID n */
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

/* Create and append to dinamyc length string - Alez */
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

/* Create a duplicate of a string */
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

/*
 * Strips \r\n from end of string.
 */
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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

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
			return (chk);	/* not equal */

	if (arg1.length() == arg2.length() || (n == 0))
		return (0);

	if (i == arg1.length())
		return (LOWER('\0') - LOWER(arg2[i]));
	else
		return (LOWER(arg1[i]) - LOWER('\0'));
}

// дескрипторы открытых файлов логов для сброса буфера при креше
std::list<FILE *> opened_files;

/**
* Чтобы не дублировать создание даты в каждом виде лога.
*/
void write_time(FILE *file)
{
	char time_buf[20];
	time_t ct = time(0);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	fprintf(file, "%s :: ", time_buf);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void log(const char *format, ...)
{
	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	if (logfile == NULL)
		puts("SYSERR: Using log() before stream was initialized!");
	if (format == NULL)
		format = "SYSERR: log() received a NULL format.";

	time_s[strlen(time_s) - 1] = '\0';

	fprintf(logfile, "%-15.15s :: ", time_s + 4);

	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);

	fprintf(logfile, "\n");

// shapirus: для дебаггинга
#ifdef LOG_AUTOFLUSH
	fflush(logfile);
#endif
}

void olc_log(const char *format, ...)
{
	const char *filename = "../log/olc.log";
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
	else if (!format)
		format = "SYSERR: olc_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
}

void imm_log(const char *format, ...)
{
	const char *filename = "../log/imm.log";
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
	else if (!format)
		format = "SYSERR: imm_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
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

/* the "touch" command, essentially. */
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
		return;		/* eh, oh well. */
	if (channel < 0 || channel >= NLOG)
		return;
	if (file)
	{
		logfile = logs[channel].logfile;
		log(str);
		logfile = logs[SYSLOG].logfile;
	}
	if (level < 0)
		return;

	sprintf(tmpbuf, "[ %s ]\r\n", str);
	bool kroder = 0;
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING || IS_NPC(i->character))	/* switch */
			continue;
		if (GET_LOGS(i->character)[channel] < type && type != DEF)
			continue;
		kroder = Privilege::check_flag(i->character, Privilege::KRODER);
		if (type == DEF && GET_LEVEL(i->character) < LVL_IMMORT && !kroder)
			continue;
		if (GET_LEVEL(i->character) < level && !kroder)
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

int sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div)
{
	long nr = 0, fail = 0, divider = FALSE;

	*result = '\0';

	if ((unsigned long) bitvector < (unsigned long) INT_ONE);
	else if ((unsigned long) bitvector < (unsigned long) INT_TWO)
		fail = 1;
	else if ((unsigned long) bitvector < (unsigned long) INT_THREE)
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
		{
			if (*names[nr] != '\n')
			{
				strcat(result, names[nr]);
				strcat(result, div);
				divider = TRUE;
			}
			else
			{
				strcat(result, "UNDEF");
				strcat(result, div);
				divider = TRUE;
			}
		}
		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result)
	{
		strcat(result, empty_string);
		return FALSE;
	}
	else if (divider)
		*(result + strlen(result) - 1) = '\0';
	return TRUE;
}

int sprintbit(bitvector_t bitvector, const char *names[], char *result)
{
	return sprintbitwd(bitvector, names, result, ",");
}

void sprintbits(FLAG_DATA flags, const char *names[], char *result, const char *div)
{
	char buffer[MAX_STRING_LENGTH];
	int i;
	*result = '\0';
	for (i = 0; i < 4; i++)
	{
		if (sprintbitwd(flags.flags[i] | (i << 30), names, buffer, div))
		{
			if (strlen(result))
				strcat(result, div);
			strcat(result, buffer);
		}
	}
	if (!strlen(result))
		strcat(result, buffer);
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


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1)
{
	long secs;
	static TIME_INFO_DATA now;

	secs = (long)(t2 - t1);

	now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_REAL_HOUR * now.hours;

	now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
	/* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

	now.month = -1;
	now.year = -1;

	return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1)
{
	long secs;
	static TIME_INFO_DATA now;

	secs = (long)(t2 - t1);

	now.hours = (secs / (SECS_PER_MUD_HOUR * TIME_KOEFF)) % HOURS_PER_DAY;	/* 0..23 hours */
	secs -= SECS_PER_MUD_HOUR * TIME_KOEFF * now.hours;

	now.day = (secs / (SECS_PER_MUD_DAY * TIME_KOEFF)) % DAYS_PER_MONTH;	/* 0..29 days  */
	secs -= SECS_PER_MUD_DAY * TIME_KOEFF * now.day;

	now.month = (secs / (SECS_PER_MUD_MONTH * TIME_KOEFF)) % MONTHS_PER_YEAR;	/* 0..11 months */
	secs -= SECS_PER_MUD_MONTH * TIME_KOEFF * now.month;

	now.year = (secs / (SECS_PER_MUD_YEAR * TIME_KOEFF));	/* 0..XX? years */

	return (&now);
}



TIME_INFO_DATA *age(CHAR_DATA * ch)
{
	static TIME_INFO_DATA player_age;

	player_age = *mud_time_passed(time(0), ch->player_data.time.birth);

	player_age.year += 17;	/* All players start at 17 */

	return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
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
		act("$N сбросил$G Вас со своей спины.", FALSE, ch->master, 0, ch, TO_CHAR);
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


/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
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
		act("$n прекратил$g следовать за $N4.", TRUE, ch, 0, ch->master, TO_NOTVICT);
	}

	//log("[Stop follower] Stop horse");
	if (get_horse(ch->master) == ch && on_horse(ch->master))
		horse_drop(ch);
	else
		act("$n прекратил$g следовать за Вами.", TRUE, ch, 0, ch->master, TO_VICT);

	//log("[Stop follower] Remove from followers list");
	if (!ch->master->followers)
		log("[Stop follower] SYSERR: Followers absent for %s (master %s).", GET_NAME(ch), GET_NAME(ch->master));
	else if (ch->master->followers->follower == ch)  	/* Head of follower-list? */
	{
		k = ch->master->followers;
		if (!(ch->master->followers = k->next) && !ch->master->master)
			REMOVE_BIT(AFF_FLAGS(ch->master, AFF_GROUP), AFF_GROUP);
		free(k);
	}
	else  		/* locate follower who is not head of list */
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
		if (FIGHTING(ch))
			stop_fighting(ch, TRUE);
		/*
		   log("[Stop follower] Stop fight charmee opponee");
		   for (vict = world[IN_ROOM(ch)]->people; vict; vict = vict->next)
		   {if (FIGHTING(vict) &&
		   FIGHTING(vict) == ch &&
		   FIGHTING(ch) != vict)
		   stop_fighting(vict);
		   }
		 */
		//log("[Stop follower] Charmee MOB reaction");
		if (IS_NPC(ch))
		{
			if (MOB_FLAGGED(ch, MOB_CORPSE))
			{
				act("Налетевший ветер развеял $n3, не оставив и следа.", TRUE, ch, 0, 0, TO_ROOM);
				GET_LASTROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
				perform_drop_gold(ch, get_gold(ch), SCMD_DROP, 0);
				set_gold(ch, 0);
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
						CAN_SEE(ch, master) && !FIGHTING(ch) &&
						!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))   //Polud - ну не надо агрить в мирках, незачем это
				{
					if (number(1, GET_REAL_INT(ch) * 2) > GET_REAL_CHA(master))
					{
						act("$n посчитал$g, что Вы заслуживаете смерти !",
							FALSE, ch, 0, master, TO_VICT | CHECK_DEAF);
						act("$n заорал$g : \"Ты долго водил$G меня за нос, но дальше так не пойдет !\"" "              \"Теперь только твоя смерть может искупить твой обман !!!\"", TRUE, ch, 0, master, TO_NOTVICT | CHECK_DEAF);
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



/* Called when a character that follows/is followed dies */
void die_follower(CHAR_DATA * ch)
{
	struct follow_type *j, *k = ch->followers;

	if (ch->master)
		stop_follower(ch, SF_FOLLOWERDIE);

	if (on_horse(ch))
		REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);

	for (k = ch->followers; k; k = j)
	{
		j = k->next;
		stop_follower(k->follower, SF_MASTERDIE);
	}
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
		act("$n начал$g следовать за Вами.", TRUE, ch, 0, leader, TO_VICT);
		act("$n начал$g следовать за $N4.", TRUE, ch, 0, leader, TO_NOTVICT);
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
	case CRASH_FILE:
		prefix = LIB_PLROBJS;
		suffix = SUF_OBJS;
		break;
	case TEXT_CRASH_FILE:
		prefix = LIB_PLROBJS;
		suffix = TEXT_SUF_OBJS;
		break;
	case TIME_CRASH_FILE:
		prefix = LIB_PLROBJS;
		suffix = TIME_SUF_OBJS;
		break;
	case ALIAS_FILE:
		prefix = LIB_PLRALIAS;
		suffix = SUF_ALIAS;
		break;
	case PLAYERS_FILE:
		prefix = LIB_PLRS;
		suffix = SUF_PLAYER;
		break;
	case PQUESTS_FILE:
		prefix = LIB_PLRS;
		suffix = SUF_QUESTS;
		break;
	case PMKILL_FILE:
		prefix = LIB_PLRS;
		suffix = SUF_PMKILL;
		break;
	case ETEXT_FILE:
		prefix = LIB_PLRTEXT;
		suffix = SUF_TEXT;
		break;
	case SCRIPT_VARS_FILE:
		prefix = LIB_PLRVARS;
		suffix = SUF_MEM;
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
	/* These would be duplicated otherwise... */
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

/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
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


/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, DESCRIPTOR_DATA * d, int maxlen)
{
	int total_chars, cap_next = TRUE, cap_next_next = FALSE;
	char *flow, *start = NULL, temp;
	/* warning: do not edit messages with max_str's of over this value */
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

			/* this is so that if we stopped on a sentance .. we move off the sentance delim. */
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


const char *some_pads[3][22] =
{
	{"дней", "часов", "лет", "очков", "минут", "минут", "кун", "кун", "штук", "штук", "уровней", "верст", "верст", "единиц", "единиц", "секунд", "градусов", "строк", "предметов", "перевоплощений", "недель", "месяцев"},
	{"день", "час", "год", "очко", "минута", "минуту", "куна", "куну", "штука", "штуку", "уровень", "верста", "версту", "единица", "единицу", "секунду", "градус", "строка", "предмет", "перевоплощение", "неделя", "месяц"},
	{"дня", "часа", "года", "очка", "минуты", "минуты", "куны", "куны", "штуки", "штуки", "уровня", "версты", "версты", "единицы", "единицы", "секунды", "градуса", "строки", "предмета", "перевоплощения", "недели", "месяца"}
};

const char * desc_count(int how_many, int of_what)
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



char *noclan_title(CHAR_DATA * ch)
{
	static char title[MAX_STRING_LENGTH];
	char *pos, *pos1;

	if (!GET_TITLE(ch) || !*GET_TITLE(ch))
		sprintf(title, "%s %s", race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch));
	else
	{
		if ((pos1 = strchr(GET_TITLE(ch), '/')))
			* (pos1++) = '\0';
		if ((pos = strchr(GET_TITLE(ch), ';')))
		{
			*(pos++) = '\0';
			if ((!GET_TITLE(ch) || !*GET_TITLE(ch)) && GET_REMORT(ch) == 0 && !IS_IMMORTAL(ch))
				sprintf(title, "%s %s", race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch));
			else
			{
				if (GET_REMORT(ch) == 0 && !IS_IMMORTAL(ch))
				{
					if (GET_LEVEL(ch) < MIN_TITLE_LEV)
						sprintf(title, "%s %s",
								race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch));
					else
						sprintf(title, "%s%s%s", GET_NAME(ch), *GET_TITLE(ch) ? ", " : " ",
								GET_TITLE(ch));
				}
				else
				{
					if (GET_LEVEL(ch) < MIN_TITLE_LEV)
						sprintf(title, "%s %s", pos, GET_NAME(ch));
					else
						sprintf(title, "%s %s%s%s", pos, GET_NAME(ch),
								*GET_TITLE(ch) ? ", " : "", GET_TITLE(ch));
				}
			}
			*(--pos) = ';';
		}
		else
		{
			if (GET_LEVEL(ch) < MIN_TITLE_LEV)
				sprintf(title, "%s %s", race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch));
			else if (GET_LEVEL(ch) >= MIN_TITLE_LEV)
				sprintf(title, "%s%s%s", GET_NAME(ch), *GET_TITLE(ch) ? ", " : " ", GET_TITLE(ch));
		}
		if (pos1)
			*(--pos1) = '/';
	}
	return title;
}

char *only_title(CHAR_DATA * ch)
{
	static char title[MAX_STRING_LENGTH];
	static char clan[MAX_STRING_LENGTH];
	char *pos, *pos1;

	if (CLAN(ch) && !IS_IMMORTAL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		sprintf(clan, " (%s)", GET_CLAN_STATUS(ch));
	else
		clan[0] = 0;

	if (!GET_TITLE(ch) || !*GET_TITLE(ch))
		sprintf(title, "%s%s", GET_NAME(ch), clan);
	else
	{
		if ((pos1 = strchr(GET_TITLE(ch), '/')))
			* (pos1++) = '\0';
		if ((pos = strchr(GET_TITLE(ch), ';')))
		{
			*(pos++) = '\0';
			if ((!GET_TITLE(ch) || !*GET_TITLE(ch)) && GET_REMORT(ch) == 0 && !IS_IMMORTAL(ch))
				sprintf(title, "%s%s", GET_NAME(ch), clan);
			else
			{
				if (GET_REMORT(ch) == 0 && !IS_IMMORTAL(ch))
				{
					if (GET_LEVEL(ch) < MIN_TITLE_LEV)
						sprintf(title, "%s%s", GET_NAME(ch), clan);
					else
						sprintf(title, "%s%s%s%s", GET_NAME(ch), *GET_TITLE(ch) ? ", " : " ",
								GET_TITLE(ch), clan);
				}
				else
				{
					if (GET_LEVEL(ch) < MIN_TITLE_LEV)
						sprintf(title, "%s %s%s", pos, GET_NAME(ch), clan);
					else
						sprintf(title, "%s %s%s%s%s", pos, GET_NAME(ch),
								*GET_TITLE(ch) ? ", " : "", GET_TITLE(ch), clan);
				}
			}
			*(--pos) = ';';
		}
		else
		{
			if (GET_LEVEL(ch) < MIN_TITLE_LEV)
				sprintf(title, "%s%s", GET_NAME(ch), clan);
			else if (GET_LEVEL(ch) >= MIN_TITLE_LEV)
				sprintf(title, "%s%s%s%s", GET_NAME(ch), *GET_TITLE(ch) ? ", " : " ", GET_TITLE(ch),
						clan);
		}
		if (pos1)
			*(--pos1) = '/';
	}
	return title;
}

char *race_or_title(CHAR_DATA * ch)
{
	static char title[MAX_STRING_LENGTH];
	static char clan[MAX_STRING_LENGTH];
	char *pos, *pos1;

	if (CLAN(ch) && !IS_IMMORTAL(ch) && !PRF_FLAGGED(ch, PRF_CODERINFO))
		sprintf(clan, " (%s)", GET_CLAN_STATUS(ch));
	else
		clan[0] = 0;

	if (!GET_TITLE(ch) || !*GET_TITLE(ch))
		sprintf(title, "%s %s%s", race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch), clan);
	else
	{
		if ((pos1 = strchr(GET_TITLE(ch), '/')))
			* (pos1++) = '\0';
		if ((pos = strchr(GET_TITLE(ch), ';')))
		{
			*(pos++) = '\0';
			if ((!GET_TITLE(ch) || !*GET_TITLE(ch)) && GET_REMORT(ch) == 0 && !IS_IMMORTAL(ch))
				sprintf(title, "%s %s%s", race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)],
						GET_NAME(ch), clan);
			else
			{
				if (GET_REMORT(ch) == 0 && !IS_IMMORTAL(ch))
				{
					if (GET_LEVEL(ch) < MIN_TITLE_LEV)
						sprintf(title, "%s %s%s",
								race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)], GET_NAME(ch),
								clan);
					else
						sprintf(title, "%s%s%s%s", GET_NAME(ch), *GET_TITLE(ch) ? ", " : " ",
								GET_TITLE(ch), clan);
				}
				else
				{
					if (GET_LEVEL(ch) < MIN_TITLE_LEV)
						sprintf(title, "%s %s%s", pos, GET_NAME(ch), clan);
					else
						sprintf(title, "%s %s%s%s%s", pos, GET_NAME(ch),
								*GET_TITLE(ch) ? ", " : "", GET_TITLE(ch), clan);
				}
			}
			*(--pos) = ';';
		}
		else
		{
			if (GET_LEVEL(ch) < MIN_TITLE_LEV)
				sprintf(title, "%s %s%s", race_name[(int) GET_RACE(ch)][(int) GET_SEX(ch)],
						GET_NAME(ch), clan);
			else if (GET_LEVEL(ch) >= MIN_TITLE_LEV)
				sprintf(title, "%s%s%s%s", GET_NAME(ch), *GET_TITLE(ch) ? ", " : " ", GET_TITLE(ch),
						clan);
		}
		if (pos1)
			*(--pos1) = '/';
	}
	return title;
}

int same_group(CHAR_DATA * ch, CHAR_DATA * tch)
{
	if (!ch || !tch)
		return (FALSE);

	if (IS_NPC(ch) && ch->master && !IS_NPC(ch->master))
		ch = ch->master;
	if (IS_NPC(tch) && tch->master && !IS_NPC(tch->master))
		tch = tch->master;

	// NPC's always in same group
	if ((IS_NPC(ch) && IS_NPC(tch)) || ch == tch)
		return (TRUE);

	if (!AFF_FLAGGED(ch, AFF_GROUP) || !AFF_FLAGGED(tch, AFF_GROUP))
		return (FALSE);

	if (ch->master == tch || tch->master == ch || (ch->master && ch->master == tch->master))
		return (TRUE);

	return (FALSE);
}

// Проверка является комната рентой.
int is_rent(room_rnum room)
{
	CHAR_DATA *ch;
	for (ch = world[room]->people; ch; ch = ch->next_in_room)
		if (IS_NPC(ch) && IS_RENTKEEPER(ch))
			return (TRUE);
	return (FALSE);

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
/* output act format*/
char *format_act(const char *orig, CHAR_DATA * ch, OBJ_DATA * obj, const void *vict_obj)
{
	const char *i = NULL;
	char *buf, *lbuf;
	ubyte padis;
	int stopbyte;
	CHAR_DATA *dg_victim = NULL;

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
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'm':
				i = HMHR(ch);
				break;
			case 'M':
				if (vict_obj)
					i = HMHR((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OMHR(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 's':
				i = HSHR(ch);
				break;
			case 'S':
				if (vict_obj)
					i = HSHR((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OSHR(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'e':
				i = HSSH(ch);
				break;
			case 'E':
				if (vict_obj)
					i = HSSH((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OSSH(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
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
				dg_victim = (CHAR_DATA *) vict_obj;
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
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'g':
				i = GET_CH_SUF_1(ch);
				break;
			case 'G':
				if (vict_obj)
					i = GET_CH_SUF_1((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_1(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'y':
				i = GET_CH_SUF_5(ch);
				break;
			case 'Y':
				if (vict_obj)
					i = GET_CH_SUF_5((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_5(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'u':
				i = GET_CH_SUF_2(ch);
				break;
			case 'U':
				if (vict_obj)
					i = GET_CH_SUF_2((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_2(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'w':
				i = GET_CH_SUF_3(ch);
				break;
			case 'W':
				if (vict_obj)
					i = GET_CH_SUF_3((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_3(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'q':
				i = GET_CH_SUF_4(ch);
				break;
			case 'Q':
				if (vict_obj)
					i = GET_CH_SUF_4((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, GET_OBJ_SUF_4(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;
//Polud Добавил склонение местоимения Ваш(е,а,и)
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

/* Функция проверяет может ли ch нести предмет obj и загружает предмет
   в инвентарь игрока или в комнату, где игрок находится */

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

	/* Наличие запрещенных символов или кириллицы */
	if (addr.find_first_of(special_symbols) != string::npos)
		return 0;
	size = 	addr.size();
	for (i = 0; i < size; i++)
		if (addr[i] <= ' ' || addr[i] >= 127)
			return 0;
	/* Собака должна быть только одна и на второй и далее позиции */
	while ((pos = addr.find_first_of('@', pos)) != string::npos)
	{
		dog_pos = pos;
		++count;
		++pos;
	}
	if (count != 1 || dog_pos == 0)
		return 0;
	/* Проверяем правильность синтаксиса домена */
	/* В доменной части должно быть как минимум 4 символа, считая собаку */
	if (size - dog_pos <= 3)
		return 0;
	/* Точка отсутствует, расположена сразу после собаки, или на последнем месте */
	if (addr[dog_pos + 1] == '.' || addr[size - 1] == '.' || addr.find('.', dog_pos) == string::npos)
		return 0;

	return 1;
}

/**
* Гетер голды на руках.
*/
int get_gold(CHAR_DATA *ch)
{
	return ch->points.gold;
}

/**
* Аналог GET_GOLD не поделенный на add/remove, т.к. везде логика завязана
* на то, что голда знаковый тип, менять надо более основательно.
* Изменения кун у чаров логиурем.
*/
void add_gold(CHAR_DATA *ch, int gold)
{
	if (!gold) return;

	if (!IS_NPC(ch))
	{
		if (gold > 0)
			log("Gold: %s add %d", GET_NAME(ch), gold);
		else
			log("Gold: %s remove %d", GET_NAME(ch), -gold);
	}

	ch->points.gold += gold;
}

/**
* Сет кун на руках, чаров логируем, если надо.
* \param need_log - логировать или нет, по дефолту 1, т.е. логируем
* В основнм сеты остались только при обнулении бабла и ините полей,
* т.е. всяких фишек и сетом отрицательных значений быть не должно,
* все расчеты и выкрутасы идут через add_gold.
*/
void set_gold(CHAR_DATA *ch, int gold, bool need_log)
{
	if (gold < 0 || gold > 100000000 || ch->points.gold == gold) return;

	if (need_log && !IS_NPC(ch))
	{
		int change = gold - ch->points.gold;
		if (change > 0)
			log("Gold: %s add %d", GET_NAME(ch), change);
		else
			log("Gold: %s remove %d", GET_NAME(ch), -change);
	}

	ch->points.gold = gold;
}

/**
* См get_gold.
*/
long get_bank_gold(CHAR_DATA *ch)
{
	return ch->points.bank_gold;
}

/**
* См add_gold.
*/
void add_bank_gold(CHAR_DATA *ch, long gold)
{
	if (!gold) return;

	if (!IS_NPC(ch))
	{
		if (gold > 0)
			log("Gold: %s add %ld", GET_NAME(ch), gold);
		else
			log("Gold: %s remove %ld", GET_NAME(ch), -gold);
	}

	ch->points.bank_gold += gold;
}

/**
* См set_gold.
*/
void set_bank_gold(CHAR_DATA *ch, long gold, bool need_log)
{
	if (gold < 0 || gold > 100000000 || ch->points.bank_gold == gold) return;

	if (need_log && !IS_NPC(ch))
	{
		long change = gold - ch->points.bank_gold;
		if (change > 0)
			log("Gold: %s add %ld", GET_NAME(ch), change);
		else
			log("Gold: %s remove %ld", GET_NAME(ch), -change);
	}

	ch->points.bank_gold = gold;
}

/**
* Вывод времени (минут) в зависимости от их кол-ва с округлением вплоть до месяцев.
* Для таймеров славы по 'glory имя' и 'слава информация'.
*/
std::string time_format(int in_timer)
{
	char buffer[256];
	std::ostringstream out;

	double timer = in_timer;
	int one = 0, two = 0;

	if (timer < 60)
		out << timer << " " << desc_count(in_timer, WHAT_MINa);
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
			out << " " << desc_count(one, WHAT_WEEK);
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

/**
* Для обрезания точек в карме при сете славы.
*/
void skip_dots(char **string)
{
	for (; **string && (strchr(" .", **string) != NULL); (*string)++);
}

/* Return pointer to first occurrence in string ct in */
/* cs, or NULL if not present.  Case insensitive */
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

/* remove ^M's from file output */
void kill_ems(char *str)
{
	char *ptr1, *ptr2, *tmp;

	tmp = str;
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
