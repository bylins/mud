/* ************************************************************************
*   File: config.cpp                                    Part of Bylins    *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#define __CONFIG_C__

#include "config.hpp"

#include "interpreter.h"	// alias_data definition for structs.h
#include "utils.h"
#include "constants.h"
#include "char.hpp"
#include "birth_places.hpp"
#include "structs.h"
#include "conf.h"
#include "sysdep.h"

#include <boost/version.hpp>

#include <iostream>

#define YES	    1
#define FALSE	0
#define NO	    0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .cpp file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

int level_exp(CHAR_DATA * ch, int level);

// GAME PLAY OPTIONS

// exp change limits
int max_exp_gain_npc = 100000;	// max gainable per kill

// number of tics (usually 75 seconds) before PC/NPC corpses decompose
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 30;

// How many ticks before a player is sent to the void or idle-rented.
int idle_void = 10;
int idle_rent_time = 40;

// This level and up is immune to idling, LVL_IMPL+1 will disable it.
int idle_max_level = LVL_IMMORT;

// should items in death traps automatically be junked?
int dts_are_dumps = YES;

/*
 * Whether you want items that immortals load to appear on the ground or not.
 * It is most likely best to set this to 'YES' so that something else doesn't
 * grab the item before the immortal does, but that also means people will be
 * able to carry around things like boards.  That's not necessarily a bad
 * thing, but this will be left at a default of 'NO' for historic reasons.
 */
int load_into_inventory = YES;

// "okay" etc.
const char *OK = "Ладушки.\r\n";
const char *NOPERSON = "Нет такого создания в этом мире.\r\n";
const char *NOEFFECT = "Ваши потуги оказались напрасными.\r\n";

/*
 * You can define or not define TRACK_THOUGH_DOORS, depending on whether
 * or not you want track to find paths which lead through closed or
 * hidden doors. A setting of 'NO' means to not go through the doors
 * while 'YES' will pass through doors to find the target.
 */
//int track_through_doors = YES;

// RENT/CRASHSAVE OPTIONS

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
int free_rent = NO;

// maximum number of items players are allowed to rent
//int max_obj_save = 120;

// receptionist's surcharge on top of item costs
int min_rent_cost(CHAR_DATA * ch)
{
	if ((GET_LEVEL(ch) < 15) && (GET_REMORT(ch) == 0))
		return (0);
	else
		return ((GET_LEVEL(ch) + 30 * GET_REMORT(ch)) * 2);
}

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.  This
 * option has an added meaning past bpl13.  If auto_save is YES, then
 * the 'save' command will be disabled to prevent item duplication via
 * game crashes.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 5;

// Lifetime of crashfiles, forced-rent and idlesave files in days
int crash_file_timeout = 30;

// Lifetime of normal rent files in days
int rent_file_timeout = 30;

// The period of free rent after crash or forced-rent in hours
int free_crashrent_period = 2;

/* Система автоудаления
   В этой структуре хранится информация которая определяет какаие игроки
   автоудалятся при перезагрузке мада. Уровни должны следовать в
   возрастающем порядке с невозможно малым уровнем в конце массива.
   Уровень -1 определяет удаленных угроков - т.е. через сколько времени
   они физически попуржатся.
   Если количество дней равно -1, то не удалять никогда
*/
struct pclean_criteria_data pclean_criteria[] =
{
	//     УРОВЕНЬ           ДНИ
	{ -1, 0},		// Удаленные чары - удалять сразу
	{0, 0},			// Чары 0го уровня никогда не войдут в игру, так что глюки удалять сразу
	{1, 7},
	{2, 14},
	{3, 21},
	{4, 28},
	{5, 35},
	{6, 42},
	{7, 49},
	{8, 56},
	{9, 63},
	{10, 70},
	{11, 77},
	{12, 84},
	{13, 91},
	{14, 98},
	{15, 105},
	{16, 112},
	{17, 119},
	{18, 126},
	{19, 133},
	{20, 140},
	{21, 147},
	{22, 154},
	{23, 161},
	{24, 168},
	{25, 360},
	{LVL_IMPL, -1},		// c 25го и дальше живут вечно
	{ -2, 0}			// Последняя обязательная строка
};


// ROOM NUMBERS

// virtual number of room that mortals should enter at
room_vnum mortal_start_room = 4056;	// tavern in village

// virtual number of room that immorts should enter at by default
room_vnum immort_start_room = 100;	// place  in castle

// virtual number of room that frozen players should enter at
room_vnum frozen_start_room = 101;	// something in castle

// virtual number of room that helled players should enter at
room_vnum helled_start_room = 101;	// something in castle
room_vnum named_start_room = 105;
room_vnum unreg_start_room = 103;


// GAME OPERATION OPTIONS

/*
 * This is the default port on which the game should run if no port is
 * given on the command-line.  NOTE WELL: If you're using the
 * 'autorun' script, the port number there will override this setting.
 * Change the PORT= line in autorun instead of (or in addition to)
 * changing this.
 */
ush_int DFLT_PORT = 4000;

/*
 * IP address to which the MUD should bind.  This is only useful if
 * you're running Circle on a host that host more than one IP interface,
 * and you only want to bind to *one* of them instead of all of them.
 * Setting this to NULL (the default) causes Circle to bind to all
 * interfaces on the host.  Otherwise, specify a numeric IP address in
 * dotted quad format, and Circle will only bind to that IP address.  (Of
 * course, that IP address must be one of your host's interfaces, or it
 * won't work.)
 */
const char *DFLT_IP = NULL;	// bind to all interfaces
// const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface

// default directory to use as data directory
const char *DFLT_DIR = "lib";

/*
 * What file to log messages to (ex: "log/syslog").  Setting this to NULL
 * means you want to log to stderr, which was the default in earlier
 * versions of Circle.  If you specify a file, you don't get messages to
 * the screen. (Hint: Try 'tail -f' if you have a UNIX machine.)
 */
const char *LOGNAME = NULL;
// const char *LOGNAME = "log/syslog";  -- useful for Windows users

// maximum number of players allowed before game starts to turn people away
int max_playing = 300;

// maximum size of bug, typo and idea files in bytes (to prevent bombing)
int max_filesize = 500000;

// maximum number of password attempts before disconnection
int max_bad_pws = 3;

/*
 * Rationale for enabling this, as explained by naved@bird.taponline.com.
 *
 * Usually, when you select ban a site, it is because one or two people are
 * causing troubles while there are still many people from that site who you
 * want to still log on.  Right now if I want to add a new select ban, I need
 * to first add the ban, then SITEOK all the players from that site except for
 * the one or two who I don't want logging on.  Wouldn't it be more convenient
 * to just have to remove the SITEOK flags from those people I want to ban
 * rather than what is currently done?
 */
int siteok_everyone = TRUE;

/*
 * Some nameservers are very slow and cause the game to lag terribly every
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = YES;


const char *MENU = "\r\n"
				   "0) Отсоединиться.\r\n"
				   "1) Начать игру.\r\n"
				   "2) Ввести описание вашего персонажа.\r\n"
				   "3) Узнать историю.\r\n"
				   "4) Изменить пароль.\r\n"
				   "5) Удалить персонажа.\r\n"
				   "6) Изменить параметры персонажа.\r\n"
				   "7) Включить/выключить режим слепого игрока.\r\n"
				   "\r\n"
				   "   Чего ваша душа желает? ";

const char *WELC_MESSG =
	"\r\n"
	"  Добро пожаловать на землю Киевскую, богатую историей и самыми невероятными\r\n"
	"приключениями. Возможно, вам они понравятся, и вы станете в один ряд с героями\r\n" "давно минувших дней.\r\n\r\n";

const char *START_MESSG =
	" Буде здравы, странник.\r\n"
	" Вот и ты стал на тропу увлекательных приключений, которые, надеемся, ждут\r\n"
	"тебя в нашем мире.\r\n"
	" Твоя задача непроста, но надеемся, что ты сумеешь достойно решить ее.\r\n"
	" В добрый час, путник, и да будет скатертью тебе дорога...\r\n" "\r\n";

int max_exp_gain_pc(CHAR_DATA * ch)
{
	int result = 1;
	if (!IS_NPC(ch))
	{
		int max_per_lev =
			level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch) + 0);
		result = max_per_lev / (10 + GET_REMORT(ch));
	}
	return result;
}

int max_exp_loss_pc(CHAR_DATA * ch)
{
	return (IS_NPC(ch) ? 1 : (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch) + 0)) / 3);
}

int calc_loadroom(CHAR_DATA * ch, int bplace_mode = BIRTH_PLACE_UNDEFINED)
{
	int loadroom;
    if (IS_IMMORTAL(ch))
		return (immort_start_room);
	else if (PLR_FLAGGED(ch, PLR_FROZEN))
		return (frozen_start_room);
	else
	{
        loadroom = BirthPlace::GetLoadRoom(bplace_mode);
        if (loadroom != BIRTH_PLACE_UNDEFINED)
            return loadroom;
	}
	return (mortal_start_room);
}

#if defined(BOOST_ENABLE_ASSERT_HANDLER)
void boost::assertion_failed(char const * expr, char const * function, char const * file, long line)
{
	log("Assert: expr='%s', funct='%s', file='%s', line=%ld",
		expr, function, file, line);
}

#if BOOST_VERSION >= 104600
void boost::assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line)
{
	log("Assert: expr='%s', msg='%s', funct='%s', file='%s', line=%ld",
		expr, msg, function, file, line);
}
#endif
#endif

runtime_config::logs_t runtime_config::m_logs =
{
	CLogInfo("syslog", "СИСТЕМНЫЙ"),
	CLogInfo("log/errlog.txt", "ОШИБКИ МИРА"),
	CLogInfo("log/imlog.txt", "ИНГРЕДИЕНТНАЯ МАГИЯ"),
	CLogInfo("log/msdp.txt", "лог MSDP пакетов")
};

std::string runtime_config::m_log_stderr;

bool runtime_config::open_log(const EOutputStream stream)
{
	return m_logs[stream].open();
}

void runtime_config::handle(const EOutputStream stream, FILE* handle)
{
	m_logs[stream].handle(handle);
}

void runtime_config::load_stream_config(CLogInfo& log, const pugi::xml_node* node)
{
	const auto filename = node->child("filename");
	if (filename)
	{
		log.filename(filename.child_value());
	}
	const auto buffered = node->child("buffered");
	if (buffered)
	{
		try
		{
			log.buffered(ITEM_BY_NAME<CLogInfo::EBuffered>(buffered.child_value()));
		}
		catch (...)
		{
			std::cerr << "Could not set value \"" << buffered.child_value() << "\" as buffered option." << std::endl;
		}
	}
}

typedef std::map<EOutputStream, std::string> EOutputStream_name_by_value_t;
typedef std::map<const std::string, EOutputStream> EOutputStream_value_by_name_t;
EOutputStream_name_by_value_t EOutputStream_name_by_value;
EOutputStream_value_by_name_t EOutputStream_value_by_name;

void init_EOutputStream_ITEM_NAMES()
{
	EOutputStream_name_by_value.clear();
	EOutputStream_value_by_name.clear();

	EOutputStream_name_by_value[SYSLOG] = "SYSLOG";
	EOutputStream_name_by_value[IMLOG] = "IMLOG";
	EOutputStream_name_by_value[ERRLOG] = "ERRLOG";
	EOutputStream_name_by_value[MSDP_LOG] = "MSDPLOG";

	for (const auto& i : EOutputStream_name_by_value)
	{
		EOutputStream_value_by_name[i.second] = i.first;
	}
}

template <>
EOutputStream ITEM_BY_NAME(const std::string& name)
{
	if (EOutputStream_name_by_value.empty())
	{
		init_EOutputStream_ITEM_NAMES();
	}
	return EOutputStream_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM<EOutputStream>(const EOutputStream item)
{
	if (EOutputStream_name_by_value.empty())
	{
		init_EOutputStream_ITEM_NAMES();
	}
	return EOutputStream_name_by_value.at(item);
}

typedef std::map<CLogInfo::EBuffered, std::string> EBuffered_name_by_value_t;
typedef std::map<const std::string, CLogInfo::EBuffered> EBuffered_value_by_name_t;
EBuffered_name_by_value_t EBuffered_name_by_value;
EBuffered_value_by_name_t EBuffered_value_by_name;

void init_EBuffered_ITEM_NAMES()
{
	EBuffered_name_by_value.clear();
	EBuffered_value_by_name.clear();

	EBuffered_name_by_value[CLogInfo::EBuffered::EB_FULL] = "FULL";
	EBuffered_name_by_value[CLogInfo::EBuffered::EB_LINE] = "LINE";
	EBuffered_name_by_value[CLogInfo::EBuffered::EB_NO] = "NO";

	for (const auto& i : EBuffered_name_by_value)
	{
		EBuffered_value_by_name[i.second] = i.first;
	}
}

template <>
CLogInfo::EBuffered ITEM_BY_NAME(const std::string& name)
{
	if (EBuffered_name_by_value.empty())
	{
		init_EBuffered_ITEM_NAMES();
	}
	return EBuffered_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM<CLogInfo::EBuffered>(const CLogInfo::EBuffered item)
{
	if (EBuffered_name_by_value.empty())
	{
		init_EBuffered_ITEM_NAMES();
	}
	return EBuffered_name_by_value.at(item);
}

const char* runtime_config::CONFIGURATION_FILE_NAME = "lib/misc/configuration.xml";

void runtime_config::load_from_file(const char* filename)
{
	try
	{
		pugi::xml_document document;
		if (!document.load_file(filename))
		{
			throw std::runtime_error("could not load XML configuration file");
		}

		const auto root = document.child("configuration");
		if (!root)
		{
			throw std::runtime_error("Root tag \"configuration\" not found");
		}

		const auto logging = root.child("logging");
		if (logging)
		{
			const auto log_stderr = logging.child("log_stderr");
			m_log_stderr = log_stderr.child_value();

			const auto syslog = logging.child("syslog");
			if (syslog)
			{
				load_stream_config(m_logs[SYSLOG], &syslog);
			}

			const auto errlog = logging.child("errlog");
			if (errlog)
			{
				load_stream_config(m_logs[ERRLOG], &errlog);
			}

			const auto imlog = logging.child("imlog");
			if (imlog)
			{
				load_stream_config(m_logs[IMLOG], &imlog);
			}

			const auto msdplog = logging.child("msdplog");
			if (msdplog)
			{
				load_stream_config(m_logs[MSDP_LOG], &msdplog);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error when loading configuration file " << filename << ": " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unexpected error when loading configuration file " << filename << std::endl;
	}
}

bool CLogInfo::open()
{
	FILE* handle = fopen(m_filename.c_str(), "w");

	if (handle)
	{
		setvbuf(handle, m_buffer, m_buffered, BUFFER_SIZE);

		m_handle = handle;
		printf("Using log file '%s' with %s buffering.\n", m_filename.c_str(), NAME_BY_ITEM(m_buffered).c_str());
		return true;
	}

	fprintf(stderr, "SYSERR: Error opening file '%s': %s\n", m_filename.c_str(), strerror(errno));

	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
