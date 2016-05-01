/* ************************************************************************
*   File: db.h                                          Part of Bylins    *
*  Usage: header file for database handling                               *
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

#ifndef _DB_H_
#define _DB_H_

#include "obj.hpp"
#include "utils.h"
#include "structs.h"
#include "conf.h"	// to get definition of build type: (CIRCLE_AMIGA|CIRCLE_UNIX|CIRCLE_WINDOWS|CIRCLE_ACORN|CIRCLE_VMS)

#include <boost/array.hpp>
#include <map>

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.
class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

// arbitrary constants used by index_boot() (must be unique)
#define MAX_PROTO_NUMBER 999999	//Максимально возможный номер комнаты, предмета и т.д.

#define MIN_ZONE_LEVEL	1
#define MAX_ZONE_LEVEL	50

#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_HLP	4
#define DB_BOOT_TRG	5
#define DB_BOOT_SOCIAL 6

#define DL_LOAD_ANYWAY     0
#define DL_LOAD_IFLAST     1
#define DL_LOAD_ANYWAY_NC  2
#define DL_LOAD_IFLAST_NC  3

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

// room manage functions
void room_copy(ROOM_DATA * dst, ROOM_DATA * src);
void room_free(ROOM_DATA * room);

// public procedures in db.cpp
void tag_argument(char *argument, char *tag);
void boot_db(void);
void free_db(void);
int create_entry(const char *name);
void zone_update(void);
bool can_be_reset(zone_rnum zone);
room_rnum real_room(room_vnum vnum);
char *fread_string(FILE * fl, char *error);
long get_id_by_name(char *name);
long get_id_by_uid(long uid);
int get_uid_by_id(int id);
long cmp_ptable_by_name(char *name, int len);
const char *get_name_by_id(long id);
char* get_name_by_unique(int unique);
int get_level_by_unique(long unique);
long get_lastlogon_by_unique(long unique);
long get_ptable_by_unique(long unique);
int get_zone_rooms(int, int *, int *);

int load_char(const char *name, CHAR_DATA * char_element, bool reboot = 0);
void save_char(CHAR_DATA *ch);
void init_char(CHAR_DATA *ch);
CHAR_DATA *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int vnum_mobile(char *searchname, CHAR_DATA * ch);
void reset_char(CHAR_DATA * ch);
void clear_char_skills(CHAR_DATA * ch);
int correct_unique(int unique);

#define REAL          0
#define VIRTUAL       (1 << 0)
#define OBJ_NO_CALC   (1 << 1)

OBJ_DATA *create_obj(const char *alias = 0);
void free_obj(OBJ_DATA * obj);
obj_rnum real_object(obj_vnum vnum);
OBJ_DATA *read_object(obj_vnum nr, int type);
const OBJ_DATA* read_object_mirror(obj_vnum nr, int type = VIRTUAL);

int vnum_object(char *searchname, CHAR_DATA * ch);
int vnum_flag(char *searchname, CHAR_DATA * ch);
int vnum_room(char *searchname, CHAR_DATA * ch);

// structure for the reset commands
struct reset_com
{
	char command;		// current command

	int if_flag;		// 4 modes of command execution
	int arg1;		//
	int arg2;		// Arguments to the command
	int arg3;		//
	int arg4;
	int line;		// line number this command appears on
	char *sarg1;		// string argument
	char *sarg2;		// string argument

	/*
	 *  Commands:              *
	 *  'M': Read a mobile     *
	 *  'O': Read an object    *
	 *  'G': Give obj to mob   *
	 *  'P': Put obj in obj    *
	 *  'G': Obj to char       *
	 *  'E': Obj to char equip *
	 *  'D': Set state of door *
	 *  'T': Trigger command   *
	 */
};

struct _case
{
	// внум сундука
	int vnum;
	// шанс выпадаения
	int chance;
	// внумы шмоток, которые выпадают из кейса
	std::vector<int> vnum_objs;
};

// zone definition structure. for the 'zone-table'
struct zone_data
{
	char *name;		// name of this zone
	// автор, дата...
	char *comment;
//MZ.load
	int level;	// level of this zone (is used in ingredient loading)
	int type;	// the surface type of this zone (is used in ingredient loading)
//-MZ.load

	int lifespan;		// how long between resets (minutes)
	int age;		// current age of this zone (minutes)
	room_vnum top;		// upper limit for rooms in this zone

	int reset_mode;		// conditions for reset (see below)
	zone_vnum number;	// virtual number of this zone
	struct reset_com *cmd;	// command table for reset

	/*
	 * Reset mode:
	 *   0: Don't reset, and don't update age.
	 *   1: Reset if no PC's are located in zone.
	 *   2: Just reset.
	 *   3: Multi reset.
	 */
	int typeA_count;
	int *typeA_list;	// список номеров зон, которые сбрасываются одновременно с этой
	int typeB_count;
	int *typeB_list;	// список номеров зон, которые сбрасываются независимо, но они должны быть сброшены до сброса зон типа А
	bool *typeB_flag;	// флаги, были ли сброшены зоны типа В
	int under_construction;	// зона в процессе тестирования
	bool locked;
	bool reset_idle;	// очищать ли зону, в которой никто не бывал
	bool used;		// был ли кто-то в зоне после очистки
	unsigned long long activity;	// параметр активности игроков в зоне
	// <= 1 - обычная зона (соло), >= 2 - зона для группы из указанного кол-ва человек
	int group;
	// средний уровень мобов в зоне
	int mob_level;
	// является ли зона городом
	bool is_town;
};

extern zone_data *zone_table;

// for queueing zones for update
struct reset_q_element
{
	zone_rnum zone_to_reset;	// ref to zone_data
	struct reset_q_element *next;
};



// structure for the update queue
struct reset_q_type
{
	struct reset_q_element *head;
	struct reset_q_element *tail;
};

#define OBJECT_SAVE_ACTIVITY 300
#define PLAYER_SAVE_ACTIVITY 300
#define MAX_SAVED_ITEMS      1000

struct player_index_element
{
	char *name;
	//added by WorM индексируюца еще мыло и последний айпи
	char *mail;
	char *last_ip;
	//end by WorM
	int id;
	int unique;
	int level;
	int last_logon;
	int activity;		// When player be saved and checked
	save_info *timer;
};

#define SEASON_WINTER		0
#define SEASON_SPRING		1
#define SEASON_SUMMER		2
#define SEASON_AUTUMN		3

#define MONTH_JANUARY   	0
#define MONTH_FEBRUARY  	1
#define MONTH_MART			2
#define MONTH_APRIL			3
#define MONTH_MAY			4
#define MONTH_JUNE			5
#define MONTH_JULY			6
#define MONTH_AUGUST		7
#define MONTH_SEPTEMBER		8
#define MONTH_OCTOBER		9
#define MONTH_NOVEMBER		10
#define MONTH_DECEMBER		11
#define DAYS_PER_WEEK		7

struct month_temperature_type
{
	int min;
	int max;
	int med;
};

//Polud тестовый класс для хранения параметров различных рас мобов
struct ingredient
{
	int imtype;
	std::string imname;
	boost::array<int, MAX_MOB_LEVEL + 1> prob; // вероятность загрузки для каждого уровня моба
};

class MobRace{
public:
	MobRace();
	~MobRace();
	std::string race_name;
	std::vector<ingredient> ingrlist;
};

typedef boost::shared_ptr<MobRace> MobRacePtr;
typedef std::map<int, MobRacePtr> MobRaceListType;

//-Polud

extern room_rnum top_of_world;
extern struct player_special_data dummy_mob;

#ifndef __CONFIG_C__
extern char const *OK;
extern char const *NOPERSON;
extern char const *NOEFFECT;
#endif

// external variables

extern const int sunrise[][2];
extern const int Reverse[];

// external vars
extern CHAR_DATA *combat_list;

#include <vector>
#include <deque>

using CRooms = std::deque<ROOM_DATA*>;

extern CRooms world;
extern CHAR_DATA *character_list;
extern OBJ_DATA *object_list;
extern INDEX_DATA *mob_index;
extern mob_rnum top_of_mobt;

class CObjectPrototypes
{
public:
	using prototypes_t = std::deque<OBJ_DATA *>;
	using const_iterator = prototypes_t::const_iterator;

	using index_t = std::deque<index_data>;

	/**
	** \name Proxy calls to std::vector member functions.
	**
	** This methods just perform proxy calls to corresponding functions of the m_prototype field.
	**
	** @{
	*/
	auto begin() const { return m_prototypes.begin(); }
	auto end() const { return m_prototypes.end(); }
	auto size() const { return m_prototypes.size(); }
	const auto& at(const size_t index) const { return m_prototypes.at(index); }

	const auto& operator[](size_t index) const { return m_prototypes[index]; }
	/** @} */

	size_t add(const prototypes_t::value_type& prototype, const obj_vnum vnum);

	obj_vnum vnum(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].vnum : -1; }
	obj_vnum vnum(const OBJ_DATA* object) const { return vnum(object->item_number); }
	void vnum(const size_t rnum, const obj_vnum value);

	void zone(const size_t rnum, const size_t zone_rnum) { m_index[rnum].zone = static_cast<int>(zone_rnum); }

	auto stored(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].stored : -1; }
	auto stored(const OBJ_DATA* object) const { return stored(object->item_number); }
	void dec_stored(const size_t rnum) { --m_index[rnum].stored; }
	void inc_stored(const size_t rnum) { ++m_index[rnum].stored; }

	auto number(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].number : -1; }
	auto number(const OBJ_DATA* object) const { return number(object->item_number); }
	void dec_number(const size_t rnum) { --m_index[rnum].number; }
	void inc_number(const size_t rnum) { ++m_index[rnum].number; }

	auto zone(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].zone : -1; }

	auto actual_count(const size_t rnum) const { return number(rnum) + stored(rnum); }
	auto actual_count(const OBJ_DATA* object) const { return actual_count(object->item_number); }

	auto func(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].func : nullptr; }
	auto func(const OBJ_DATA* object) const { return func(object->item_number); }
	void func(const size_t rnum, const decltype(index_data::func) function) { m_index[rnum].func = function; }

	auto spec(const OBJ_DATA* object) const { return func(object->item_number); }

	auto set_idx(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].set_idx : ~0; }
	void set_idx(const size_t rnum, const decltype(index_data::set_idx) value) { m_index[rnum].set_idx = value; }

	int rnum(const obj_vnum vnum) const;

	prototypes_t::value_type swap(const size_t index, const prototypes_t::value_type& new_value);

	auto index_size() const { return m_index.size()*(sizeof(index_t::value_type) + sizeof(vnum2index_t::value_type)); }
	auto prototypes_size() const { return m_prototypes.size()*sizeof(prototypes_t::value_type); }

	const auto& proto_script(const size_t rnum) const { return m_prototypes.at(rnum)->proto_script; }

private:
	using vnum2index_t = std::map<obj_vnum, size_t>;

	bool is_index_safe(const size_t index) const;

	prototypes_t m_prototypes;
	index_t m_index;
	vnum2index_t m_vnum2index;
};

inline bool CObjectPrototypes::is_index_safe(const size_t index) const
{
	/*
	if (index >= m_index.size)
	{
		// Uncomment and set breakpoint here to find places where OBJ_DATA instance does not contain a valid RNUM value.
		log("WARNING: passed RNUM is invalid.\n");
	}
	*/
	return index < m_index.size();
}

extern CObjectPrototypes obj_proto;

inline obj_vnum GET_OBJ_VNUM(const OBJ_DATA* obj) { return obj_proto.vnum(obj); }
inline auto GET_OBJ_SPEC(const OBJ_DATA* obj) { return obj_proto.spec(obj); }

// returns the real number of the object with given virtual number
inline obj_rnum real_object(obj_vnum vnum) { return obj_proto.rnum(vnum); }

extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *mob_proto;
extern const char *MENU;

extern struct portals_list_type *portals_list;
extern TIME_INFO_DATA time_info;

extern int convert_drinkcon_skill(OBJ_DATA *obj, bool proto);

int dl_parse(load_list ** dl_list, char *line);
int dl_load_obj(OBJ_DATA * corpse, CHAR_DATA * ch, CHAR_DATA * chr, int DL_LOAD_TYPE);
int trans_obj_name(OBJ_DATA * obj, CHAR_DATA * ch);
void dl_list_copy(load_list * *pdst, load_list * src);
void paste_mobiles();

extern room_rnum r_helled_start_room;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

long get_ptable_by_name(const char *name);
void free_alias(struct alias_data *a);
extern player_index_element* player_table;

inline save_info* SAVEINFO(const size_t number)
{
	return player_table[number].timer;
}

inline void clear_saveinfo(const size_t number)
{
	delete player_table[number].timer;
	player_table[number].timer = NULL;
}

inline void recreate_saveinfo(const size_t number)
{
	delete player_table[number].timer;
	NEWCREATE(player_table[number].timer);
}

void set_god_skills(CHAR_DATA *ch);
void check_room_flags(int rnum);

namespace OfftopSystem
{

void init();
void set_flag(CHAR_DATA *ch);

} // namespace OfftopSystem

extern int now_entrycount;
void load_ignores(CHAR_DATA * ch, char *line);
void delete_char(const char *name);

void set_test_data(CHAR_DATA *mob);

extern zone_rnum top_of_zone_table;
void set_zone_mob_level();

bool can_snoop(CHAR_DATA *imm, CHAR_DATA *vict);

extern insert_wanted_gem iwg;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
