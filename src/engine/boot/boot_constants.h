#ifndef __BOOT_CONSTANTS_HPP__
#define __BOOT_CONSTANTS_HPP__

#include "engine/core/sysdep.h"
#include <string>
#include <unordered_map>

// arbitrary constants used by BootIndex() (must be unique)
const int kMaxProtoNumber = 9999999;    //Максимально возможный номер комнаты, предмета и т.д.

#define MIN_ZONE_LEVEL    1
#define MAX_ZONE_LEVEL    50

#define DUPLICATE_MINI_SET_VNUM 1000000

enum SetStuffMode {
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
#define LIB_CFG		":cfg:"
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
#define LIB_CFG         "cfg/"
#define LIB_MISC      "misc/"
#define LIB_ACCOUNTS  "plrs/accounts/"
#define LIB_MISC_CRAFT        "misc/crafts/"
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

#define TEXT_SUF_OBJS    "textobjs"
#define TIME_SUF_OBJS    "timeobjs"
#define SUF_ALIAS    "alias"
#define SUF_MEM        "mem"
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
#define FASTBOOT_FILE   "../.fastboot"    // autorun: boot without sleep
#define KILLSCRIPT_FILE "../.killscript"    // autorun: shut mud down
#define PAUSE_FILE      "../pause"    // autorun: don't restart mud
#endif

// names of various files and directories
#define INDEX_FILE    "index"    // index of world files
#define MINDEX_FILE    "index.mini"    // ... and for mini-mud-mode
#define WLD_PREFIX    LIB_WORLD "wld" SLASH    // room definitions
#define MOB_PREFIX    LIB_WORLD "mob" SLASH    // monster prototypes
#define OBJ_PREFIX    LIB_WORLD "obj" SLASH    // object prototypes
#define ZON_PREFIX    LIB_WORLD "zon" SLASH    // zon defs & command tables
#define TRG_PREFIX    LIB_WORLD "trg" SLASH    // shop definitions
#define HLP_PREFIX    LIB_TEXT "help" SLASH    // for HELP <keyword>
#define PLAYER_F_PREFIX LIB_PLRS "" LIB_F
#define PLAYER_K_PREFIX LIB_PLRS "" LIB_K
#define PLAYER_P_PREFIX LIB_PLRS "" LIB_P
#define PLAYER_U_PREFIX LIB_PLRS "" LIB_U
#define PLAYER_Z_PREFIX LIB_PLRS "" LIB_Z

#define HELP_PAGE_FILE    LIB_TEXT_HELP "screen"    // for HELP <CR>
#define HANDBOOK_FILE    LIB_TEXT "handbook"    // handbook for new immorts

#define PROXY_FILE        LIB_MISC "proxy"    // register proxy list
#define XNAME_FILE      LIB_MISC "xnames"    // invalid name substrings
#define ANAME_FILE      LIB_MISC "apr_name" // одобренные имена
#define DNAME_FILE      LIB_MISC "dis_name" // запрещенные имена
#define NNAME_FILE      LIB_MISC "new_name" // ждущие одобрения
// issue.daily-quest: daily_quest.xml перенесён в cfg/quests и грузится через CfgManager.

#define MAIL_FILE        LIB_ETC "plrmail"    // for the mudmail system
#define BAN_FILE        LIB_ETC "badsites"    // for the siteban system
#define PROXY_BAN_FILE    LIB_ETC "badproxy"    // for the siteban system

#define WHOLIST_FILE    LIB_STAT "wholist.html"    // for the stat system

enum EBootType : int {
	DB_BOOT_WLD = 0,
	DB_BOOT_MOB = 1,
	DB_BOOT_OBJ = 2,
	DB_BOOT_ZON = 3,
	DB_BOOT_HLP = 4,
	DB_BOOT_TRG = 5
};

class FilesPrefixes : private std::unordered_map<int, std::string> {
 public:
	FilesPrefixes();
	const std::string &operator()(const EBootType mode) const;

 private:
	static std::string s_empty_prefix;
};

extern FilesPrefixes prefixes;

const char *boot_mode_name(EBootType mode);

#define READ_SIZE 256

#endif // __BOOT_CONSTANTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
