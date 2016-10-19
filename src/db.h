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
#include "boot.constants.hpp"
#include "utils.h"
#include "structs.h"
#include "conf.h"	// to get definition of build type: (CIRCLE_AMIGA|CIRCLE_UNIX|CIRCLE_WINDOWS|CIRCLE_ACORN|CIRCLE_VMS)

#include <boost/array.hpp>
#include <map>

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.
class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

// room manage functions
void room_copy(ROOM_DATA * dst, ROOM_DATA * src);
void room_free(ROOM_DATA * room);

// public procedures in db.cpp
void tag_argument(char *argument, char *tag);
void boot_db(void);
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
void init_char(CHAR_DATA *ch);
CHAR_DATA *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int vnum_mobile(char *searchname, CHAR_DATA * ch);
void clear_char_skills(CHAR_DATA * ch);
int correct_unique(int unique);
bool check_unlimited_timer(const CObjectPrototype* obj);

#define REAL          0
#define VIRTUAL       (1 << 0)
#define OBJ_NO_CALC   (1 << 1)

OBJ_DATA *create_obj(const std::string& alias = "");
void free_obj(OBJ_DATA * obj);
obj_rnum real_object(obj_vnum vnum);
OBJ_DATA *read_object(obj_vnum nr, int type);
CObjectPrototype::shared_ptr get_object_prototype(obj_vnum nr, int type = VIRTUAL);

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

// для экстраффектов в random_obj
struct ExtraAffects
{
	int number; // номер экстрааафетка
	int min_val; // минимальное значение
	int max_val; // максимальное значение
	int chance; // вероятность того, что данный экстраффект будет на шмотке
};

class RandomObj
{
	public:
		// внум объекта
		int vnum;
		// массив, в котором показывается, кому шмотка недоступна + шанс, что эта "недоступность" при выпадении предмета будет на нем
		std::map<std::string, int> not_wear;
		// минимальный и максимальный вес
		int min_weight;
		int max_weight;
		// минимальная и максимальная цена за предмет
		int min_price;
		int max_price;
		// прочность
		int min_stability;
		int max_stability;
		// value0, value1, value2, value3
		int value0_min, value1_min, value2_min, value3_min;
		int value0_max, value1_max, value2_max, value3_max;
		// список аффектов и их шанс упасть на шмотку
		std::map<std::string, int> affects;
		// список экстраффектов и их шанс упасть на шмотку
		std::vector<ExtraAffects> extraffect;
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
	// Местоположение зоны
	char *location;
	// Описание зоны
	char *description;
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
	// показывает количество репопов зоны, при условии, что в зону ходят
	int count_reset;
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
extern int top_of_p_table;

class CObjectPrototypes
{
private:
	struct SPrototypeIndex
	{
		SPrototypeIndex() : number(0), stored(0), func(NULL), farg(NULL), proto(NULL), zone(0), set_idx(-1) {}

		int number;		// number of existing units of this mob/obj //
		int stored;		// number of things in rent file            //
		int(*func)(CHAR_DATA*, void*, int, char*);
		char *farg;		// string argument for special function     //
		TRIG_DATA *proto;	// for triggers... the trigger     //
		int zone;			// mob/obj zone rnum //
		size_t set_idx; // индекс сета в obj_sets::set_list, если != -1
	};

public:
	using prototypes_t = std::deque<CObjectPrototype::shared_ptr>;
	using const_iterator = prototypes_t::const_iterator;

	using index_t = std::deque<SPrototypeIndex>;

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

	size_t add(CObjectPrototype* prototype, const obj_vnum vnum);
	size_t add(const CObjectPrototype::shared_ptr& prototype, const obj_vnum vnum);

	void zone(const size_t rnum, const size_t zone_rnum) { m_index[rnum].zone = static_cast<int>(zone_rnum); }

	auto stored(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].stored : -1; }
	auto stored(const CObjectPrototype::shared_ptr& object) const { return stored(object->get_rnum()); }
	void dec_stored(const size_t rnum) { --m_index[rnum].stored; }
	void inc_stored(const size_t rnum) { ++m_index[rnum].stored; }

	auto number(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].number : -1; }
	auto number(const CObjectPrototype::shared_ptr& object) const { return number(object->get_rnum()); }
	void dec_number(const size_t rnum);
	void inc_number(const size_t rnum) { ++m_index[rnum].number; }

	auto zone(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].zone : -1; }

	auto actual_count(const size_t rnum) const { return number(rnum) + stored(rnum); }

	auto func(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].func : nullptr; }
	void func(const size_t rnum, const decltype(SPrototypeIndex::func) function) { m_index[rnum].func = function; }

	auto spec(const size_t rnum) const { return func(rnum); }

	auto set_idx(const size_t rnum) const { return is_index_safe(rnum) ? m_index[rnum].set_idx : ~0; }
	void set_idx(const size_t rnum, const decltype(SPrototypeIndex::set_idx) value) { m_index[rnum].set_idx = value; }

	int rnum(const obj_vnum vnum) const;

	void set(const size_t index, CObjectPrototype* new_value);

	auto index_size() const { return m_index.size()*(sizeof(index_t::value_type) + sizeof(vnum2index_t::value_type)); }
	auto prototypes_size() const { return m_prototypes.size()*sizeof(prototypes_t::value_type); }

	const auto& proto_script(const size_t rnum) const { return m_prototypes.at(rnum)->get_proto_script(); }
	const auto& vnum2index() const { return m_vnum2index; }

private:
	using vnum2index_t = std::map<obj_vnum, size_t>;

	bool is_index_safe(const size_t index) const;

	prototypes_t m_prototypes;
	index_t m_index;
	vnum2index_t m_vnum2index;
};

inline bool CObjectPrototypes::is_index_safe(const size_t index) const
{
	return index < m_index.size();
}

extern CObjectPrototypes obj_proto;

inline obj_vnum GET_OBJ_VNUM(const CObjectPrototype* obj) { return obj->get_vnum(); }
inline auto GET_OBJ_SPEC(const CObjectPrototype* obj) { return obj_proto.spec(obj->get_rnum()); }

// returns the real number of the object with given virtual number
inline obj_rnum real_object(obj_vnum vnum) { return obj_proto.rnum(vnum); }

extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *mob_proto;
extern const char *MENU;

extern struct portals_list_type *portals_list;
extern TIME_INFO_DATA time_info;

extern int convert_drinkcon_skill(CObjectPrototype *obj, bool proto);

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

class WorldLoader
{
	public:
		WorldLoader();
		void boot_world();
		void index_boot(const EBootType mode);
};

extern WorldLoader world_loader;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
