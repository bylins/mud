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

#include "config.h"

#include "gameplay/communication/boards/boards_changelog_loaders.h"
#include "gameplay/communication/boards/boards_constants.h"
#include "engine/entities/char_data.h"
#include "engine/structs/meta_enum.h"

#if CIRCLE_UNIX
#include <sys/stat.h>
#endif

#include <iostream>
#include <cstdlib>

#define YES        1
#define NO        0

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
long GetExpUntilNextLvl(CharData *ch, int level);

// GAME PLAY OPTIONS

// exp change limits
int max_exp_gain_npc = 100000;    // max gainable per kill

// number of tics (usually 75 seconds) before PC/NPC corpses decompose
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 30;

// How many ticks before a player is sent to the void or idle-rented.
int idle_void = 10;
int idle_rent_time = 40;

// This level and up is immune to idling, kLevelImplementator+1 will disable it.
int idle_max_level = kLvlImmortal;

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

const char *OK = "О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n";
const char *NOPERSON = "О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫.\r\n";
const char *NOEFFECT = "О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n";
const char *nothing_string = "О©╫О©╫О©╫О©╫О©╫О©╫";

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
int free_rent = YES;

// maximum number of items players are allowed to rent
//int max_obj_save = 120;

// receptionist's surcharge on top of item costs
int min_rent_cost(CharData *ch) {
	if ((GetRealLevel(ch) < 15) && (GetRealRemort(ch) == 0))
		return (0);
	else
		return ((GetRealLevel(ch) + 30 * GetRealRemort(ch)) * 2);
}

// Lifetime of crashfiles, forced-rent and idlesave files in days
int crash_file_timeout = 60;

// Lifetime of normal rent files in days
int rent_file_timeout = 60;

// The period of free rent after crash or forced-rent in hours
int free_crashrent_period = 2;

/* О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
   О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
   О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫. О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫
   О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫.
   О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -1 О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ - О©╫.О©╫. О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
   О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.
   О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ -1, О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
*/
struct PCCleanCriteria pclean_criteria[] =
	{
		//     О©╫О©╫О©╫О©╫О©╫О©╫О©╫           О©╫О©╫О©╫
		{-1, 0},        // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ - О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
		{0, 0},            // О©╫О©╫О©╫О©╫ 0О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫, О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
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
		{kLvlImplementator, -1},        // c 25О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
		{-2, 0}            // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	};


// ROOM NUMBERS

// virtual number of room that mortals should enter at
RoomVnum mortal_start_room = 4056;    // tavern in village

// virtual number of room that immorts should enter at by default
RoomVnum immort_start_room = 100;    // place  in castle

// virtual number of room that frozen players should enter at
RoomVnum frozen_start_room = 101;    // something in castle

// virtual number of room that helled players should enter at
RoomVnum helled_start_room = 101;    // something in castle
RoomVnum named_start_room = 105;
RoomVnum unreg_start_room = 103;


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
const char *DFLT_IP = nullptr;    // bind to all interfaces
// const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface

// default directory to use as data directory
const char *DFLT_DIR = "lib";

/*
 * What file to log messages to (ex: "log/syslog").  Setting this to NULL
 * means you want to log to stderr, which was the default in earlier
 * versions of Circle.  If you specify a file, you don't get messages to
 * the screen. (Hint: Try 'tail -f' if you have a UNIX machine.)
 */
const char *LOGNAME = nullptr;
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
int siteok_everyone = true;

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
				   "0) О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "1) О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫.\r\n"
				   "2) О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "3) О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "4) О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "5) О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "6) О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "7) О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫/О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
				   "8) О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ email'e.\r\n"
				   "\r\n"
				   "   О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫? ";

const char *WELC_MESSG =
	"\r\n"
	"  О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫\r\n"
	"О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫. О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫\r\n" "О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫.\r\n\r\n";

const char *START_MESSG =
	" О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.\r\n"
	" О©╫О©╫О©╫ О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫\r\n"
	"О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫.\r\n"
	" О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫.\r\n"
	" О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫, О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫...\r\n" "\r\n";

int max_exp_gain_pc(CharData *ch) {
	int result = 1;
	if (!ch->IsNpc()) {
		int max_per_lev = GetExpUntilNextLvl(ch, ch->GetLevel() + 1) - GetExpUntilNextLvl(ch, ch->GetLevel() + 0); //О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
		result = max_per_lev / (10 + GetRealRemort(ch));
	}
	return result;
}

int max_exp_loss_pc(CharData *ch) {
	return (ch->IsNpc() ? 1 : (GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - GetExpUntilNextLvl(ch, GetRealLevel(ch) + 0)) / 3);
}

int calc_loadroom(const CharData *ch, int bplace_mode /*= BIRTH_PLACE_UNDEFINED*/) {
	if (IS_IMMORTAL(ch)) {
		return (immort_start_room);
	} else if (ch->IsFlagged(EPlrFlag::kFrozen)) {
		return (frozen_start_room);
	} else {
		const int loadroom = Birthplaces::GetLoadRoom(bplace_mode);
		if (loadroom != kBirthplaceUndefined) {
			return loadroom;
		}
	}

	return (mortal_start_room);
}

bool RuntimeConfiguration::open_log(const EOutputStream stream) {
	return m_logs[stream].open();
}

void RuntimeConfiguration::handle(const EOutputStream stream, FILE *handle) {
	m_logs[stream].handle(handle);
}

class StreamConfigLoader {
 private:
	static constexpr int UMASK_BASE = 8;

 public:
	StreamConfigLoader(CLogInfo &log, const pugi::xml_node *node) : m_log(log), m_node(node) {}

	void load_filename();
	void load_buffered();
	void load_mode();
	void load_umask();

 private:
	CLogInfo &m_log;
	const pugi::xml_node *m_node;
};

void StreamConfigLoader::load_filename() {
	const auto filename = m_node->child("filename");
	if (filename) {
		m_log.filename(filename.child_value());
	}
}

void StreamConfigLoader::load_buffered() {
	const auto buffered = m_node->child("buffered");
	if (buffered) {
		try {
			m_log.buffered(ITEM_BY_NAME<CLogInfo::EBuffered>(buffered.child_value()));
		}
		catch (...) {
			std::cerr << "Could not set value \"" << buffered.child_value()
					  << "\" as buffered option. Using default value " << NAME_BY_ITEM(m_log.buffered()) << "\r\n";
		}
	}
}

void StreamConfigLoader::load_mode() {
	const auto mode = m_node->child("mode");
	if (mode) {
		try {
			m_log.mode(ITEM_BY_NAME<CLogInfo::EMode>(mode.child_value()));
		}
		catch (...) {
			std::cerr << "Could not set value \"" << mode.child_value() << "\" as opening mode. Using default value "
					  << NAME_BY_ITEM(m_log.mode()) << "\r\n";
		}
	}
}

void StreamConfigLoader::load_umask() {
	const auto umask = m_node->child("umask");
	if (umask) {
		try {
			const int umask_value = std::stoi(umask.child_value(), nullptr, UMASK_BASE);
			m_log.umask(umask_value);
		}
		catch (...) {
			std::cerr << "Could not set value \"" << umask.child_value()
					  << "\" as umask value. Using default value " << "\r\n";
		}
	}
}

constexpr unsigned short RuntimeConfiguration::StatisticsConfiguration::DEFAULT_PORT;

void RuntimeConfiguration::load_stream_config(CLogInfo &log, const pugi::xml_node *node) {
	StreamConfigLoader loader(log, node);
	loader.load_filename();
	loader.load_buffered();
	loader.load_mode();
	loader.load_umask();
}

void RuntimeConfiguration::setup_converters() {
	if (!log_stderr().empty()) {
		// set up converter
		const auto &encoding = log_stderr();
		if ("cp1251" == encoding) {
			m_syslog_converter = &koi_to_win;
		} else if ("alt" == encoding) {
			m_syslog_converter = static_cast<void (*)(char *, int)>(koi_to_alt);
		}
	}
}

void RuntimeConfiguration::load_logging_configuration(const pugi::xml_node *root) {
	const auto logging = root->child("logging");
	if (!logging) {
		return;
	}

	const auto log_stderr = logging.child("log_stderr");
	m_log_stderr = log_stderr.child_value();

	const auto syslog = logging.child("syslog");
	if (syslog) {
		load_stream_config(m_logs[SYSLOG], &syslog);
	}

	const auto errlog = logging.child("errlog");
	if (errlog) {
		load_stream_config(m_logs[ERRLOG], &errlog);
	}

	const auto imlog = logging.child("imlog");
	if (imlog) {
		load_stream_config(m_logs[IMLOG], &imlog);
	}

	const auto msdplog = logging.child("msdplog");
	if (msdplog) {
		load_stream_config(m_logs[MSDP_LOG], &msdplog);
	}
	const auto moneylog = logging.child("moneylog");
	if (moneylog) {
		load_stream_config(m_logs[MONEY_LOG], &moneylog);
	}

	const auto separate_thread_node = logging.child("separate_thread");
	if (separate_thread_node) {
		m_output_thread = true;
		const auto queue_size_node = separate_thread_node.child("queue_size");
		const auto queue_size_string = queue_size_node.child_value();
		const auto queue_size = std::strtoul(queue_size_string, nullptr, 10);
		if (0 < queue_size) {
			m_output_queue_size = queue_size;
		} else {
			std::cerr << "Couldn't set queue size to value '" << queue_size_string
					  << "'. Leaving default value " << m_output_queue_size << "." << "\r\n";
		}
	}
}

void RuntimeConfiguration::load_features_configuration(const pugi::xml_node *root) {
	const auto features = root->child("features");
	if (!features) {
		return;
	}

	const auto msdp = features.child("msdp");
	if (msdp) {
		load_msdp_configuration(&msdp);
	}
}

void RuntimeConfiguration::setup_logs() {
	mkdir("log", 0700);
	mkdir("log/perslog", 0700);

	for (int i = 0; i < 1 + LAST_LOG; ++i) {
		auto stream = static_cast<EOutputStream>(i);

		constexpr int MAX_SRC_PATH_LENGTH = 4096;
		char src_path[MAX_SRC_PATH_LENGTH];
		const char *getcwd_result = getcwd(src_path, MAX_SRC_PATH_LENGTH);
		UNUSED_ARG(getcwd_result);

		if (logs(stream).filename().empty()) {
			handle(stream, stderr);
			puts("Using file descriptor for logging.");
			continue;
		}

		if (!runtime_config.open_log(stream))    //s_fp
		{
			puts("SYSERR: Couldn't open anything to log to, giving up.");
			exit(1);
		}
	}

	setup_converters();

	printf("Bylins server will use %schronous output into syslog file.\n",
		   output_thread() ? "asyn" : "syn");
}

void RuntimeConfiguration::load_msdp_configuration(const pugi::xml_node *msdp) {
	if (!msdp) {
		return;
	}
	const auto disabled = msdp->child("disabled");
	const auto disabled_value = disabled.child_value();
	if (disabled_value
		&& 0 == strcmp("true", disabled_value)) {
		m_msdp_disabled = true;
	} else {
		m_msdp_disabled = false;
	}
}

void RuntimeConfiguration::load_boards_configuration(const pugi::xml_node *root) {
	const auto boards = root->child("boards");
	if (!boards) {
		return;
	}

	const auto changelog = boards.child("changelog");
	if (!changelog) {
		return;
	}

	const auto format = changelog.child("format");
	if (format) {
		m_changelog_format = format.child_value();
		std::transform(m_changelog_format.begin(), m_changelog_format.end(), m_changelog_format.begin(), ::tolower);
	}
	const auto filename = changelog.child("filename");
	if (filename) {
		m_changelog_file_name = filename.child_value();
	}
}

void RuntimeConfiguration::load_external_triggers(const pugi::xml_node *root) {
	const auto external_triggers = root->child("external_triggers");
	if (!external_triggers) {
		return;
	}

	const auto reboot_value = external_triggers.child_value("reboot");
	if (reboot_value) {
		m_external_reboot_trigger_file_name = reboot_value;
	}
}

void RuntimeConfiguration::load_statistics_configuration(const pugi::xml_node *root) {
	const auto statistics = root->child("statistics");
	if (!statistics) {
		return;
	}

	const auto server = statistics.child("server");
	if (!server) {
		return;
	}

	std::string host;
	const auto host_value = server.child_value("host");
	if (host_value) {
		host = host_value;
	}

	unsigned short port = StatisticsConfiguration::DEFAULT_PORT;
	const auto port_value = server.child_value("port");
	if (port_value) {
		port = static_cast<unsigned short>(std::strtoul(port_value, nullptr, 10));
	}

	m_statistics = StatisticsConfiguration(host, port);
}

void RuntimeConfiguration::load_world_loader_configuration(const pugi::xml_node *root) {
	// Read YAML_THREADS environment variable (takes precedence)
	const char* env_threads = std::getenv("YAML_THREADS");
	if (env_threads) {
		size_t threads = static_cast<size_t>(std::strtoul(env_threads, nullptr, 10));
		if (threads > 0 && threads <= 64) {  // Sanity check
			m_yaml_threads = threads;
			return;
		}
	}

	// Fall back to XML configuration if available
	const auto world_loader = root->child("world_loader");
	if (!world_loader) {
		return;
	}

	const auto yaml_config = world_loader.child("yaml");
	if (yaml_config) {
		m_yaml_threads = static_cast<size_t>(std::strtoul(yaml_config.child_value("threads"), nullptr, 10));
	}
}

typedef std::map<EOutputStream, std::string> EOutputStream_name_by_value_t;
typedef std::map<const std::string, EOutputStream> EOutputStream_value_by_name_t;
EOutputStream_name_by_value_t EOutputStream_name_by_value;
EOutputStream_value_by_name_t EOutputStream_value_by_name;

void init_EOutputStream_ITEM_NAMES() {
	EOutputStream_name_by_value.clear();
	EOutputStream_value_by_name.clear();

	EOutputStream_name_by_value[SYSLOG] = "SYSLOG";
	EOutputStream_name_by_value[IMLOG] = "IMLOG";
	EOutputStream_name_by_value[ERRLOG] = "ERRLOG";
	EOutputStream_name_by_value[MSDP_LOG] = "MSDPLOG";
	EOutputStream_name_by_value[MONEY_LOG] = "MONEYLOG";

	for (const auto &i : EOutputStream_name_by_value) {
		EOutputStream_value_by_name[i.second] = i.first;
	}
}

template<>
EOutputStream ITEM_BY_NAME(const std::string &name) {
	if (EOutputStream_name_by_value.empty()) {
		init_EOutputStream_ITEM_NAMES();
	}
	return EOutputStream_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<EOutputStream>(const EOutputStream item) {
	if (EOutputStream_name_by_value.empty()) {
		init_EOutputStream_ITEM_NAMES();
	}
	return EOutputStream_name_by_value.at(item);
}

typedef std::map<CLogInfo::EBuffered, std::string> EBuffered_name_by_value_t;
typedef std::map<const std::string, CLogInfo::EBuffered> EBuffered_value_by_name_t;
EBuffered_name_by_value_t EBuffered_name_by_value;
EBuffered_value_by_name_t EBuffered_value_by_name;

void init_EBuffered_ITEM_NAMES() {
	EBuffered_name_by_value.clear();
	EBuffered_value_by_name.clear();

	EBuffered_name_by_value[CLogInfo::EBuffered::EB_FULL] = "FULL";
	EBuffered_name_by_value[CLogInfo::EBuffered::EB_LINE] = "LINE";
	EBuffered_name_by_value[CLogInfo::EBuffered::EB_NO] = "NO";

	for (const auto &i : EBuffered_name_by_value) {
		EBuffered_value_by_name[i.second] = i.first;
	}
}

template<>
CLogInfo::EBuffered ITEM_BY_NAME(const std::string &name) {
	if (EBuffered_name_by_value.empty()) {
		init_EBuffered_ITEM_NAMES();
	}
	return EBuffered_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<CLogInfo::EBuffered>(const CLogInfo::EBuffered item) {
	if (EBuffered_name_by_value.empty()) {
		init_EBuffered_ITEM_NAMES();
	}
	return EBuffered_name_by_value.at(item);
}

typedef std::map<CLogInfo::EMode, std::string> EMode_name_by_value_t;
typedef std::map<const std::string, CLogInfo::EMode> EMode_value_by_name_t;
EMode_name_by_value_t EMode_name_by_value;
EMode_value_by_name_t EMode_value_by_name;

void init_EMode_ITEM_NAMES() {
	EMode_name_by_value.clear();
	EMode_value_by_name.clear();

	EMode_name_by_value[CLogInfo::EMode::EM_REWRITE] = "REWRITE";
	EMode_name_by_value[CLogInfo::EMode::EM_APPEND] = "APPEND";

	for (const auto &i : EMode_name_by_value) {
		EMode_value_by_name[i.second] = i.first;
	}
}

template<>
CLogInfo::EMode ITEM_BY_NAME(const std::string &name) {
	if (EMode_name_by_value.empty()) {
		init_EMode_ITEM_NAMES();
	}
	return EMode_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<CLogInfo::EMode>(const CLogInfo::EMode item) {
	if (EMode_name_by_value.empty()) {
		init_EMode_ITEM_NAMES();
	}
	return EMode_name_by_value.at(item);
}

const char *RuntimeConfiguration::CONFIGURATION_FILE_NAME = "misc/configuration.xml";

const RuntimeConfiguration::logs_t LOGS({
											CLogInfo("syslog", "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫"),
											CLogInfo("log/errlog.txt", "О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫"),
											CLogInfo("log/imlog.txt", "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫"),
											CLogInfo("log/msdp.txt", "О©╫О©╫О©╫ MSDP О©╫О©╫О©╫О©╫О©╫О©╫О©╫"),
											CLogInfo("log/money.txt", "О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫")
										});

constexpr std::size_t RuntimeConfiguration::OUTPUT_QUEUE_SIZE;

RuntimeConfiguration::RuntimeConfiguration() :
	m_logs(LOGS),
	m_output_thread(false),
	m_output_queue_size(OUTPUT_QUEUE_SIZE),
	m_syslog_converter(nullptr),
	m_logging_enabled(true),
	m_msdp_disabled(false),
	m_msdp_debug(false),
	m_changelog_file_name(Boards::constants::CHANGELOG_FILE_NAME),
	m_changelog_format(Boards::constants::loader_formats::GIT),
	m_yaml_threads(0) {
}

void RuntimeConfiguration::load_from_file(const char *filename) {
	try {
		pugi::xml_document document;
		if (!document.load_file(filename)) {
			throw std::runtime_error("could not load XML configuration file");
		}

		const auto root = document.child("configuration");
		if (!root) {
			throw std::runtime_error("Root tag \"configuration\" not found");
		}

		load_logging_configuration(&root);
		load_features_configuration(&root);
		load_boards_configuration(&root);
		load_external_triggers(&root);
		load_statistics_configuration(&root);
		load_world_loader_configuration(&root);
#ifdef ENABLE_ADMIN_API
		load_admin_api_configuration(&root);
#endif
	}
	catch (const std::exception &e) {
		std::cerr << "ERROR: Failed to load configuration file " << filename << ": " << e.what() << "\r\n";
		std::cerr << "WARNING: Running with default configuration settings. YAML_THREADS will use hardware_concurrency().\r\n";
	}
	catch (...) {
		std::cerr << "ERROR: Unexpected error when loading configuration file " << filename << "\r\n";
		std::cerr << "WARNING: Running with default configuration settings.\r\n";
	}
}

#ifdef ENABLE_ADMIN_API
void RuntimeConfiguration::load_admin_api_configuration(const pugi::xml_node *root) {
	fprintf(stderr, "DEBUG: load_admin_api_configuration called\n");
	fflush(stderr);

	// List all children
	fprintf(stderr, "DEBUG: Root node children:\n");
	for (auto child = root->first_child(); child; child = child.next_sibling()) {
		fprintf(stderr, "DEBUG:   - %s\n", child.name());
	}
	fflush(stderr);

	const auto admin_api = root->child("admin_api");
	if (!admin_api) {
		fprintf(stderr, "DEBUG: admin_api node not found\n");
		fflush(stderr);
		return;
	}
	fprintf(stderr, "DEBUG: admin_api node found\n");
	fflush(stderr);

	const auto enabled_value = admin_api.child_value("enabled");
	fprintf(stderr, "DEBUG: enabled_value = '%s'\n", enabled_value ? enabled_value : "NULL");
	fflush(stderr);
	if (enabled_value && std::string(enabled_value) == "true") {
		fprintf(stderr, "DEBUG: enabling Admin API\n");
		fflush(stderr);
		m_admin_api_enabled = true;
	}

	const auto socket_path_value = admin_api.child_value("socket_path");
	fprintf(stderr, "DEBUG: socket_path_value = '%s'\n", socket_path_value ? socket_path_value : "NULL");
	fflush(stderr);
	if (socket_path_value) {
		m_admin_socket_path = socket_path_value;
	}

	const auto port_value = admin_api.child_value("port");
	fprintf(stderr, "DEBUG: port_value = '%s'\n", port_value ? port_value : "NULL");
	fflush(stderr);
	if (port_value && strlen(port_value) > 0) {
		m_admin_port = static_cast<unsigned short>(std::strtoul(port_value, nullptr, 10));
	}

	const auto require_auth_value = admin_api.child_value("require_auth");
	fprintf(stderr, "DEBUG: require_auth_value = '%s'\n", require_auth_value ? require_auth_value : "NULL");
	fflush(stderr);
	if (require_auth_value && std::string(require_auth_value) == "false") {
		m_admin_require_auth = false;
		fprintf(stderr, "DEBUG: Setting require_auth = false\n");
		fflush(stderr);
	}
}
#endif

class UMaskToggle {
 public:
	UMaskToggle(const umask_t umask_value) : m_umask(set_umask(umask_value)) {}
	~UMaskToggle() { set_umask(m_umask); }

	static umask_t set_umask(const umask_t umask_value) {
#if defined CIRCLE_UNIX
		return CLogInfo::UMASK_DEFAULT == umask_value ? umask_value : umask(umask_value);
#else
		return umask_value;
#endif
	}

 private:
	umask_t m_umask;
};

bool CLogInfo::open() {
	UMaskToggle umask_toggle(this->umask());

	const char *mode = EM_APPEND == this->mode() ? "a+" : "w";
	FILE *handle = fopen(filename().c_str(), mode);

	if (handle) {
		setvbuf(handle, m_buffer, buffered(), BUFFER_SIZE);

		m_handle = handle;
		printf("Using log file '%s' with %s buffering. Opening in %s mode.\n",
			   filename().c_str(),
			   NAME_BY_ITEM(buffered()).c_str(),
			   NAME_BY_ITEM(this->mode()).c_str());
		return true;
	}

	fprintf(stderr, "SYSERR: Error opening file '%s': %s\n", filename().c_str(), strerror(errno));

	return false;
}

RuntimeConfiguration runtime_config;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
