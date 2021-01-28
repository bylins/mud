#ifndef __BOOT_CONSTANTS_HPP__
#define __BOOT_CONSTANTS_HPP__

#include "sysdep.h"
#include <string>
#include <unordered_map>

// arbitrary constants used by index_boot() (must be unique)
#define MAX_PROTO_NUMBER 9999999	//Максимально возможный номер комнаты, предмета и т.д.

#define MIN_ZONE_LEVEL	1
#define MAX_ZONE_LEVEL	50

#define DL_LOAD_ANYWAY     0
#define DL_LOAD_IFLAST     1
#define DL_LOAD_ANYWAY_NC  2
#define DL_LOAD_IFLAST_NC  3

#define DUPLICATE_MINI_SET_VNUM 1000000

enum SetStuffMode
{
	SETSTUFF_SNUM,
	SETSTUFF_NAME,
	SETSTUFF_ALIS,
	SETSTUFF_VNUM,
	SETSTUFF_OQTY,
	SETSTUFF_CLSS,
	SETSTUFF_AMSG,
	SETSTUFF_DMSG,
	SETSTUFF_RAMG,
	SETSTUFF_RDMG,
	SETSTUFF_AFFS,
	SETSTUFF_AFCN,
};

#define LIB_A       "A-E"
#define LIB_F       "F-J"
#define LIB_K       "K-O"
#define LIB_P       "P-T"
#define LIB_U       "U-Z"
#define LIB_Z       "ZZZ"

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD	":world:"
#define LIB_TEXT	":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC	":misc:"
#define LIB_MISC_MOBRACES	":misc:mobraces:"
#define LIB_MISC_CRAFT		":misc:craft:"
#define LIB_ETC		":etc:"
#define ETC_BOARD	":etc:board:"
#define LIB_PLROBJS	":plrobjs:"
#define LIB_PLRS    ":plrs:"
#define LIB_PLRALIAS	":plralias:"
#define LIB_PLRSTUFF ":plrstuff:"
#define LIB_HOUSE	":plrstuff:house:"
#define LIB_PLRVARS	":plrvars:"
#define LIB             ":lib:"
#define SLASH		":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD     "world/"
#define LIB_TEXT      "text/"
#define LIB_TEXT_HELP "text/help/"
#define LIB_MISC      "misc/"
#define LIB_ACCOUNTS  "plrs/accounts/"
#define LIB_MISC_MOBRACES	"misc/mobraces/"
#define LIB_MISC_CRAFT		"misc/craft/"
#define LIB_STAT      "stat/"
#define LIB_ETC       "etc/"
#define ETC_BOARD     "etc/board/"
#define LIB_PLRS      "plrs/"
#define LIB_PLROBJS   "plrobjs/"
#define LIB_PLRALIAS  "plralias/"
#define LIB_PLRSTUFF  "plrstuff/"
#define LIB_PLRVARS   "plrvars/"
#define LIB_HOUSE     LIB_PLRSTUFF"house/"
#define LIB_DEPOT     LIB_PLRSTUFF"depot/"
#define LIB           "lib/"
#define SLASH         "/"
#else
#error "Unknown path components."
#endif

#define TEXT_SUF_OBJS	"textobjs"
#define TIME_SUF_OBJS	"timeobjs"
#define SUF_ALIAS	"alias"
#define SUF_MEM		"mem"
#define SUF_PLAYER  "player"
#define SUF_PERS_DEPOT "pers"
#define SUF_SHARE_DEPOT "share"
#define SUF_PURGE_DEPOT "purge"

#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"	// autorun: boot without sleep
#define KILLSCRIPT_FILE "/.killscript"	// autorun: shut mud down
#define PAUSE_FILE      "/pause"	// autorun: don't restart mud
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"	// autorun: boot without sleep
#define KILLSCRIPT_FILE	"::.killscript"	// autorun: shut mud down
#define PAUSE_FILE	"::pause"	// autorun: don't restart mud
#else
#define FASTBOOT_FILE   "../.fastboot"	// autorun: boot without sleep
#define KILLSCRIPT_FILE "../.killscript"	// autorun: shut mud down
#define PAUSE_FILE      "../pause"	// autorun: don't restart mud
#endif

// names of various files and directories
#define INDEX_FILE	"index"	// index of world files
#define MINDEX_FILE	"index.mini"	// ... and for mini-mud-mode
#define WLD_PREFIX	LIB_WORLD "wld" SLASH	// room definitions
#define MOB_PREFIX	LIB_WORLD "mob" SLASH	// monster prototypes
#define OBJ_PREFIX	LIB_WORLD "obj" SLASH	// object prototypes
#define ZON_PREFIX	LIB_WORLD "zon" SLASH	// zon defs & command tables
#define TRG_PREFIX	LIB_WORLD "trg" SLASH	// shop definitions
#define HLP_PREFIX	LIB_TEXT "help" SLASH	// for HELP <keyword>
#define SOC_PREFIX	LIB_MISC
#define PLAYER_F_PREFIX LIB_PLRS "" LIB_F
#define PLAYER_K_PREFIX LIB_PLRS "" LIB_K
#define PLAYER_P_PREFIX LIB_PLRS "" LIB_P
#define PLAYER_U_PREFIX LIB_PLRS "" LIB_U
#define PLAYER_Z_PREFIX LIB_PLRS "" LIB_Z

#define CREDITS_FILE	LIB_TEXT "credits"	// for the 'credits' command
#define MOTD_FILE       LIB_TEXT "motd"	// messages of the day / mortal
#define RULES_FILE      LIB_TEXT "rules"	// rules for immort
#define GREETINGS_FILE	LIB_TEXT "greetings"	// The opening screen.
#define HELP_PAGE_FILE	LIB_TEXT_HELP "screen"	// for HELP <CR>
#define INFO_FILE       LIB_TEXT "info"	// for INFO
#define IMMLIST_FILE	LIB_TEXT "immlist"	// for IMMLIST
#define BACKGROUND_FILE	LIB_TEXT "background"	// for the background story
#define POLICIES_FILE	LIB_TEXT "policies"	// player policies/rules
#define HANDBOOK_FILE	LIB_TEXT "handbook"	// handbook for new immorts
#define NAME_RULES_FILE LIB_TEXT "namerules" // rules of character's names

#define PROXY_FILE	    LIB_MISC "proxy"	// register proxy list
#define IDEA_FILE	    LIB_MISC "ideas"	// for the 'idea'-command
#define TYPO_FILE	    LIB_MISC "typos"	//         'typo'
#define BUG_FILE	    LIB_MISC "bugs"	//         'bug'
#define MESS_FILE       LIB_MISC "messages"	// damage messages
#define SOCMESS_FILE    LIB_MISC "socials"	// messgs for social acts
#define XNAME_FILE      LIB_MISC "xnames"	// invalid name substrings
#define ANAME_FILE      LIB_MISC "apr_name" // одобренные имена
#define DNAME_FILE      LIB_MISC "dis_name" // запрещенные имена
#define NNAME_FILE      LIB_MISC "new_name" // ждущие одобрения

#define MAIL_FILE	    LIB_ETC "plrmail"	// for the mudmail system
#define BAN_FILE	    LIB_ETC "badsites"	// for the siteban system
#define PROXY_BAN_FILE	LIB_ETC "badproxy"	// for the siteban system

#define WHOLIST_FILE    LIB_STAT "wholist.html"	// for the stat system

//Dead load (dl_load) options
#define DL_ORDINARY    0
#define DL_PROGRESSION 1
#define DL_SKIN        2

enum EBootType: int
{
	DB_BOOT_WLD = 0,
	DB_BOOT_MOB = 1,
	DB_BOOT_OBJ = 2,
	DB_BOOT_ZON = 3,
	DB_BOOT_HLP = 4,
	DB_BOOT_TRG = 5,
	DB_BOOT_SOCIAL = 6
};

class FilesPrefixes : private std::unordered_map<int, std::string>
{
public:
	FilesPrefixes();
	const std::string& operator()(const EBootType mode) const;

private:
	static std::string s_empty_prefix;
};

extern FilesPrefixes prefixes;

const char *boot_mode_name(EBootType mode);

#define READ_SIZE 256

#endif // __BOOT_CONSTANTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
