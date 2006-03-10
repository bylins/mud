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

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data definition for structs.h */
#include "utils.h"
#include "constants.h"

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

/****************************************************************************/
/****************************************************************************/
int level_exp(CHAR_DATA * ch, int level);

/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int pk_allowed = NO;

/* is playerthieving allowed? */
int pt_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 4;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* exp change limits */
int max_exp_gain_npc = 100000;	/* max gainable per kill */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 10;

/* How many ticks before a player is sent to the void or idle-rented. */
int idle_void = 10;
int idle_rent_time = 40;

/* This level and up is immune to idling, LVL_IMPL+1 will disable it. */
int idle_max_level = LVL_IMMORT;

/* should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/*
 * Whether you want items that immortals load to appear on the ground or not.
 * It is most likely best to set this to 'YES' so that something else doesn't
 * grab the item before the immortal does, but that also means people will be
 * able to carry around things like boards.  That's not necessarily a bad
 * thing, but this will be left at a default of 'NO' for historic reasons.
 */
int load_into_inventory = YES;

/* "okay" etc. */
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

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
int free_rent = NO;

/* maximum number of items players are allowed to rent */
//int max_obj_save = 120;

/* receptionist's surcharge on top of item costs */
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

/* Lifetime of crashfiles, forced-rent and idlesave files in days */
int crash_file_timeout = 30;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 30;

/* The period of free rent after crash or forced-rent in hours*/
int free_crashrent_period = 2;

/* Система автоудаления 
   В этой структуре хранится информация которая определяет какаие игроки
   автоудалятся при перезагрузке мада. Уровни должны следовать в
   возрастающем порядке с невозможно малым уровнем в конце массива.
   Уровень -1 определяет удаленных угроков - т.е. через сколько времени
   они физически попуржатся.
   Если количество дней равно -1, то не удалять никогда  
*/
struct pclean_criteria_data pclean_criteria[] = {
/*     УРОВЕНЬ           ДНИ   */
	{-1, 0},		/* Удаленные чары - удалять сразу */
	{0, 0},			/* Чары 0го уровня никогда не войдут в игру, так что глюки удалять сразу */
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
	{LVL_IMPL, -1},		/* c 25го и дальше живут вечно */
	{-2, 0}			/* Последняя обязательная строка */
};

/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
room_vnum mortal_start_room = 4056;	/* tavern in village */

/* virtual number of room that immorts should enter at by default */
room_vnum immort_start_room = 100;	/* place  in castle */

/* virtual number of room that frozen players should enter at */
room_vnum frozen_start_room = 101;	/* something in castle */

/* virtual number of room that helled players should enter at */
room_vnum helled_start_room = 101;	/* something in castle */
room_vnum named_start_room = 105;
room_vnum unreg_start_room = 103;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.cpp if you change the number of non-NOWHERE
 * donation rooms.
 */
room_vnum donation_room_1 = 100;
room_vnum donation_room_2 = NOWHERE;	/* unused - room for expansion */
room_vnum donation_room_3 = NOWHERE;	/* unused - room for expansion */


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

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
const char *DFLT_IP = NULL;	/* bind to all interfaces */
/* const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface */

/* default directory to use as data directory */
const char *DFLT_DIR = "lib";

/*
 * What file to log messages to (ex: "log/syslog").  Setting this to NULL
 * means you want to log to stderr, which was the default in earlier
 * versions of Circle.  If you specify a file, you don't get messages to
 * the screen. (Hint: Try 'tail -f' if you have a UNIX machine.)
 */
const char *LOGNAME = NULL;
/* const char *LOGNAME = "log/syslog";  -- useful for Windows users */

/* maximum number of players allowed before game starts to turn people away */
int max_playing = 300;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
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
// "Добро пожаловать в мир Киевской Руси !\r\n"
    "0) Отсоединиться.\r\n"
    "1) Начать игру.\r\n"
    "2) Ввести описание Вашего персонажа.\r\n"
    "3) Узнать историю.\r\n" "4) Изменить пароль.\r\n" "5) Удалить персонаж.\r\n" "\r\n" "   Чего Ваша душа желает ? ";



const char *WELC_MESSG =
    "\r\n"
    "  Добро пожаловать на землю Киевскую, богатую историей и самыми невероятными\r\n"
    "приключениями. Возможно, Вам они понравятся, и Вы станете в один ряд с героями\r\n" "давно минувших дней.\r\n\r\n";

const char *START_MESSG =
    " Буде здравы, странник.\r\n"
    " Вот и ты стал на тропу увлекательных приключений, которые, надеемся, ждут\r\n"
    "тебя в нашем мире.\r\n"
    " Твоя задача непроста, но надеемся, что ты сумеешь достойно решить ее.\r\n"
    " В добрый час, путник, и да будет скатертью тебе дорога...\r\n" "\r\n";

/****************************************************************************/
/****************************************************************************/

extern float exp_coefficients[];
int max_exp_gain_pc(CHAR_DATA * ch)
{
	float exp_modifier;
	int max_per_lev = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch) + 0);
	if (GET_REMORT(ch) < MAX_EXP_COEFFICIENTS_USED)
		exp_modifier = exp_coefficients[GET_REMORT(ch)];
	else
		exp_modifier = exp_coefficients[MAX_EXP_COEFFICIENTS_USED];
	return (IS_NPC(ch) ? 1 : (max_per_lev / 10) + (2 / 3 * max_per_lev * GET_REMORT(ch) / 10));
}

int max_exp_loss_pc(CHAR_DATA * ch)
{
	return (IS_NPC(ch) ? 1 : (level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch) + 0)) / 3);
}

int calc_loadroom(CHAR_DATA * ch)
{
	if (IS_IMMORTAL(ch))
		return (immort_start_room);
	else if (PLR_FLAGGED(ch, PLR_FROZEN))
		return (frozen_start_room);
	else if (GET_LEVEL(ch) < 15) {
		switch (GET_RACE(ch)) {
		case RACE_SEVERANE:
			return 4056;
			break;
		case RACE_POLANE:
			return 5000;
			break;
		case RACE_KRIVICHI:
			return 6049;
			break;
		case RACE_VATICHI:
			return 5000;
			break;
		case RACE_VELANE:
			return 4056;
			break;
		case RACE_DREVLANE:
			return 6049;
			break;
	        case RACE_POLOVCI:
			return 4056;
			break;
		case RACE_PECHENEGI:      
			return 4056;
			break;
		case RACE_MONGOLI:
			return 4056;
			break;
		case RACE_YIGURI: 
			return 4056;
			break;
		case RACE_KANGARI:
			return 4056;
			break;
		case RACE_XAZARI: 
			return 4056;
			break;
		case RACE_SVEI: 
			return 4056;
			break;
		case RACE_DATCHANE: 
			return 4056;
			break;
		case RACE_GETTI: 
			return 4056;
			break;
		case RACE_UTTI: 
			return 4056;
			break;
		case RACE_XALEIGI: 
			return 4056;
			break;
		case RACE_NORVEZCI: 
			return 4056;
			break;
		}
	}
	return (mortal_start_room);
}
