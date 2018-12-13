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
#include "structs.h"
#include "conf.h"	// to get definition of build type: (CIRCLE_AMIGA|CIRCLE_UNIX|CIRCLE_WINDOWS|CIRCLE_ACORN|CIRCLE_VMS)

#include <map>
#include <list>
#include <memory>

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.
class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

// room manage functions
void room_copy(ROOM_DATA * dst, ROOM_DATA * src);
void room_free(ROOM_DATA * room);

// public procedures in db.cpp
void tag_argument(char *argument, char *tag);
void boot_db(void);
void zone_update(void);
bool can_be_reset(zone_rnum zone);
room_rnum real_room(room_vnum vnum);
long get_id_by_name(char *name);
long get_id_by_uid(long uid);
int get_uid_by_id(int id);
long cmp_ptable_by_name(char *name, int len);
const char *get_name_by_id(long id);
const char* get_name_by_unique(int unique);
int get_level_by_unique(long unique);
long get_lastlogon_by_unique(long unique);
long get_ptable_by_unique(long unique);
int get_zone_rooms(int, int *, int *);

int load_char(const char *name, CHAR_DATA * char_element, bool reboot = 0, const bool find_id = true);
CHAR_DATA *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int vnum_mobile(char *searchname, CHAR_DATA * ch);
void clear_char_skills(CHAR_DATA * ch);
int correct_unique(int unique);
bool check_unlimited_timer(const CObjectPrototype* obj);
void SaveGlobalUID(void);
void flush_player_index(void);

#define REAL          0
#define VIRTUAL       (1 << 0)
#define OBJ_NO_CALC   (1 << 1)

CObjectPrototype::shared_ptr get_object_prototype(obj_vnum nr, int type = VIRTUAL);

int vnum_object(char *searchname, CHAR_DATA * ch);
int vnum_flag(char *searchname, CHAR_DATA * ch);
int vnum_room(char *searchname, CHAR_DATA * ch);

// structure for the reset commands
struct reset_com
{
	/**
	 *  Commands:
	 *  'M': Read a mobile
	 *  'O': Read an object
	 *  'G': Give obj to mob
	 *  'P': Put obj in obj
	 *  'G': Obj to char
	 *  'E': Obj to char equip
	 *  'D': Set state of door
	 *  'T': Trigger command
	 */
	char command;		// current command

	int if_flag;		// 4 modes of command execution
	int arg1;		//
	int arg2;		// Arguments to the command
	int arg3;		//
	int arg4;
	int line;		// line number this command appears on
	char *sarg1;		// string argument
	char *sarg2;		// string argument
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

struct QuestBodrichRewards
{
	int level;
	int vnum;
	int money;
	int exp;
};

class QuestBodrich
{
public:
	QuestBodrich();

private:
	void load_mobs();
	void load_objs();
	void load_rewards();

	// здесь храним предметы для каждого класса
	std::map<int, std::vector<int>> objs;
	// здесь храним мобов
	std::map<int, std::vector<int>> mobs;
	// а здесь награды
	std::map<int, std::vector<QuestBodrichRewards>> rewards;
};

struct City
{
	std::string name; // имя города
	std::vector<int> vnums; // номера зон, которые принадлежат городу
	int rent_vnum; // внум ренты города
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

class player_index_element
{
public:
	player_index_element(const int id, const char* name);

	//added by WorM индексируюца еще мыло и последний айпи
	char *mail;
	char *last_ip;
	//end by WorM
	int unique;
	int level;
	int remorts;
	int last_logon;
	int activity;		// When player be saved and checked
	save_info *timer;

	const char* name() const { return m_name; }
	int id() const { return m_id; }

	void set_name(const char* name);
	void set_id(const int id) { m_id = id; }

private:
	int m_id;
	const char *m_name;
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
	std::array<int, MAX_MOB_LEVEL + 1> prob; // вероятность загрузки для каждого уровня моба
};

class MobRace
{
public:
	MobRace();
	~MobRace();
	std::string race_name;
	std::vector<ingredient> ingrlist;
};

typedef std::shared_ptr<MobRace> MobRacePtr;
typedef std::map<int, MobRacePtr> MobRaceListType;

//-Polud

extern room_rnum top_of_world;

void add_trig_index_entry(int nr, TRIG_DATA* proto);
extern INDEX_DATA **trig_index;

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

class Rooms: public std::vector<ROOM_DATA *>
{
public:
	static constexpr int UNDEFINED_ROOM_VNUM = -1;
	~Rooms();
};

extern Rooms& world;

extern INDEX_DATA *mob_index;
extern mob_rnum top_of_mobt;

inline obj_vnum GET_OBJ_VNUM(const CObjectPrototype* obj) { return obj->get_vnum(); }

extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *mob_proto;
extern const char *MENU;

extern struct portals_list_type *portals_list;
extern TIME_INFO_DATA time_info;

extern int convert_drinkcon_skill(CObjectPrototype *obj, bool proto);

OBJ_DATA *create_material(CHAR_DATA *mob);
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

class PlayersIndex : public std::vector<player_index_element>
{
public:
	using parent_t = std::vector<player_index_element>;
	using parent_t::operator[];
	using parent_t::size;
	using free_names_list_t = std::list<std::string>;

	static const std::size_t NOT_FOUND;

	~PlayersIndex();

	std::size_t append(const player_index_element& element);
	bool player_exists(const int id) const { return m_id_to_index.find(id) != m_id_to_index.end(); }
	bool player_exists(const char* name) const { return NOT_FOUND != get_by_name(name); }
	std::size_t get_by_name(const char* name) const;
	void set_name(const std::size_t index, const char* name);

	void add_free(const std::string& name) { m_free_names.push_back(name); }
	auto free_names_count() const { return m_free_names.size(); }
	void get_free_names(const int count, free_names_list_t& names) const;

private:
	class hasher
	{
	public:
		std::size_t operator()(const std::string& value) const;
	};

	class equal_to
	{
	public:
		bool operator()(const std::string& left, const std::string& right) const;
	};

	using id_to_index_t = std::unordered_map<int, std::size_t>;
	using name_to_index_t = std::unordered_map<std::string, std::size_t, hasher, equal_to>;
	using free_names_t = std::deque<std::string>;

	void add_name_to_index(const char* name, const std::size_t index);

	id_to_index_t m_id_to_index;
	name_to_index_t m_name_to_index;
	free_names_t m_free_names;
};

extern PlayersIndex& player_table;

extern long top_idnum;

bool player_exists(const long id);

inline save_info* SAVEINFO(const size_t number)
{
	return player_table[number].timer;
}

inline void clear_saveinfo(const size_t number)
{
	delete player_table[number].timer;
	player_table[number].timer = NULL;
}

void recreate_saveinfo(const size_t number);

void set_god_skills(CHAR_DATA *ch);
void check_room_flags(int rnum);

namespace OfftopSystem
{
	void init();
	void set_flag(CHAR_DATA *ch);
} // namespace OfftopSystem

void delete_char(const char *name);

void set_test_data(CHAR_DATA *mob);

void set_zone_mob_level();

bool can_snoop(CHAR_DATA *imm, CHAR_DATA *vict);

extern insert_wanted_gem iwg;

class GameLoader
{
public:
	GameLoader();

	void boot_world();
	void index_boot(const EBootType mode);

private:
	static void prepare_global_structures(const EBootType mode, const int rec_count);
};

extern GameLoader world_loader;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
