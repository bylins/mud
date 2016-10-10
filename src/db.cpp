/* ************************************************************************
*   File: db.cpp                                        Part of Bylins    *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
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

#define __DB_C__

#include "db.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "pk.h"
#include "olc.h"
#include "diskio.h"
#include "im.h"
#include "top.h"
#include "craft.hpp"
#include "stuff.hpp"
#include "ban.hpp"
#include "item.creation.hpp"
#include "features.hpp"
#include "description.h"
#include "deathtrap.hpp"
#include "title.hpp"
#include "privilege.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "genchar.h"
#include "random.hpp"
#include "file_crc.hpp"
#include "char.hpp"
#include "skills.h"
#include "char_player.hpp"
#include "parcel.hpp"
#include "liquid.hpp"
#include "corpse.hpp"
#include "name_list.hpp"
#include "modify.h"
#include "room.hpp"
#include "glory_const.hpp"
#include "glory_misc.hpp"
#include "shop_ext.hpp"
#include "named_stuff.hpp"
#include "celebrates.hpp"
#include "player_races.hpp"
#include "morph.hpp"
#include "birth_places.hpp"
#include "pugixml.hpp"
#include "sets_drop.hpp"
#include "fight.h"
#include "help.hpp"
#include "ext_money.hpp"
#include "noob.hpp"
#include "parse.hpp"
#include "reset_stats.hpp"
#include "mob_stat.hpp"
#include "obj.hpp"
#include "obj_sets.hpp"
#include "bonus.h"
#include "time_utils.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/dynamic_bitset.hpp>

#include <sys/stat.h>

#include <sstream>
#include <string>
#include <cmath>

#define  TEST_OBJECT_TIMER   30
#define CRITERION_FILE "criterion.xml"
#define CASES_FILE "cases.xml"
#define RANDOMOBJ_FILE "randomobj.xml"
/**************************************************************************
*  declarations of global containers and objects                          *
**************************************************************************/
BanList *ban = 0;		// contains list of banned ip's and proxies + interface

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
long beginning_of_time = -1561789232;
#else
long beginning_of_time = 650336715;
#endif

CRooms world;

room_rnum top_of_world = 0;	// ref to top element of world

CHAR_DATA *character_list = NULL;	// global linked list of chars

INDEX_DATA **trig_index;	// index table for triggers
int top_of_trigt = 0;		// top of trigger index table
long max_id = MOBOBJ_ID_BASE;	// for unique mob/obj id's

INDEX_DATA *mob_index;		// index table for mobile file
CHAR_DATA *mob_proto;		// prototypes for mobs
mob_rnum top_of_mobt = 0;	// top of mobile index table
void Load_Criterion(pugi::xml_node XMLCriterion, int type);
int global_uid = 0;

OBJ_DATA *object_list = NULL;	// global linked list of objs
CObjectPrototypes obj_proto;

struct zone_data *zone_table;	// zone table
zone_rnum top_of_zone_table = 0;	// top element of zone tab
struct message_list fight_messages[MAX_MESSAGES];	// fighting messages

struct player_index_element *player_table = NULL;	// index to plr file
FILE *player_fl = NULL;		// file desc of player file
int top_of_p_table = 0;		// ref to top of table
int top_of_p_file = 0;		// ref of size of p file
long top_idnum = 0;		// highest idnum in use

time_t boot_time = 0;		// time of mud boot
int circle_restrict = 0;	// level of game restriction
room_rnum r_mortal_start_room;	// rnum of mortal start room
room_rnum r_immort_start_room;	// rnum of immort start room
room_rnum r_frozen_start_room;	// rnum of frozen start room
room_rnum r_helled_start_room;
room_rnum r_named_start_room;
room_rnum r_unreg_start_room;

char *credits = NULL;		// game credits
char *motd = NULL;		// message of the day - mortals
char *rules = NULL;		// rules for immorts
char *GREETINGS = NULL;		// opening credits screen
char *help = NULL;		// help screen
char *info = NULL;		// info page
char *immlist = NULL;		// list of peon gods
char *background = NULL;	// background story
char *handbook = NULL;		// handbook for new immortals
char *policies = NULL;		// policies page
char *name_rules = NULL;		// rules of character's names

TIME_INFO_DATA time_info;	// the infomation about the time
struct weather_data weather_info;	// the infomation about the weather
struct player_special_data dummy_mob;	// dummy spec area for mobs
struct reset_q_type reset_q;	// queue of zones to be reset

const FLAG_DATA clear_flags;

struct portals_list_type *portals_list;	// Список проталов для townportal
int now_entrycount = FALSE;
extern int reboot_uptime;

guardian_type guardian_list;

insert_wanted_gem iwg;

// local functions
void SaveGlobalUID(void);
void LoadGlobalUID(void);
bool check_object_spell_number(OBJ_DATA * obj, unsigned val);
bool check_object_level(OBJ_DATA * obj, int val);
void setup_dir(FILE * fl, int room, unsigned dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode, char *filename);
bool check_object(OBJ_DATA *);
void parse_trigger(FILE * fl, int virtual_nr);
void parse_room(FILE * fl, int virtual_nr, int virt);
char *parse_object(FILE * obj_f, const int nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE * fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void init_spec_procs(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void do_reboot(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void boot_world(void);
int count_alias_records(FILE * fl);
int count_hash_records(FILE * fl);
int count_social_records(FILE * fl, int *messages, int *keywords);
void parse_simple_mob(FILE * mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE * mob_f, int i, int nr);
void get_one_line(FILE * fl, char *buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
int mobs_in_room(int m_num, int r_num);
void new_build_player_index(void);
void renum_obj_zone(void);
void renum_mob_zone(void);
int get_zone_rooms(int, int *, int *);
void init_guilds(void);
void init_basic_values(void);
int init_grouping(void);
void init_portals(void);
void init_im(void);
void init_zone_types();
void load_guardians();

// external functions
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void sort_commands(void);
void Read_Invalid_List(void);
int find_name(const char *name);
int csort(const void *a, const void *b);
void prune_crlf(char *txt);
int Crash_read_timer(int index, int temp);
void Crash_clear_objects(int index);
void extract_mob(CHAR_DATA * ch);
//F@N|
int exchange_database_load(void);

void load_socials(FILE * fl);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void calc_easter(void);
void do_start(CHAR_DATA * ch, int newbie);
int calc_loadroom(CHAR_DATA * ch, int bplace_mode = BIRTH_PLACE_UNDEFINED);
extern void repop_decay(zone_rnum zone);	// рассыпание обьектов ITEM_REPOP_DECAY
int real_zone(int number);
int level_exp(CHAR_DATA * ch, int level);
extern void NewNameRemove(CHAR_DATA * ch);
extern void NewNameLoad();
extern char *fread_action(FILE * fl, int nr);

//polud
void load_mobraces();
//-polud

// external vars
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum helled_start_room;
extern room_vnum named_start_room;
extern room_vnum unreg_start_room;
extern DESCRIPTOR_DATA *descriptor_list;
extern const char *unused_spellname;
extern int top_of_socialm;
extern int top_of_socialk;
extern struct month_temperature_type year_temp[];
extern const char *pc_class_types[];
extern char *house_rank[];
extern struct pclean_criteria_data pclean_criteria[];
extern void LoadProxyList();
extern void add_karma(CHAR_DATA * ch, const char * punish , const char * reason);

/*
extern struct global_drop_obj;
extern std::vector<global_drop_obj> drop_list_obj;*/
#define READ_SIZE 256



int strchrn (const char *s, int c) {
	
 int n=-1;
 while (*s) {
  n++;
  if (*s==c) return n;
  s++;
 }
 return -1;
}


// поиск подстроки
int strchrs(const char *s, const char *t) { 
 while (*t) {
  int r=strchrn(s,*t);
  if (r>-1) return r;
  t++;
 }
 return -1;
}


struct Item_struct
{
	// для параметров
	std::map<std::string, double> params;
	// для аффектов
	std::map<std::string, double> affects;
};

// массив для критерии, каждый элемент массива это отдельный слот
Item_struct items_struct[17]; // = new Item_struct[16];

// определение степени двойки
int exp_two_implementation(int number)
{
	int count = 0;
	while (true)
	{
		const int tmp = 1 << count;
		if (number < tmp)
		{
			return -1;
		}
		if (number == tmp)
		{
			return count;
		}
		count++;
	}
}

template <typename EnumType>
inline int exp_two(EnumType number)
{
	return exp_two_implementation(to_underlying(number));
}

template <> inline int exp_two(int number) { return exp_two_implementation(number); }

bool check_obj_in_system_zone(int vnum)
{
    // ковка
    if ((vnum < 400) && (vnum > 299))
	return true;
    // сет-шмот
    if ((vnum >= 1200) && (vnum <= 1299))
	return true; 
    if ((vnum >= 2300) && (vnum <= 2399))
	return true;
    // луки
    if ((vnum >= 1500) && (vnum <= 1599))
	return true;
    return false;
}

bool check_unlimited_timer(const CObjectPrototype* obj)
{
	char buf_temp[MAX_STRING_LENGTH];
	char buf_temp1[MAX_STRING_LENGTH];
	//sleep(15);
	// куда одевается наш предмет
	int item_wear = -1;
	bool type_item = false;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WORN
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		type_item = true;
	}
	// сумма для статов
	double sum = 0;
	// сумма для аффектов
	double sum_aff = 0;
	// по другому чот не получилось
	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_FINGER))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_FINGER);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_NECK))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_NECK);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_BODY))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_BODY);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_HEAD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_HEAD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_LEGS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_LEGS);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_FEET))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_FEET);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_HANDS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_HANDS);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_ARMS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_ARMS);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_SHIELD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_SHIELD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_ABOUT))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_ABOUT);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_WAIST))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_WAIST);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_WRIST))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_WRIST);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_WIELD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_WIELD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_HOLD))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_HOLD);
	}

	if (obj->has_wear_flag(EWearFlag::ITEM_WEAR_BOTHS))
	{
		item_wear = exp_two(EWearFlag::ITEM_WEAR_BOTHS);
	}

	if (!type_item)
	{
		return false;
	}
	// находится ли объект в системной зоне
	if (check_obj_in_system_zone(obj->get_vnum()))
	{
		return false;
	}
	// если объект никуда не надевается, то все, облом
	if (item_wear == -1)
	{
		return false;
	}
	// если шмотка магическая или энчантнута таймер обычный
    if (obj->get_extra_flag(EExtraFlag::ITEM_MAGIC))
	{
		return false;
	}
	// если это сетовый предмет
	if (obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF))
	{
		return false;
	}
	// рассыпется вне зоны
	if (obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY))
	{
		return false;
	}

	// если предмет требует реморты, то он явно овер
	if (obj->get_manual_mort_req() > 0)
		return false;
	// проверяем дырки в предмете
	 if (obj->get_extra_flag(EExtraFlag::ITEM_WITH1SLOT)
		 || obj->get_extra_flag(EExtraFlag::ITEM_WITH2SLOTS)
		 || obj->get_extra_flag(EExtraFlag::ITEM_WITH3SLOTS))
	 {
		 return false;
	 }
	// если у объекта таймер ноль, то облом. 
	if (obj->get_timer() == 0)
	{
		return false;
	}
	// проходим по всем характеристикам предмета
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->get_affected(i).modifier)
		{
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// проходим по нашей таблице с критериями
			for (std::map<std::string, double>::iterator it = items_struct[item_wear].params.begin(); it != items_struct[item_wear].params.end(); it++)
			{
				if (strcmp(it->first.c_str(), buf_temp) == 0)
				{
					if (obj->get_affected(i).modifier > 0)
					{
						sum += it->second * obj->get_affected(i).modifier;
					}
				}
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");
	
	// проходим по всем аффектам в нашей таблице
	for(std::map<std::string, double>::iterator it = items_struct[item_wear].affects.begin(); it != items_struct[item_wear].affects.end(); it++) 
	{
		// проверяем, есть ли наш аффект на предмете
		if (strstr(buf_temp1, it->first.c_str()) != NULL)
		{
			sum_aff += it->second;
		}
		//std::cout << it->first << " " << it->second << std::endl;
	}

	// если сумма больше или равна единице
	if (sum >= 1)
	{
		return false;
	}

	// тоже самое для аффектов на объекте
	if (sum_aff >= 1)
	{
		return false;
	}

	// иначе все норм
	return true;
}

float count_koef_obj(const CObjectPrototype *obj,int item_wear)
{
	double sum = 0.0;
	double sum_aff = 0.0;
	char buf_temp[MAX_STRING_LENGTH];
	char buf_temp1[MAX_STRING_LENGTH];

	// проходим по всем характеристикам предмета
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->get_affected(i).modifier)
		{
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// проходим по нашей таблице с критериями
			for (std::map<std::string, double>::iterator it = items_struct[item_wear].params.begin(); it != items_struct[item_wear].params.end(); it++)
			{

				if (strcmp(it->first.c_str(), buf_temp) == 0)
				{
					if (obj->get_affected(i).modifier > 0)
					{
						sum += it->second * obj->get_affected(i).modifier;
					}
				}

				//std::cout << it->first << " " << it->second << std::endl;
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");
	
	// проходим по всем аффектам в нашей таблице
	for(std::map<std::string, double>::iterator it = items_struct[item_wear].affects.begin(); it != items_struct[item_wear].affects.end(); it++) 
	{
		// проверяем, есть ли наш аффект на предмете
		if (strstr(buf_temp1, it->first.c_str()) != NULL)
		{
			sum_aff += it->second;
		}
		//std::cout << it->first << " " << it->second << std::endl;
	}
	sum += sum_aff;
	return sum;
}

float count_unlimited_timer(const CObjectPrototype *obj)
{
	float result = 0.0;
	bool type_item = false;
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WORN
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		type_item = true;
	}
	// сумма для статов
	
	result = 0.0;
	
	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_FINGER));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_NECK));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_BODY));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_HEAD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_LEGS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_FEET));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_HANDS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_ARMS));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_SHIELD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_ABOUT));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_WAIST));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_WRIST));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_WIELD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_HOLD));
	}

	if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
	{
		result += count_koef_obj(obj, exp_two(EWearFlag::ITEM_WEAR_BOTHS));
	}

	if (!type_item)
	{
		return 0.0;
	}

	return result;
}

float count_remort_requred(const CObjectPrototype *obj)
{
	float result = 0.0;
	const float SQRT_MOD = 1.7095f;
	const int AFF_SHIELD_MOD = 30;
	const int AFF_BLINK_MOD = 10;

	result = 0.0;
	
	if (ObjSystem::is_mob_item(obj)
		|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))
	{
		return result;
	}

	float total_weight = 0.0;

	// аффекты APPLY_x
	for (int k = 0; k < MAX_OBJ_AFFECT; k++)
	{
		if (obj->get_affected(k).location == 0) continue;

		// случай, если один аффект прописан в нескольких полях
		for (int kk = 0; kk < MAX_OBJ_AFFECT; kk++)
		{
			if (obj->get_affected(k).location == obj->get_affected(kk).location
				&& k != kk)
			{
				log("SYSERROR: double affect=%d, obj_vnum=%d",
					obj->get_affected(k).location, GET_OBJ_VNUM(obj));
				return 1000000;
			}
		}
		if (obj->get_affected(k).modifier < 0)
		{
			continue;
		}
		float weight = ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
		total_weight += pow(weight, SQRT_MOD);
	}
	// аффекты AFF_x через weapon_affect
	for (const auto& m : weapon_affect)
	{
		if (IS_OBJ_AFF(obj, m.aff_pos))
		{
			if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_AIRSHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_FIRESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_ICESHIELD)
			{
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			}
			else if (static_cast<EAffectFlag>(m.aff_bitvector) == EAffectFlag::AFF_BLINK)
			{
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			}
		}
	}

	result = ceil(pow(total_weight, 1/SQRT_MOD));

	return result;
}

void Load_Criterion(pugi::xml_node XMLCriterion, const EWearFlag type)
{
	int index = exp_two(type);
	pugi::xml_node params, CurNode, affects;
	params = XMLCriterion.child("params");
	affects = XMLCriterion.child("affects");
	
	// добавляем в массив все параметры, типа силы, ловкости, каст и тд
	for (CurNode = params.child("param"); CurNode; CurNode = CurNode.next_sibling("param"))
	{
		items_struct[index].params.insert(std::make_pair(CurNode.attribute("name").value(), CurNode.attribute("value").as_double()));
		//log("str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}
	
	// добавляем в массив все аффекты
	for (CurNode = affects.child("affect"); CurNode; CurNode = CurNode.next_sibling("affect"))
	{
		items_struct[index].affects.insert(std::make_pair(CurNode.attribute("name").value(), CurNode.attribute("value").as_double()));
		//log("Affects:str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}	
}

// Separate a 4-character id tag from the data it precedes
void tag_argument(char *argument, char *tag)
{
	char *tmp = argument, *ttag = tag, *wrt = argument;
	int i;

	for (i = 0; i < 4; i++)
		*(ttag++) = *(tmp++);
	*ttag = '\0';

	while (*tmp == ':' || *tmp == ' ')
		tmp++;

	while (*tmp)
		*(wrt++) = *(tmp++);
	*wrt = '\0';
}

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

void go_boot_socials(void)
{
	int i;

	if (soc_mess_list)
	{
		for (i = 0; i <= top_of_socialm; i++)
		{
			if (soc_mess_list[i].char_no_arg)
				free(soc_mess_list[i].char_no_arg);
			if (soc_mess_list[i].others_no_arg)
				free(soc_mess_list[i].others_no_arg);
			if (soc_mess_list[i].char_found)
				free(soc_mess_list[i].char_found);
			if (soc_mess_list[i].others_found)
				free(soc_mess_list[i].others_found);
			if (soc_mess_list[i].vict_found)
				free(soc_mess_list[i].vict_found);
			if (soc_mess_list[i].not_found)
				free(soc_mess_list[i].not_found);
		}
		free(soc_mess_list);
	}
	if (soc_keys_list)
	{
		for (i = 0; i <= top_of_socialk; i++)
			if (soc_keys_list[i].keyword)
				free(soc_keys_list[i].keyword);
		free(soc_keys_list);
	}
	top_of_socialm = -1;
	top_of_socialk = -1;
	index_boot(DB_BOOT_SOCIAL);


}

void load_sheduled_reboot()
{
	FILE *sch;
	int day = 0, hour = 0, minutes = 0, numofreaded = 0, timeOffset = 0;
	char str[10];
	int offsets[7];

	time_t currTime;

	sch = fopen(LIB_MISC "schedule", "r");
	if (!sch)
	{
		log("Error opening schedule");
		return;
	}
	time(&currTime);
	for (int i = 0; i < 7; i++)
	{
		if (fgets(str, 10, sch) == NULL)
		{
			break;
		}
		numofreaded = sscanf(str, "%i %i:%i", &day, &hour, &minutes);
		if (numofreaded < 1)
		{
			continue;
		}
		if (day != i)
		{
			break;
		}
		if (numofreaded == 1)
		{
			offsets[i] = 24 * 60;
			continue;
		}
		offsets[i] = hour * 60 + minutes;
	}
	fclose(sch);

	timeOffset = (int) difftime(currTime, boot_time);

	//set reboot_uptime equal to current uptime and align to the start of the day
	reboot_uptime = timeOffset / 60 - localtime(&currTime)->tm_min - 60 * (localtime(&currTime)->tm_hour);
	for (int i = localtime(&boot_time)->tm_wday; i < 7; i++)
	{
		//7 empty days was cycled - break with default uptime
		if (reboot_uptime - 1440 * 7 >= 0)
		{
			reboot_uptime = DEFAULT_REBOOT_UPTIME;	//2 days reboot bu default if no schedule
			break;
		}
		//if we get non-1-full-day offset, but server will reboot to early (36 hour is minimum)
		//we are going to find another scheduled day
		if (offsets[i] < 24 * 60 && (reboot_uptime + offsets[i]) < UPTIME_THRESHOLD * 60)
		{
			reboot_uptime += 24 * 60;
		}
		//we've found next point of reboot! :) break cycle
		if (offsets[i] < 24 * 60 && (reboot_uptime + offsets[i]) > UPTIME_THRESHOLD * 60)
		{
			reboot_uptime += offsets[i];
			break;
		}
		//empty day - add 24 hour and go next
		if (offsets[i] == 24 * 60)
		{
			reboot_uptime += offsets[i];
		}
		// jump to 1st day of the week
		if (i == 6)
			i = -1;
	}
	log("Setting up reboot_uptime: %i", reboot_uptime);
}

// Базовая функция загрузки XML конфигов
pugi::xml_node XMLLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document& Doc)
{
	std::ostringstream buffer;
	pugi::xml_parse_result Result;
	pugi::xml_node NodeList;

    // Пробуем прочитать файл
	Result = Doc.load_file(PathToFile);
	// Oops, файла нет
	if (!Result)
	{
		buffer << "..." << Result.description();
		mudlog(string(buffer.str()).c_str(), CMP, LVL_IMMORT, SYSLOG, TRUE);
		return NodeList;
	}

    // Ищем в файле указанный тэг
	NodeList = Doc.child(MainTag);
	// Тэга нет - кляузничаем в сислоге
	if (!NodeList)
	{
		mudlog(ErrorStr, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}

	return NodeList;
};

std::vector<_case> cases;
// Заггрузка сундуков в мире
void load_cases()
{
	pugi::xml_document doc_cases;	
	pugi::xml_node case_, object_, file_case;
	file_case = XMLLoad(LIB_MISC CASES_FILE, "cases", "Error loading cases file: cases.xml", doc_cases);
	for (case_ = file_case.child("casef"); case_; case_ = case_.next_sibling("casef"))
	{
		_case __case;
		__case.vnum = case_.attribute("vnum").as_int();
		__case.chance = case_.attribute("chance").as_int();
		for (object_ = case_.child("object"); object_;  object_ = object_.next_sibling("object"))
		{
			__case.vnum_objs.push_back(object_.attribute("vnum").as_int());
		}
		cases.push_back(__case);
	}
}

std::vector<RandomObj> random_objs;

// загрузка параметров для рандомных шмоток
void load_random_obj()
{
	pugi::xml_document doc_robj;
	pugi::xml_node robj, object, file_robj;
	int tmp_buf2;
	std::string tmp_str;
	file_robj = XMLLoad(LIB_MISC RANDOMOBJ_FILE, "object", "Error loading cases file: random_obj.xml", doc_robj);
	for (robj = file_robj.child("obj"); robj; robj = robj.next_sibling("obj"))
	{
		RandomObj tmp_robj;
		tmp_robj.vnum = robj.attribute("vnum").as_int();
		for (object = robj.child("not_to_wear");object;object = object.next_sibling("not_to_wear"))
		{
			tmp_str = object.attribute("value").as_string();
			tmp_buf2 = object.attribute("chance").as_int();
			tmp_robj.not_wear.insert(std::pair<std::string, int>(tmp_str, tmp_buf2));
		}
		tmp_robj.min_weight = robj.child("weight").attribute("value_min").as_int();
		tmp_robj.max_weight = robj.child("weight").attribute("value_max").as_int();
		tmp_robj.min_price = robj.child("price").attribute("value_min").as_int();
		tmp_robj.max_price = robj.child("price").attribute("value_max").as_int();
		tmp_robj.min_stability = robj.child("stability").attribute("value_min").as_int();
		tmp_robj.max_stability = robj.child("stability").attribute("value_max").as_int();
		tmp_robj.value0_min = robj.child("value_zero").attribute("value_min").as_int();
		tmp_robj.value0_max = robj.child("value_zero").attribute("value_max").as_int();
		tmp_robj.value1_min = robj.child("value_one").attribute("value_min").as_int();
		tmp_robj.value1_max = robj.child("value_one").attribute("value_max").as_int();
		tmp_robj.value2_min = robj.child("value_two").attribute("value_min").as_int();
		tmp_robj.value2_max = robj.child("value_two").attribute("value_max").as_int();
		tmp_robj.value3_min = robj.child("value_three").attribute("value_min").as_int();
		tmp_robj.value3_max = robj.child("value_three").attribute("value_max").as_int();
		for (object = robj.child("affect");object;object = object.next_sibling("affect"))
		{
			tmp_str = object.attribute("value").as_string();
			tmp_buf2 = object.attribute("chance").as_int();
			tmp_robj.affects.insert(std::pair<std::string, int>(tmp_str, tmp_buf2));
		}
		for (object = robj.child("extraaffect");object;object = object.next_sibling("extraaffect"))
		{
			ExtraAffects extraAffect;
			extraAffect.number = object.attribute("value").as_int();
			extraAffect.chance = object.attribute("chance").as_int();
			extraAffect.min_val = object.attribute("min_val").as_int();
			extraAffect.max_val = object.attribute("max_val").as_int();
			tmp_robj.extraffect.push_back(extraAffect);
		}
		random_objs.push_back(tmp_robj);
	}
}



/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
void do_reboot(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	argument = one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*')
	{
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
		file_to_string_alloc(IMMLIST_FILE, &immlist);
		file_to_string_alloc(CREDITS_FILE, &credits);
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(RULES_FILE, &rules);
		file_to_string_alloc(HELP_PAGE_FILE, &help);
		file_to_string_alloc(INFO_FILE, &info);
		file_to_string_alloc(POLICIES_FILE, &policies);
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
		file_to_string_alloc(BACKGROUND_FILE, &background);
		file_to_string_alloc(NAME_RULES_FILE, &name_rules);
		go_boot_socials();
		init_im();
		init_zone_types();
		init_portals();
		load_sheduled_reboot();
		oload_table.init();
		OBJ_DATA::init_set_table();
		load_mobraces();
		load_morphs();
		GlobalDrop::init();
		OfftopSystem::init();
		//Celebrates::load(XMLLoad(LIB_MISC CELEBRATES_FILE, CELEBRATES_MAIN_TAG, CELEBRATES_ERROR_STR));
		Celebrates::load();
		HelpSystem::reload_all();
		Remort::init();
		Noob::init();
		ResetStats::init();
		Bonus::bonus_log_load();
	}
	else if (!str_cmp(arg, "portals"))
		init_portals();
	else if (!str_cmp(arg, "imagic"))
		init_im();
	else if (!str_cmp(arg, "ztypes"))
		init_zone_types();
	else if (!str_cmp(arg, "oloadtable"))
		oload_table.init();
	else if (!str_cmp(arg, "setstuff"))
	{
		OBJ_DATA::init_set_table();
		HelpSystem::reload(HelpSystem::STATIC);
	}
	else if (!str_cmp(arg, "immlist"))
		file_to_string_alloc(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "credits"))
		file_to_string_alloc(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		file_to_string_alloc(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "rules"))
		file_to_string_alloc(RULES_FILE, &rules);
	else if (!str_cmp(arg, "help"))
		file_to_string_alloc(HELP_PAGE_FILE, &help);
	else if (!str_cmp(arg, "info"))
		file_to_string_alloc(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		file_to_string_alloc(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		file_to_string_alloc(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "namerules"))
		file_to_string_alloc(NAME_RULES_FILE, &name_rules);
	else if (!str_cmp(arg, "greetings"))
	{
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
	}
	else if (!str_cmp(arg, "xhelp"))
	{
		HelpSystem::reload_all();
	}
	else if (!str_cmp(arg, "socials"))
		go_boot_socials();
	else if (!str_cmp(arg, "schedule"))
		load_sheduled_reboot();
	else if (!str_cmp(arg, "clan"))
		Clan::ClanLoad();
	else if (!str_cmp(arg, "proxy"))
		LoadProxyList();
	else if (!str_cmp(arg, "boards"))
		Board::reload_all();
	else if (!str_cmp(arg, "titles"))
		TitleSystem::load_title_list();
	else if (!str_cmp(arg, "emails"))
		RegisterSystem::load();
	else if (!str_cmp(arg, "privilege"))
		Privilege::load();
	else if (!str_cmp(arg, "mobraces"))
		load_mobraces();
	else if (!str_cmp(arg, "morphs"))
		load_morphs();
	else if (!str_cmp(arg, "depot") && PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		skip_spaces(&argument);
		if (*argument)
		{
			long uid = GetUniqueByName(std::string(argument));
			if (uid > 0)
			{
				Depot::reload_char(uid, ch);
			}
			else
			{
				send_to_char("Указанный чар не найден\r\n"
					"Формат команды: reload depot <имя чара>.\r\n", ch);
			}
		}
		else
		{
			send_to_char("Формат команды: reload depot <имя чара>.\r\n", ch);
		}
	}
	else if (!str_cmp(arg, "globaldrop"))
	{
		GlobalDrop::init();
	}
	else if (!str_cmp(arg, "offtop"))
	{
		OfftopSystem::init();
	}
	else if (!str_cmp(arg, "shop"))
	{
		ShopExt::load(true);
	}
	else if (!str_cmp(arg, "named"))
	{
		NamedStuff::load();
	}
	else if (!str_cmp(arg, "celebrates"))
	{
		//Celebrates::load(XMLLoad(LIB_MISC CELEBRATES_FILE, CELEBRATES_MAIN_TAG, CELEBRATES_ERROR_STR));
		Celebrates::load();
	}
	else if (!str_cmp(arg, "setsdrop") && PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		skip_spaces(&argument);
		if (*argument && is_number(argument))
		{
			SetsDrop::reload(atoi(argument));
		}
		else
		{
			SetsDrop::reload();
		}
	}
	else if (!str_cmp(arg, "remort.xml"))
	{
		Remort::init();
	}
	else if (!str_cmp(arg, "noob_help.xml"))
	{
		Noob::init();
	}
	else if (!str_cmp(arg, "reset_stats.xml"))
	{
		ResetStats::init();
	}
	else if (!str_cmp(arg, "obj_sets.xml"))
	{
		obj_sets::load();
	}
	else
	{
		send_to_char("Неверный параметр для перезагрузки файлов.\r\n", ch);
		return;
	}

	std::string str = boost::str(boost::format("%s reload %s.")
		% ch->get_name() % arg);
	mudlog(str.c_str(), NRM, LVL_IMMORT, SYSLOG, TRUE);

	send_to_char(OK, ch);
}

void init_portals(void)
{
	FILE *portal_f;
	char nm[300], nm2[300], *wrd;
	int rnm = 0, i, level = 0;
	struct portals_list_type *curr, *prev;

	// Сначала освобождаем все порталы
	for (curr = portals_list; curr; curr = prev)
	{
		prev = curr->next_portal;
		free(curr->wrd);
		free(curr);
	}

	portals_list = NULL;
	prev = NULL;

	// читаем файл
	if (!(portal_f = fopen(LIB_MISC "portals.lst", "r")))
	{
		log("Can not open portals.lst");
		return;
	}

	while (get_line(portal_f, nm))
	{
		if (!nm[0] || nm[0] == ';')
			continue;
		sscanf(nm, "%d %d %s", &rnm, &level, nm2);
		if (real_room(rnm) == NOWHERE || nm2[0] == '\0')
		{
			log("Invalid portal entry detected");
			continue;
		}
		wrd = nm2;
		for (i = 0; !(i == 10 || wrd[i] == ' ' || wrd[i] == '\0'); i++);
		wrd[i] = '\0';
		// добавляем портал в список - rnm - комната, wrd - слово
		CREATE(curr, 1);
		CREATE(curr->wrd, strlen(wrd) + 1);
		curr->vnum = rnm;
		curr->level = level;
		for (i = 0, curr->wrd[i] = '\0'; wrd[i]; i++)
			curr->wrd[i] = LOWER(wrd[i]);
		curr->wrd[i] = '\0';
		curr->next_portal = NULL;
		if (!portals_list)
			portals_list = curr;
		else
			prev->next_portal = curr;

		prev = curr;

	}
	fclose(portal_f);
}

/// конверт поля GET_OBJ_SKILL в емкостях TODO: 12.2013
int convert_drinkcon_skill(CObjectPrototype *obj, bool proto)
{
	if (GET_OBJ_SKILL(obj) > 0
		&& (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON
			|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_FOUNTAIN))
	{
		log("obj_skill: %d - %s (%d)", GET_OBJ_SKILL(obj),
			GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_VNUM(obj));
		// если емскости уже просетили какие-то заклы, то зелье
		// из обж-скилл их не перекрывает, а просто удаляется
		if (obj->get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) < 0)
		{
			OBJ_DATA *potion = read_object(GET_OBJ_SKILL(obj), VIRTUAL);
			if (potion
				&& GET_OBJ_TYPE(potion) == OBJ_DATA::ITEM_POTION)
			{
				drinkcon::copy_potion_values(potion, obj);
				if (proto)
				{
					// copy_potion_values сетит до кучи и внум из пошена,
					// поэтому уточним здесь, что зелье не перелито
					// емкости из read_one_object_new идут как перелитые
					obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, 0);
				}
			}
		}
		obj->set_skill(SKILL_INVALID);

		return 1;
	}
	return 0;
}

/// конверт параметров прототипов ПОСЛЕ лоада всех файлов с прототипами
void convert_obj_values()
{
	int save = 0;
	for (const auto i : obj_proto)
	{
		save = std::max(save, convert_drinkcon_skill(i.get(), true));
		if (i->get_extra_flag(EExtraFlag::ITEM_1INLAID))
		{
			i->unset_extraflag(EExtraFlag::ITEM_1INLAID);
			save = 1;
		}
		if (i->get_extra_flag(EExtraFlag::ITEM_2INLAID))
		{
			i->unset_extraflag(EExtraFlag::ITEM_2INLAID);
			save = 1;
		}
		if (i->get_extra_flag(EExtraFlag::ITEM_3INLAID))
		{
			i->unset_extraflag(EExtraFlag::ITEM_3INLAID);
			save = 1;
		}
		// ...
		if (save)
		{
			olc_add_to_save_list(i->get_vnum()/100, OLC_SAVE_OBJ);
			save = 0;
		}
	}
}

void boot_world(void)
{
	utils::CSteppedProfiler boot_profiler("World booting");

	boot_profiler.next_step("Loading zone table");
	log("Loading zone table.");
	index_boot(DB_BOOT_ZON);

	boot_profiler.next_step("Loading triggers");
	log("Loading triggers and generating index.");
	index_boot(DB_BOOT_TRG);

	boot_profiler.next_step("Loading rooms");
	log("Loading rooms.");
	index_boot(DB_BOOT_WLD);

	boot_profiler.next_step("Renumbering rooms");
	log("Renumbering rooms.");
	renum_world();

	boot_profiler.next_step("Checking start rooms");
	log("Checking start rooms.");
	check_start_rooms();

	boot_profiler.next_step("Loading mobs and regerating index");
	log("Loading mobs and generating index.");
	index_boot(DB_BOOT_MOB);

	boot_profiler.next_step("Counting mob's levels");
	log("Count mob quantity by level");
	MobMax::init();

	boot_profiler.next_step("Loading objects");
	log("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);

	boot_profiler.next_step("Converting deprecated obj values");
	log("Converting deprecated obj values.");
	convert_obj_values();

	boot_profiler.next_step("enumbering zone table");
	log("Renumbering zone table.");
	renum_zone_table();

	boot_profiler.next_step("Renumbering Obj_zone");
	log("Renumbering Obj_zone.");
	renum_obj_zone();

	boot_profiler.next_step("Renumbering Mob_zone");
	log("Renumbering Mob_zone.");
	renum_mob_zone();

	boot_profiler.next_step("Initialization of object rnums");
	log("Init system_obj rnums.");
	system_obj::init();
	log("Init global_drop_obj.");
}

//MZ.load
void init_zone_types()
{
	FILE *zt_file;
	char tmp[1024], dummy[128], name[128], itype_num[128];
	int names = 0;
	int i, j, k, n;

	if (zone_types != NULL)
	{
		for (i = 0; *zone_types[i].name != '\n'; i++)
		{
			if (zone_types[i].ingr_qty > 0)
				free(zone_types[i].ingr_types);

			free(zone_types[i].name);
		}
		free(zone_types[i].name);
		free(zone_types);
		zone_types = NULL;
	}

	zt_file = fopen(LIB_MISC "ztypes.lst", "r");
	if (!zt_file)
	{
		log("Can not open ztypes.lst");
		return;
	}

	while (get_line(zt_file, tmp))
	{
		if (!strn_cmp(tmp, "ИМЯ", 3))
		{
			if (sscanf(tmp, "%s %s", dummy, name) != 2)
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!get_line(zt_file, tmp))
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!strn_cmp(tmp, "ТИПЫ", 4))
			{
				if (tmp[4] != ' ' && tmp[4] != '\0')
				{
					log("Corrupted file : ztypes.lst");
					return;
				}
				for (i = 4; tmp[i] != '\0'; i++)
				{
					if (!a_isdigit(tmp[i]) && !a_isspace(tmp[i]))
					{
						log("Corrupted file : ztypes.lst");
						return;
					}
				}
			}
			else
			{
				log("Corrupted file : ztypes.lst");
				return;
			}
			names++;
		}
		else
		{
			log("Corrupted file : ztypes.lst");
			return;
		}
	}
	names++;

	CREATE(zone_types, names);
	for (i = 0; i < names; i++)
	{
		zone_types[i].name = NULL;
		zone_types[i].ingr_qty = 0;
		zone_types[i].ingr_types = NULL;
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp))
	{
		sscanf(tmp, "%s %s", dummy, name);
		for (j = 0; name[j] != '\0'; j++)
			if (name[j] == '_')
				name[j] = ' ';
		zone_types[i].name = str_dup(name);
//		if (get_line(zt_file, tmp)); не уверен что тут хотел автор -- Krodo
		get_line(zt_file, tmp);
		for (j = 4; tmp[j] != '\0'; j++)
		{
			if (a_isspace(tmp[j]))
				continue;
			zone_types[i].ingr_qty++;
			for (; tmp[j] != '\0' && a_isdigit(tmp[j]); j++);
			j--;
		}
		i++;
	}
	zone_types[i].name = str_dup("\n");

	for (i = 0; *zone_types[i].name != '\n'; i++)
	{
		if (zone_types[i].ingr_qty > 0)
		{
			CREATE(zone_types[i].ingr_types, zone_types[i].ingr_qty);
		}
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp))
	{
//		if (get_line(zt_file, tmp)); аналогично тому, что выше
		get_line(zt_file, tmp);
		for (j = 4, n = 0; tmp[j] != '\0'; j++)
		{
			if (a_isspace(tmp[j]))
				continue;
			for (k = 0; tmp[j] != '\0' && a_isdigit(tmp[j]); j++)
				itype_num[k++] = tmp[j];
			itype_num[k] = '\0';
			zone_types[i].ingr_types[n] = atoi(itype_num);
			n++;
			j--;
		}
		i++;
	}

	fclose(zt_file);
}
//-MZ.load

void OBJ_DATA::init_set_table()
{
	std::string cppstr, tag;
	int mode = SETSTUFF_SNUM;
	id_to_set_info_map::iterator snum;
	set_info::iterator vnum;
	qty_to_camap_map::iterator oqty;
	class_to_act_map::iterator clss;
	int appnum = 0;

	OBJ_DATA::set_table.clear();

	std::ifstream fp(LIB_MISC "setstuff.lst");

	if (!fp)
	{
		cppstr = "init_set_table:: Unable open input file 'lib/misc/setstuff.lst'";
		mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	while (!fp.eof())
	{
		std::istringstream isstream;

		getline(fp, cppstr);

		std::string::size_type i = cppstr.find(';');
		if (i != std::string::npos)
			cppstr.erase(i);
		reverse(cppstr.begin(), cppstr.end());
		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof())
		{
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		reverse(cppstr.begin(), cppstr.end());
		isstream.clear();

		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof())
		{
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		isstream.clear();

		if (cppstr.empty())
			continue;

		if (cppstr[0] == '#')
		{
			if (mode != SETSTUFF_SNUM && mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
			{
				cppstr = "init_set_table:: Wrong position of line '" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			cppstr.erase(cppstr.begin());

			if (cppstr.empty() || !a_isdigit(cppstr[0]))
			{
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', expected set id after #";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			int tmpsnum;

			isstream.str(cppstr);
			isstream >> std::noskipws >> tmpsnum;

			if (!isstream.eof())
			{
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', expected only set id after #";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			std::pair< id_to_set_info_map::iterator, bool > p = OBJ_DATA::set_table.insert(std::make_pair(tmpsnum, set_info()));

			if (!p.second)
			{
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', this set already exists";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			snum = p.first;
			mode = SETSTUFF_NAME;
			continue;
		}

		if (cppstr.size() < 5 || cppstr[4] != ':')
		{
			cppstr = "init_set_table:: Format error in line '" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		tag = cppstr.substr(0, 4);
		cppstr.erase(0, 5);
		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof())
		{
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		isstream.clear();

		if (cppstr.empty())
		{
			cppstr = "init_set_table:: Empty parameter field in line '" + tag + ":" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			continue;
		}

		switch (tag[0])
		{
		case 'A':
			if (tag == "Amsg")
			{
				if (mode != SETSTUFF_AMSG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_actmsg(cppstr);
				mode = SETSTUFF_DMSG;
			}
			else if (tag == "Affs")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);
				cppstr.erase();
				isstream >> std::skipws >> cppstr;
				if (!isstream.eof())
				{
					std::string suffix;

					getline(isstream, suffix);
					cppstr += suffix;
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object affects";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				FLAG_DATA tmpaffs = clear_flags;
				tmpaffs.from_string(cppstr.c_str());

				clss->second.set_affects(tmpaffs);
				appnum = 0;
				mode = SETSTUFF_AFCN;
			}
			else if (tag == "Afcn")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int tmploc, tmpmodi;

				if (!(isstream >> std::skipws >> tmploc >> std::skipws >> tmpmodi))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected apply location and modifier";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				obj_affected_type tmpafcn(static_cast<EApplyLocation>(tmploc), (sbyte)tmpmodi);

				if (!isstream.eof())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only apply location and modifier";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (tmpafcn.location <= APPLY_NONE || tmpafcn.location >= NUM_APPLIES)
				{
					cppstr = "init_set_table:: Wrong apply location in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (!tmpafcn.modifier)
				{
					cppstr = "init_set_table:: Wrong apply modifier in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (mode == SETSTUFF_AMSG || mode == SETSTUFF_AFFS)
					appnum = 0;

				if (appnum >= MAX_OBJ_AFFECT)
				{
					cppstr = "init_set_table:: Too many applies - line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}
				else
					clss->second.set_affected_i(appnum++, tmpafcn);

				mode = SETSTUFF_AFCN;
			}
			else if (tag == "Alis")
			{
				if (mode != SETSTUFF_ALIS)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				snum->second.set_alias(cppstr);
				mode = SETSTUFF_VNUM;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'C':
			if (tag == "Clss")
			{
				if (mode != SETSTUFF_CLSS && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				unique_bit_flag_data tmpclss;

				if (cppstr == "all")
					tmpclss.set_all();
				else
				{
					isstream.str(cppstr);

					int i = 0;

					while (isstream >> std::skipws >> i)
						if (i < 0 || i > NUM_PLAYER_CLASSES * NUM_KIN)
							break;
						else
							tmpclss.set(flag_data_by_num(i));

					if (i < 0 || i > NUM_PLAYER_CLASSES * NUM_KIN)
					{
						cppstr = "init_set_table:: Wrong class in line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
						continue;
					}

					if (!isstream.eof())
					{
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only class ids";
						mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
						continue;
					}
				}

				std::pair< class_to_act_map::iterator, bool > p = oqty->second.insert(std::make_pair(tmpclss, activation()));

				if (!p.second)
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr +
							 "', each class number can occur only once for each object number";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss = p.first;
				mode = SETSTUFF_AMSG;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'D':
			if (tag == "Dmsg")
			{
				if (mode != SETSTUFF_DMSG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_deactmsg(cppstr);
				mode = SETSTUFF_RAMG;
			}
			else if (tag == "Dice")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int ndices, nsides;

				if (!(isstream >> std::skipws >> ndices >> std::skipws >> nsides))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected ndices and nsides";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_dices(ndices, nsides);
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'N':
			if (tag == "Name")
			{
				if (mode != SETSTUFF_NAME)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				snum->second.set_name(cppstr);
				mode = SETSTUFF_ALIS;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'O':
			if (tag == "Oqty")
			{
				if (mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				unsigned int tmpoqty = 0;
				isstream.str(cppstr);
				isstream >> std::skipws >> tmpoqty;

				if (!isstream.eof())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object number";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (!tmpoqty || tmpoqty > NUM_WEARS)
				{
					cppstr = "init_set_table:: Wrong object number in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				std::pair< qty_to_camap_map::iterator, bool > p = vnum->second.insert(std::make_pair(tmpoqty, class_to_act_map()));

				if (!p.second)
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr +
							 "', each object number can occur only once for each object";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				oqty = p.first;
				mode = SETSTUFF_CLSS;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'R':
			if (tag == "Ramg")
			{
				if (mode != SETSTUFF_RAMG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_room_actmsg(cppstr);
				mode = SETSTUFF_RDMG;
			}
			else if (tag == "Rdmg")
			{
				if (mode != SETSTUFF_RDMG)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_room_deactmsg(cppstr);
				mode = SETSTUFF_AFFS;
			}
			else
			{
				cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'S':
			if (tag == "Skll")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int skillnum, percent;

				if (!(isstream >> std::skipws >> skillnum >> std::skipws >> percent))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected ndices and nsides";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_skill(static_cast<ESkill>(skillnum), percent);
			}
			else
			{
				cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		case 'V':
			if (tag == "Vnum")
			{
				if (mode != SETSTUFF_ALIS
					&& mode != SETSTUFF_VNUM
					&& mode != SETSTUFF_OQTY
					&& mode != SETSTUFF_AMSG
					&& mode != SETSTUFF_AFFS
					&& mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				obj_vnum tmpvnum = -1;
				isstream.str(cppstr);
				isstream >> std::skipws >> tmpvnum;

				if (!isstream.eof())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object vnum";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (real_object(tmpvnum) < 0)
				{
					cppstr = "init_set_table:: Wrong object vnum in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				id_to_set_info_map::iterator it;

				for (it = OBJ_DATA::set_table.begin(); it != OBJ_DATA::set_table.end(); it++)
					if (it->second.find(tmpvnum) != it->second.end())
						break;

				if (it != OBJ_DATA::set_table.end())
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', object can exist only in one set";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				vnum = snum->second.insert(std::make_pair(tmpvnum, qty_to_camap_map())).first;
				mode = SETSTUFF_OQTY;
			}
			else
			{
				cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;
		case 'W':
			if (tag == "Wght")
			{
				if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
				{
					cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				isstream.str(cppstr);

				int weight;

				if (!(isstream >> std::skipws >> weight))
				{
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected item weight";
					mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				clss->second.set_weight(weight);
			}
			else
			{
				cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
			break;

		default:
			cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		}
	}

	if (mode != SETSTUFF_SNUM && mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN)
	{
		cppstr = "init_set_table:: Last set was deleted, because of unexpected end of file";
		OBJ_DATA::set_table.erase(snum);
		mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
	}
}

void set_zone_mob_level()
{
	for (int i = 0; i <= top_of_zone_table; ++i)
	{
		int level = 0, count = 0;
		for (int nr = 0; nr <= top_of_mobt; ++nr)
		{
			if (mob_index[nr].vnum >= zone_table[i].number * 100
				&& mob_index[nr].vnum <= zone_table[i].number * 100 + 99)
			{
				level += mob_proto[nr].get_level();
				++count;
			}
		}
		zone_table[i].mob_level = count ? level/count : 0;
	}
}

void set_zone_town()
{
	for (int i = 0; i <= top_of_zone_table; ++i)
	{
		zone_table[i].is_town = false;
		int rnum_start = 0, rnum_end = 0;
		if (!get_zone_rooms(i, &rnum_start, &rnum_end))
		{
			continue;
		}
		bool rent_flag = false, bank_flag = false, post_flag = false;
		// зона считается городом, если в ней есть рентер, банкир и почтовик
		for (int k = rnum_start; k <= rnum_end; ++k)
		{
			for (CHAR_DATA *ch = world[k]->people; ch; ch = ch->next_in_room)
			{
				if (IS_RENTKEEPER(ch))
				{
					rent_flag = true;
				}
				else if (IS_BANKKEEPER(ch))
				{
					bank_flag = true;
				}
				else if (IS_POSTKEEPER(ch))
				{
					post_flag = true;
				}
			}
		}
		if (rent_flag && bank_flag && post_flag)
		{
			zone_table[i].is_town = true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
namespace
{

struct snoop_node
{
	// мыло имма
	std::string mail;
	// список уидов чаров с таким же мылом
	std::vector<long> uid_list;
};

// номер клан ренты -> список мыл
typedef std::map<int, std::set<std::string> > ClanMailType;
ClanMailType clan_list;

} // namespace

// * Проверка возможности снупа виктима иммом (проверка кланов и их политики).
bool can_snoop(CHAR_DATA *imm, CHAR_DATA *vict)
{
	if (!CLAN(vict))
	{
		return true;
	}

	for (ClanMailType::const_iterator i = clan_list.begin(),
		iend = clan_list.end(); i != iend; ++i)
	{
		std::set<std::string>::const_iterator k = i->second.find(GET_EMAIL(imm));
		if (k != i->second.end() && CLAN(vict)->CheckPolitics(i->first) == POLITICS_WAR)
		{
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void load_messages(void)
{
	FILE *fl;
	int i, type;
	struct message_type *messages;
	char chk[128];

	if (!(fl = fopen(MESS_FILE, "r")))
	{
		log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
		exit(1);
	}
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		fight_messages[i].a_type = 0;
		fight_messages[i].number_of_attacks = 0;
		fight_messages[i].msg = 0;
	}


	const char* dummyc = fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
	{
		dummyc = fgets(chk, 128, fl);
	}

	while (*chk == 'M')
	{
		dummyc = fgets(chk, 128, fl);

		int dummyi = sscanf(chk, " %d\n", &type);
		UNUSED_ARG(dummyi);

		for (i = 0; (i < MAX_MESSAGES) &&
				(fight_messages[i].a_type != type) && (fight_messages[i].a_type); i++);
		if (i >= MAX_MESSAGES)
		{
			log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
			exit(1);
		}
		log("BATTLE MESSAGE %d(%d)", i, type);
		CREATE(messages, 1);
		fight_messages[i].number_of_attacks++;
		fight_messages[i].a_type = type;
		messages->next = fight_messages[i].msg;
		fight_messages[i].msg = messages;

		messages->die_msg.attacker_msg = fread_action(fl, i);
		messages->die_msg.victim_msg = fread_action(fl, i);
		messages->die_msg.room_msg = fread_action(fl, i);
		messages->miss_msg.attacker_msg = fread_action(fl, i);
		messages->miss_msg.victim_msg = fread_action(fl, i);
		messages->miss_msg.room_msg = fread_action(fl, i);
		messages->hit_msg.attacker_msg = fread_action(fl, i);
		messages->hit_msg.victim_msg = fread_action(fl, i);
		messages->hit_msg.room_msg = fread_action(fl, i);
		messages->god_msg.attacker_msg = fread_action(fl, i);
		messages->god_msg.victim_msg = fread_action(fl, i);
		messages->god_msg.room_msg = fread_action(fl, i);
		dummyc = fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		{
			dummyc = fgets(chk, 128, fl);
		}
	}
	UNUSED_ARG(dummyc);

	fclose(fl);
}

// body of the booting system
void boot_db(void)
{
	utils::CSteppedProfiler boot_profiler("MUD booting");

	log("Boot db -- BEGIN.");

	boot_profiler.next_step("Creating directories");
	struct stat st;
	#define MKDIR(dir) if (stat((dir), &st) != 0) \
		mkdir((dir), 0744)
	#define MKLETTERS(BASE) MKDIR(#BASE); \
		MKDIR(#BASE "/A-E"); \
		MKDIR(#BASE "/F-J"); \
		MKDIR(#BASE "/K-O"); \
		MKDIR(#BASE "/P-T"); \
		MKDIR(#BASE "/U-Z"); \
		MKDIR(#BASE "/ZZZ")

	MKDIR("etc");
	MKDIR("etc/board");
	MKLETTERS(plralias);
	MKLETTERS(plrobjs);
	MKLETTERS(plrs);
	MKLETTERS(plrvars);
	MKDIR("plrstuff");
	MKDIR("plrstuff/depot");
	MKLETTERS(plrstuff/depot);
	MKDIR("plrstuff/house");
	MKDIR("stat");
	
	#undef MKLETTERS
	#undef MKDIR

	boot_profiler.next_step("Initialization of text IDs");
	log("Init TextId list.");
	TextId::init();

	boot_profiler.next_step("Resetting the game time");
	log("Resetting the game time:");
	reset_time();

	boot_profiler.next_step("Reading credits, help, bground, info & motds.");
	log("Reading credits, help, bground, info & motds.");
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(RULES_FILE, &rules);
	file_to_string_alloc(HELP_PAGE_FILE, &help);
	file_to_string_alloc(INFO_FILE, &info);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
	file_to_string_alloc(POLICIES_FILE, &policies);
	file_to_string_alloc(HANDBOOK_FILE, &handbook);
	file_to_string_alloc(BACKGROUND_FILE, &background);
	file_to_string_alloc(NAME_RULES_FILE, &name_rules);
	if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
		prune_crlf(GREETINGS);

	boot_profiler.next_step("Loading new skills definitions");
    log("Loading NEW skills definitions");
	pugi::xml_document doc;

	Skill::Load(XMLLoad(LIB_MISC SKILLS_FILE, SKILLS_MAIN_TAG, SKILLS_ERROR_STR, doc));
	
	boot_profiler.next_step("Loading criterion");
	log("Loading Criterion...");
	pugi::xml_document doc1;
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "finger", "Error Loading Criterion.xml: <finger>", doc1), EWearFlag::ITEM_WEAR_FINGER);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "neck", "Error Loading Criterion.xml: <neck>", doc1), EWearFlag::ITEM_WEAR_NECK);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "body", "Error Loading Criterion.xml: <body>", doc1), EWearFlag::ITEM_WEAR_BODY);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "head", "Error Loading Criterion.xml: <head>", doc1), EWearFlag::ITEM_WEAR_HEAD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "legs", "Error Loading Criterion.xml: <legs>", doc1), EWearFlag::ITEM_WEAR_LEGS);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "feet", "Error Loading Criterion.xml: <feet>", doc1), EWearFlag::ITEM_WEAR_FEET);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "hands", "Error Loading Criterion.xml: <hands>", doc1), EWearFlag::ITEM_WEAR_HANDS);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "arms", "Error Loading Criterion.xml: <arms>", doc1), EWearFlag::ITEM_WEAR_ARMS);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "shield", "Error Loading Criterion.xml: <shield>", doc1), EWearFlag::ITEM_WEAR_SHIELD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "about", "Error Loading Criterion.xml: <about>", doc1), EWearFlag::ITEM_WEAR_ABOUT);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "waist", "Error Loading Criterion.xml: <waist>", doc1), EWearFlag::ITEM_WEAR_WAIST);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "wrist", "Error Loading Criterion.xml: <wrist>", doc1), EWearFlag::ITEM_WEAR_WRIST);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "wield", "Error Loading Criterion.xml: <wield>", doc1), EWearFlag::ITEM_WEAR_WIELD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "hold", "Error Loading Criterion.xml: <hold>", doc1), EWearFlag::ITEM_WEAR_HOLD);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "boths", "Error Loading Criterion.xml: <boths>", doc1), EWearFlag::ITEM_WEAR_BOTHS);

	boot_profiler.next_step("Loading birth places definitions");
	log("Loading birth places definitions.");
	BirthPlace::Load(XMLLoad(LIB_MISC BIRTH_PLACES_FILE, BIRTH_PLACE_MAIN_TAG, BIRTH_PLACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading player races definitions");
    log("Loading player races definitions.");
	PlayerRace::Load(XMLLoad(LIB_MISC PLAYER_RACE_FILE, RACE_MAIN_TAG, PLAYER_RACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading spell definitions");
	log("Loading spell definitions.");
	mag_assign_spells();

	boot_profiler.next_step("Loading feature definitions");
	log("Loading feature definitions.");
	assign_feats();

	boot_profiler.next_step("Loading ingredients magic");
	log("Booting IM");
	init_im();

	boot_profiler.next_step("Loading zone types and ingredient for each zone type");
	log("Booting zone types and ingredient types for each zone type.");
	init_zone_types();

	boot_profiler.next_step("Loading insert_wanted.lst");
	log("Booting insert_wanted.lst.");
	iwg.init();

	boot_profiler.next_step("Loading gurdians");
	log("Load guardians.");
	load_guardians();

	boot_profiler.next_step("Loading world");
	boot_world();

	boot_profiler.next_step("Loading stuff load table");
	log("Booting stuff load table.");
	oload_table.init();

	boot_profiler.next_step("Loading setstuff table");
	log("Booting setstuff table.");
	OBJ_DATA::init_set_table();

	boot_profiler.next_step("Loading item levels");
	log("Init item levels.");
	ObjSystem::init_item_levels();

	boot_profiler.next_step("Loading help entries");
	log("Loading help entries.");
	index_boot(DB_BOOT_HLP);

	boot_profiler.next_step("Loading social entries");
	log("Loading social entries.");
	index_boot(DB_BOOT_SOCIAL);

	boot_profiler.next_step("Loading players index");
	log("Generating player index.");
	build_player_index();

	// хэши читать после генерации плеер-таблицы
	boot_profiler.next_step("Loading CRC system");
	log("Loading file crc system.");
	FileCRC::load();

	boot_profiler.next_step("Loading fight messages");
	log("Loading fight messages.");
	load_messages();

	boot_profiler.next_step("Assigning function pointers");
	log("Assigning function pointers:");
	if (!no_specials)
	{
		log("   Mobiles.");
		assign_mobiles();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}

	boot_profiler.next_step("Assigning spells and skills levels");
	log("Assigning spell and skill levels.");
	init_spell_levels();

	boot_profiler.next_step("Sorting command list");
	log("Sorting command list.");
	sort_commands();

	boot_profiler.next_step("Reading banned site, proxy, privileges and invalid-name list.");
	log("Reading banned site, proxy, privileges and invalid-name list.");
	ban = new BanList();
	Read_Invalid_List();

	boot_profiler.next_step("Loading rented objects info");
	log("Booting rented objects info");
	zone_rnum i;
	for (i = 0; i <= top_of_p_table; i++)
	{
		(player_table + i)->timer = NULL;
		Crash_read_timer(i, FALSE);
	}

	// последовательность лоада кланов/досок не менять
	boot_profiler.next_step("Loading boards");
	log("Booting boards");
	Board::BoardInit();

	boot_profiler.next_step("loading clans");
	log("Booting clans");
	Clan::ClanLoad();

	// загрузка списка именных вещей
	boot_profiler.next_step("Loading named stuff");
	log("Load named stuff");
	NamedStuff::load();

	boot_profiler.next_step("Loading basic values");
	log("Booting basic values");
	init_basic_values();

	boot_profiler.next_step("Loading grouping parameters");
	log("Booting grouping parameters");
	if (init_grouping())
		exit(1);

	boot_profiler.next_step("Loading special assignments");
	log("Booting special assignment");
	init_spec_procs();

	boot_profiler.next_step("Loading guilds");
	log("Booting guilds");
	init_guilds();

	boot_profiler.next_step("Loading portals for 'town portal' spell");
	log("Booting portals for 'town portal' spell");
	portals_list = NULL;
	init_portals();

	boot_profiler.next_step("Loading made items");
	log("Booting maked items");
	init_make_items();

	boot_profiler.next_step("Loading exchange");
	log("Booting exchange");
	exchange_database_load();

	boot_profiler.next_step("Loading scheduled reboot time");
	log("Load schedule reboot time");
	load_sheduled_reboot();

	boot_profiler.next_step("Loading proxies list");
	log("Load proxy list");
	LoadProxyList();

	boot_profiler.next_step("Loading new_name list");
	log("Load new_name list");
	NewNameLoad();

	boot_profiler.next_step("Loading global UID timer");
	log("Load global uid counter");
	LoadGlobalUID();

	boot_profiler.next_step("Initializing DT list");
	log("Init DeathTrap list.");
	DeathTrap::load();

	boot_profiler.next_step("Loading titles list");
	log("Load Title list.");
	TitleSystem::load_title_list();

	boot_profiler.next_step("Loading emails list");
	log("Load registered emails list.");
	RegisterSystem::load();

	boot_profiler.next_step("Loading privileges and gods list");
	log("Load privilege and god list.");
	Privilege::load();

	// должен идти до резета зон
	boot_profiler.next_step("Initializing depot system");
	log("Init Depot system.");
	Depot::init_depot();

	boot_profiler.next_step("Loading Parcel system");
	log("Init Parcel system.");
	Parcel::load();

	boot_profiler.next_step("Loading celebrates");
	log("Load Celebrates."); //Polud праздники. используются при ресете зон
	//Celebrates::load(XMLLoad(LIB_MISC CELEBRATES_FILE, CELEBRATES_MAIN_TAG, CELEBRATES_ERROR_STR));
	Celebrates::load();

	// резет должен идти после лоада всех шмоток вне зон (хранилища и т.п.)
	boot_profiler.next_step("Resetting zones");
	for (i = 0; i <= top_of_zone_table; i++)
	{
		log("Resetting %s (rooms %d-%d).", zone_table[i].name,
			(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
		reset_zone(i);
	}
	reset_q.head = reset_q.tail = NULL;

	// делается после резета зон, см камент к функции
	boot_profiler.next_step("Loading depot chests");
	log("Load depot chests.");
	Depot::load_chests();

	boot_profiler.next_step("Loading glory");
	log("Load glory.");
	Glory::load_glory();
	log("Load const glory.");
	GloryConst::load();
	log("Load glory log.");
	GloryMisc::load_log();

	//Polud грузим параметры рас мобов
	boot_profiler.next_step("Loading mob races");
	log("Load mob races.");
	load_mobraces();

	boot_profiler.next_step("Loading morphs");
	log("Load morphs.");
	load_morphs();

	boot_profiler.next_step("Initializing global drop list");
	log("Init global drop list.");
	GlobalDrop::init();

	boot_profiler.next_step("Loading offtop block list");
	log("Init offtop block list.");
	OfftopSystem::init();

	boot_profiler.next_step("Loading shop_ext list");
	log("load shop_ext list start.");
	ShopExt::load(false);
	log("load shop_ext list stop.");

	boot_profiler.next_step("Loading zone average mob_level");
	log("Set zone average mob_level.");
	set_zone_mob_level();

	boot_profiler.next_step("Setting zone town");
	log("Set zone town.");
	set_zone_town();

	boot_profiler.next_step("Initializing town shop_keepers");
	log("Init town shop_keepers.");
	town_shop_keepers();

	/* Commented out until full implementation
	boot_profiler.next_step("Loading craft system");
	log("Starting craft system.");
	if (!craft::start())
	{
		log("ERROR: Failed to start craft system.\n");
	}
	*/

	boot_profiler.next_step("Loading big sets in rent");
	log("Check big sets in rent.");
	SetSystem::check_rented();

	// сначала сеты, стата мобов, потом дроп сетов
	boot_profiler.next_step("Loading object sets/mob_stat/drop_sets lists");
	obj_sets::load();
	log("Load mob_stat.xml");
	mob_stat::load();
	log("Init SetsDrop lists.");
	SetsDrop::init();

	boot_profiler.next_step("Loading remorts");
	log("Load remort.xml");
	Remort::init();

	boot_profiler.next_step("Loading noob_help.xml");
	log("Load noob_help.xml");
	Noob::init();

	boot_profiler.next_step("Loading reset_stats.xml");
	log("Load reset_stats.xml");
	ResetStats::init();

	boot_profiler.next_step("Loading mail.xml");
	log("Load mail.xml");
	mail::load();
	
	// загрузка кейсов
	boot_profiler.next_step("Loading cases");
	load_cases();
	
	// справка должна иниться после всего того, что может в нее что-то добавить
	boot_profiler.next_step("Reloading help system");
	HelpSystem::reload_all();

	boot_profiler.next_step("Loading bonus log");
	Bonus::bonus_log_load();

	boot_time = time(0);
	log("Boot db -- DONE.");
}

// reset the time in the game from file
void reset_time(void)
{
	time_info = *mud_time_passed(time(0), beginning_of_time);
	// Calculate moon day
	weather_info.moon_day =
		((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % MOON_CYCLE;
	weather_info.week_day_mono =
		((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % WEEK_CYCLE;
	weather_info.week_day_poly =
		((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % POLY_WEEK_CYCLE;
	// Calculate Easter
	calc_easter();

	if (time_info.hours < sunrise[time_info.month][0])
		weather_info.sunlight = SUN_DARK;
	else if (time_info.hours == sunrise[time_info.month][0])
		weather_info.sunlight = SUN_RISE;
	else if (time_info.hours < sunrise[time_info.month][1])
		weather_info.sunlight = SUN_LIGHT;
	else if (time_info.hours == sunrise[time_info.month][1])
		weather_info.sunlight = SUN_SET;
	else
		weather_info.sunlight = SUN_DARK;

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);

	weather_info.temperature = (year_temp[time_info.month].med * 4 +
								number(year_temp[time_info.month].min, year_temp[time_info.month].max)) / 5;
	weather_info.pressure = 960;
	if ((time_info.month >= MONTH_MAY) && (time_info.month <= MONTH_NOVEMBER))
		weather_info.pressure += dice(1, 50);
	else
		weather_info.pressure += dice(1, 80);

	weather_info.change = 0;
	weather_info.weather_type = 0;

	if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_MAY)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month == MONTH_MART && weather_info.temperature >= 3)
		weather_info.season = SEASON_SPRING;
	else if (time_info.month >= MONTH_JUNE && time_info.month <= MONTH_AUGUST)
		weather_info.season = SEASON_SUMMER;
	else if (time_info.month >= MONTH_SEPTEMBER && time_info.month <= MONTH_OCTOBER)
		weather_info.season = SEASON_AUTUMN;
	else if (time_info.month == MONTH_NOVEMBER && weather_info.temperature >= 3)
		weather_info.season = SEASON_AUTUMN;
	else
		weather_info.season = SEASON_WINTER;

	if (weather_info.pressure <= 980)
		weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.pressure <= 1000)
	{
		weather_info.sky = SKY_RAINING;
		if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_OCTOBER)
			create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 40, 40, 20);
		else if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
			create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 50, 40, 10);
		else if (time_info.month == MONTH_NOVEMBER || time_info.month == MONTH_MART)
		{
			if (weather_info.temperature >= 3)
				create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 70, 30, 0);
			else
				create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 80, 20, 0);
		}
	}
	else if (weather_info.pressure <= 1020)
		weather_info.sky = SKY_CLOUDY;
	else
		weather_info.sky = SKY_CLOUDLESS;
}



// generate index table for the player file
void build_player_index(void)
{
	new_build_player_index();
	return;
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *      -gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE * fl)
{
	char key[READ_SIZE], next_key[READ_SIZE];
	char line[READ_SIZE], *scan;
	int total_keywords = 0;

	// get the first keyword line
	get_one_line(fl, key);

	while (*key != '$')  	// skip the text
	{
		do
		{
			get_one_line(fl, line);
			if (feof(fl))
				goto ackeof;
		}
		while (*line != '#');

		// now count keywords
		scan = key;
		do
		{
			scan = one_word(scan, next_key);
			if (*next_key)
				++total_keywords;
		}
		while (*next_key);

		// get next keyword line (or $)
		get_one_line(fl, key);

		if (feof(fl))
			goto ackeof;
	}

	return (total_keywords);

	// No, they are not evil. -gg 6/24/98
ackeof:
	mudlog("SYSERR: Unexpected end of help file.", DEF, LVL_IMMORT, SYSLOG, TRUE);
	return (total_keywords);
}

// function to count how many hash-mark delimited records exist in a file
int count_hash_records(FILE * fl)
{
	char buf[128];
	int count = 0;

	while (fgets(buf, 128, fl))
		if (*buf == '#')
			count++;

	return (count);
}

int count_social_records(FILE * fl, int *messages, int *keywords)
{
	char key[READ_SIZE], next_key[READ_SIZE];
	char line[READ_SIZE], *scan;

	// get the first keyword line
	get_one_line(fl, key);

	while (*key != '$')  	// skip the text
	{
		do
		{
			get_one_line(fl, line);
			if (feof(fl))
				goto ackeof;
		}
		while (*line != '#');

		// now count keywords
		scan = key;
		++(*messages);
		do
		{
			scan = one_word(scan, next_key);
			if (*next_key)
				++(*keywords);
		}
		while (*next_key);

		// get next keyword line (or $)
		get_one_line(fl, key);

		if (feof(fl))
			goto ackeof;
	}

	return (TRUE);

	// No, they are not evil. -gg 6/24/98
ackeof:
	log("SYSERR: Unexpected end of help file.");
	exit(1);		// Some day we hope to handle these things better...
}

const char* get_file_prefix(const int mode)
{
	const char* prefix = nullptr;
	switch (mode)
	{
	case DB_BOOT_TRG:
		prefix = TRG_PREFIX;
		break;

	case DB_BOOT_WLD:
		prefix = WLD_PREFIX;
		break;

	case DB_BOOT_MOB:
		prefix = MOB_PREFIX;
		break;

	case DB_BOOT_OBJ:
		prefix = OBJ_PREFIX;
		break;

	case DB_BOOT_ZON:
		prefix = ZON_PREFIX;
		break;

	case DB_BOOT_HLP:
		prefix = HLP_PREFIX;
		break;

	case DB_BOOT_SOCIAL:
		prefix = SOC_PREFIX;
		break;

	default:
		break;
	}

	return prefix;
}

int calculate_records_count(FILE* index, const int mode)
{
	int rec_count = 0;

	if (mode == DB_BOOT_WLD)
	{
		return rec_count;	// we don't need to count rooms
	}

	const char* prefix = get_file_prefix(mode);
	// first, count the number of records in the file so we can malloc
	int dummyi = fscanf(index, "%s\n", buf1);
	while (*buf1 != '$')
	{
		sprintf(buf2, "%s%s", prefix, buf1);
		FILE* db_file = fopen(buf2, "r");
		if (!db_file)
		{
			log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix, INDEX_FILE, strerror(errno));
			dummyi = fscanf(index, "%s\n", buf1);
			continue;
		}
		else
		{
			if (mode == DB_BOOT_ZON)
				rec_count++;
			else if (mode == DB_BOOT_SOCIAL)
				rec_count += count_social_records(db_file, &top_of_socialm, &top_of_socialk);
			else if (mode == DB_BOOT_HLP)
				rec_count += count_alias_records(db_file);
			else if (mode == DB_BOOT_WLD)
			{
				const int counter = count_hash_records(db_file);
				if (counter > 99)
				{
					log("SYSERR: File '%s' list more than 99 room", buf2);
					exit(1);
				}
				rec_count += (counter + 1);
			}
			else
				rec_count += count_hash_records(db_file);
		}
		fclose(db_file);
		dummyi = fscanf(index, "%s\n", buf1);
	}
	UNUSED_ARG(dummyi);

	// Exit if 0 records, unless this is shops
	if (!rec_count)
	{
		log("SYSERR: boot error - 0 records counted in %s/%s.", prefix, INDEX_FILE);
		exit(1);
	}

	// Any idea why you put this here Jeremy?
	rec_count++;

	return rec_count;
}

void index_boot(int mode)
{
	FILE *index, *db_file;

	log("Index booting %d", mode);

	const char* prefix = get_file_prefix(mode);
	if (nullptr == prefix)
	{
		log("SYSERR: Unknown subcommand %d to index_boot!", mode);
		exit(1);
	}

	sprintf(buf2, "%s%s", prefix, INDEX_FILE);

	if (!(index = fopen(buf2, "r")))
	{
		log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
		exit(1);
	}

	const int rec_count = calculate_records_count(index, mode);

	// * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	switch (mode)
	{
	case DB_BOOT_TRG:
		CREATE(trig_index, rec_count);
		break;

	case DB_BOOT_WLD:
		{
			// Creating empty world with NOWHERE room.
			world.push_back(new ROOM_DATA);
			top_of_world = FIRST_ROOM;
			const size_t rooms_bytes = sizeof(ROOM_DATA) * rec_count;
			log("   %d rooms, %zd bytes.", rec_count, rooms_bytes);
		}
		break;

	case DB_BOOT_MOB:
		{
			mob_proto = new CHAR_DATA[rec_count]; // TODO: переваять на вектор (+в medit)
			CREATE(mob_index, rec_count);
			const size_t index_size = sizeof(INDEX_DATA) * rec_count;
			const size_t characters_size = sizeof(CHAR_DATA) * rec_count;
			log("   %d mobs, %zd bytes in index, %zd bytes in prototypes.", rec_count, index_size, characters_size);
		}
		break;

	case DB_BOOT_OBJ:
		log("   %d objs, ~%zd bytes in index, ~%zd bytes in prototypes.", rec_count, obj_proto.index_size(), obj_proto.prototypes_size());
		break;

	case DB_BOOT_ZON:
		{
			CREATE(zone_table, rec_count);
			const size_t zones_size = sizeof(struct zone_data) * rec_count;
			log("   %d zones, %zd bytes.", rec_count, zones_size);
		}
		break;

	case DB_BOOT_HLP:
		break;

	case DB_BOOT_SOCIAL:
		{
			CREATE(soc_mess_list, top_of_socialm + 1);
			CREATE(soc_keys_list, top_of_socialk + 1);
			const size_t messages_size = sizeof(struct social_messg) * (top_of_socialm + 1);
			const size_t keywords_size = sizeof(struct social_keyword) * (top_of_socialk + 1);
			log("   %d entries(%d keywords), %zd(%zd) bytes.", top_of_socialm + 1,
				top_of_socialk + 1, messages_size, keywords_size);
		}
		break;
	}

	rewind(index);
	int dummyi = fscanf(index, "%s\n", buf1);
	while (*buf1 != '$')
	{
		sprintf(buf2, "%s%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r")))
		{
			log("SYSERR: %s: %s", buf2, strerror(errno));
			exit(1);
		}
		switch (mode)
		{
		case DB_BOOT_TRG:
		case DB_BOOT_WLD:
		case DB_BOOT_OBJ:
		case DB_BOOT_MOB:
			discrete_load(db_file, mode, buf2);
			break;
		case DB_BOOT_ZON:
			load_zones(db_file, buf2);
			break;
		case DB_BOOT_HLP:
			/*
			 * If you think about it, we have a race here.  Although, this is the
			 * "point-the-gun-at-your-own-foot" type of race.
			 */
			load_help(db_file);
			break;
		case DB_BOOT_SOCIAL:
			load_socials(db_file);
			break;
		}
		if (mode == DB_BOOT_WLD)
			parse_room(db_file, 0, TRUE);
		fclose(db_file);
		dummyi = fscanf(index, "%s\n", buf1);
	}
	UNUSED_ARG(dummyi);

	fclose(index);
	// Create virtual room for zone

	// sort the social index
	if (mode == DB_BOOT_SOCIAL)
	{
		qsort(soc_keys_list, top_of_socialk + 1, sizeof(struct social_keyword), csort);
	}
}

char fread_letter(FILE * fp)
{
	char c;
	do
	{
		c = getc(fp);
	} while (isspace(c));
	return c;
}

void parse_mobile(FILE * mob_f, int nr)
{
	static int i = 0;
	int j, t[10];
	char line[256], letter;
	char f1[128], f2[128];

	mob_index[i].vnum = nr;
	mob_index[i].number = 0;
	mob_index[i].func = NULL;
	mob_index[i].set_idx = -1;

	/*
	* Mobiles should NEVER use anything in the 'player_specials' structure.
	* The only reason we have every mob in the game share this copy of the
	* structure is to save newbie coders from themselves. -gg 2/25/98
	*/
	mob_proto[i].player_specials = &dummy_mob;
	sprintf(buf2, "mob vnum %d", nr);

	// **** String data
	char *tmp_str = fread_string(mob_f, buf2);
	mob_proto[i].set_pc_name(tmp_str);
	if (tmp_str)
	{
		free(tmp_str);
	}
	tmp_str = fread_string(mob_f, buf2);
	mob_proto[i].set_npc_name(tmp_str);
	if (tmp_str)
	{
		free(tmp_str);
	}

	// real name
	CREATE(GET_PAD(mob_proto + i, 0), mob_proto[i].get_npc_name().size() + 1);
	strcpy(GET_PAD(mob_proto + i, 0), mob_proto[i].get_npc_name().c_str());
	for (j = 1; j < OBJ_DATA::NUM_PADS; j++)
	{
		GET_PAD(mob_proto + i, j) = fread_string(mob_f, buf2);
	}

	mob_proto[i].player_data.long_descr = fread_string(mob_f, buf2);
	mob_proto[i].player_data.description = fread_string(mob_f, buf2);
	mob_proto[i].mob_specials.Questor = NULL;
	mob_proto[i].player_data.title = NULL;

	// mob_proto[i].mob_specials.Questor = fread_string(mob_f, buf2);

	// *** Numeric data ***
	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}', but file ended!\n%s", nr, line);
		exit(1);
	}
#ifdef CIRCLE_ACORN		// Ugh.
	if (sscanf(line, "%s %s %d %s", f1, f2, t + 2, &letter) != 4)
	{
#else
	if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4)
	{
#endif
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}'\n%s", nr, line);
		exit(1);
	}
	MOB_FLAGS(&mob_proto[i]).from_string(f1);
	MOB_FLAGS(&mob_proto[i]).set(MOB_ISNPC);
	AFF_FLAGS(&mob_proto[i]).from_string(f2);
	GET_ALIGNMENT(mob_proto + i) = t[2];
	switch (UPPER(letter))
	{
	case 'S':		// Simple monsters
		parse_simple_mob(mob_f, i, nr);
		break;
	case 'E':		// Circle3 Enhanced monsters
		parse_enhanced_mob(mob_f, i, nr);
		break;
		// add new mob types here..
	default:
		log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
		exit(1);
	}

	// DG triggers -- script info follows mob S/E section
	// DG triggers -- script is defined after the end of the room 'T'
	// Ингредиентная магия -- 'I'
	// Объекты загружаемые по-смертно -- 'D'

	do
	{
		letter = fread_letter(mob_f);
		ungetc(letter, mob_f);
		switch (letter)
		{
		case 'I':
			get_line(mob_f, line);
			im_parse(&mob_proto[i].ing_list, line + 1);
			break;
		case 'L':
			get_line(mob_f, line);
			dl_parse(&mob_proto[i].dl_list, line + 1);
			break;
		case 'T':
			dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
			break;
		default:
			letter = 0;
			break;
		}
	} while (letter != 0);

	for (j = 0; j < NUM_WEARS; j++)
	{
		mob_proto[i].equipment[j] = NULL;
	}

	mob_proto[i].nr = i;
	mob_proto[i].desc = NULL;

	set_test_data(mob_proto + i);

	top_of_mobt = i++;
}

void discrete_load(FILE * fl, int mode, char *filename)
{
	int nr = -1, last;
	char line[256];

	const char *modes[] = { "world", "mob", "obj", "ZON", "SHP", "HLP", "trg" };
	// modes positions correspond to DB_BOOT_xxx in db.h

	for (;;)
	{
		/*
		* we have to do special processing with the obj files because they have
		* no end-of-record marker :(
		*/
		if (mode != DB_BOOT_OBJ || nr < 0)
			if (!get_line(fl, line))
			{
				if (nr == -1)
				{
					log("SYSERR: %s file %s is empty!", modes[mode], filename);
				}
				else
				{
					log("SYSERR: Format error in %s after %s #%d\n"
						"...expecting a new %s, but file ended!\n"
						"(maybe the file is not terminated with '$'?)", filename,
						modes[mode], nr, modes[mode]);
				}
				exit(1);
			}

		if (*line == '$')
			return;
		// This file create ADAMANT MUD ETITOR ?
		if (strcmp(line, "#ADAMANT") == 0)
			continue;

		if (*line == '#')
		{
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1)
			{
				log("SYSERR: Format error after %s #%d", modes[mode], last);
				exit(1);
			}
			if (nr == 1)
			{
				log("SYSERR: Entity with vnum 1, filename=%s", filename);
				exit(1);
			}
			if (nr >= MAX_PROTO_NUMBER)
				return;
			else
				switch (mode)
				{
				case DB_BOOT_TRG:
					parse_trigger(fl, nr);
					break;
				case DB_BOOT_WLD:
					parse_room(fl, nr, FALSE);
					break;
				case DB_BOOT_MOB:
					parse_mobile(fl, nr);
					break;
				case DB_BOOT_OBJ:
					strcpy(line, parse_object(fl, nr));
					break;
				}
		}
		else
		{
			log("SYSERR: Format error in %s file %s near %s #%d", modes[mode], filename, modes[mode], nr);
			log("SYSERR: ... offending line: '%s'", line);
			exit(1);
		}
	}
}

// * Проверки всяких несочетаемых флагов на комнатах.
void check_room_flags(int rnum)
{
	if (DeathTrap::is_slow_dt(rnum))
	{
		// снятие номагик и прочих флагов, запрещающих чару выбраться из комнаты без выходов при наличии медленного дт
		GET_ROOM(rnum)->unset_flag(ROOM_NOMAGIC);
		GET_ROOM(rnum)->unset_flag(ROOM_NOTELEPORTOUT);
		GET_ROOM(rnum)->unset_flag(ROOM_NOSUMMON);
	}
	if (GET_ROOM(rnum)->get_flag(ROOM_HOUSE)
		&& (SECT(rnum) == SECT_MOUNTAIN || SECT(rnum) == SECT_HILLS))
	{
		// шоб в замках умные не копали
		SECT(rnum) = SECT_INSIDE;
	}
}

// load the rooms
void parse_room(FILE * fl, int virtual_nr, int virt)
{
	static int room_nr = FIRST_ROOM, zone = 0;
	int t[10], i;
	char line[256], flags[128];
	char letter;

	if (virt)
	{
		virtual_nr = zone_table[zone].top;
	}

	sprintf(buf2, "room #%d%s", virtual_nr, virt ? "(virt)" : "");

	if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1))
	{
		log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
		exit(1);
	}
	while (virtual_nr > zone_table[zone].top)
		if (++zone > top_of_zone_table)
		{
			log("SYSERR: Room %d is outside of any zone.", virtual_nr);
			exit(1);
		}
	// Создаем новую комнату
	world.push_back(new ROOM_DATA);

	world[room_nr]->zone = zone;
	world[room_nr]->number = virtual_nr;
	if (virt)
	{
		world[room_nr]->name = str_dup("Виртуальная комната");
		world[room_nr]->description_num = RoomDescription::add_desc("Похоже, здесь вам делать нечего.");
		world[room_nr]->clear_flags();
		world[room_nr]->sector_type = SECT_SECRET;
	}
	else
	{
		// не ставьте тут конструкцию ? : , т.к. gcc >=4.х вызывает в ней fread_string два раза
		world[room_nr]->name = fread_string(fl, buf2);
		if (!world[room_nr]->name)
			world[room_nr]->name = str_dup("");

		// тож временная галиматья
		char * temp_buf = fread_string(fl, buf2);
		if (!temp_buf)
			temp_buf = str_dup("");
		else
		{
			std::string buffer(temp_buf);
			boost::trim_right_if(buffer, boost::is_any_of(std::string(" _"))); //убираем пробелы в конце строки
			RECREATE(temp_buf, strlen(buffer.c_str()) + 1);
			strcpy(temp_buf, buffer.c_str());
		}
		world[room_nr]->description_num = RoomDescription::add_desc(temp_buf);
		free(temp_buf);

		if (!get_line(fl, line))
		{
			log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!", virtual_nr);
			exit(1);
		}

		if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3)
		{
			log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
			exit(1);
		}
		// t[0] is the zone number; ignored with the zone-file system
		world[room_nr]->flags_from_string(flags);
		world[room_nr]->sector_type = t[2];
	}

	check_room_flags(room_nr);

	// Обнуляем флаги от аффектов и сами аффекты на комнате.
	world[room_nr]->affected_by.clear();
	// Обнуляем базовые параметры (пока нет их загрузки)
	memset(&world[room_nr]->base_property, 0, sizeof(room_property_data));

	// Обнуляем добавочные параметры комнаты
	memset(&world[room_nr]->add_property, 0, sizeof(room_property_data));


	world[room_nr]->func = NULL;
	world[room_nr]->contents = NULL;
	world[room_nr]->people = NULL;
	world[room_nr]->track = NULL;
	world[room_nr]->light = 0;	// Zero light sources
	world[room_nr]->fires = 0;
	world[room_nr]->gdark = 0;
	world[room_nr]->glight = 0;
	world[room_nr]->ing_list = NULL;	// ингредиентов нет
	world[room_nr]->proto_script.clear();

	for (i = 0; i < NUM_OF_DIRS; i++)
		world[room_nr]->dir_option[i] = NULL;

	world[room_nr]->ex_description = NULL;
	if (virt)
	{
		top_of_world = room_nr++;
		return;
	}

	sprintf(buf, "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

	for (;;)
	{
		if (!get_line(fl, line))
		{
			log("%s", buf);
			exit(1);
		}
		switch (*line)
		{
		case 'D':
			setup_dir(fl, room_nr, atoi(line + 1));
			break;

		case 'E':
			{
				const EXTRA_DESCR_DATA::shared_ptr new_descr(new EXTRA_DESCR_DATA);
				new_descr->keyword = fread_string(fl, buf2);
				new_descr->description = fread_string(fl, buf2);
				if (new_descr->keyword && new_descr->description)
				{
					new_descr->next = world[room_nr]->ex_description;
					world[room_nr]->ex_description = new_descr;
				}
				else
				{
					sprintf(buf, "SYSERR: Format error in room #%d (Corrupt extradesc)", virtual_nr);
					log("%s", buf);
				}
			}
			break;

		case 'S':	// end of room
			// DG triggers -- script is defined after the end of the room 'T'
			// Ингредиентная магия -- 'I'
			do
			{
				letter = fread_letter(fl);
				ungetc(letter, fl);
				switch (letter)
				{
				case 'I':
					get_line(fl, line);
					im_parse(&(world[room_nr]->ing_list), line + 1);
					break;
				case 'T':
					dg_read_trigger(fl, world[room_nr], WLD_TRIGGER);
					break;
				default:
					letter = 0;
					break;
				}
			}
			while (letter != 0);
			top_of_world = room_nr++;
			return;

		default:
			log("%s", buf);
			exit(1);
		}
	}
}

// read direction data
void setup_dir(FILE * fl, int room, unsigned dir)
{
	if (dir >= world[room]->dir_option.size())
	{
		log("SYSERROR : dir=%d (%s:%d)", dir, __FILE__, __LINE__);
		return;
	}

	int t[5];
	char line[256];

	sprintf(buf2, "room #%d, direction D%u", GET_ROOM_VNUM(room), dir);

	world[room]->dir_option[dir].reset(new EXIT_DATA());
	const std::shared_ptr<char> general_description(fread_string(fl, buf2));
	world[room]->dir_option[dir]->general_description = general_description.get();

	// парс строки алиаса двери на имя;вининельный падеж, если он есть
	char *alias = fread_string(fl, buf2);
	if (alias && *alias)
	{
		std::string buffer(alias);
		std::string::size_type i = buffer.find('|');
		if (i != std::string::npos)
		{
			world[room]->dir_option[dir]->keyword = str_dup(buffer.substr(0, i).c_str());
			world[room]->dir_option[dir]->vkeyword = str_dup(buffer.substr(++i).c_str());
		}
		else
		{
			world[room]->dir_option[dir]->keyword = str_dup(buffer.c_str());
			world[room]->dir_option[dir]->vkeyword = str_dup(buffer.c_str());
		}
	}
	free(alias);

	if (!get_line(fl, line))
	{
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}
    int result = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3);
	if (result == 3)//Polud видимо "старый" формат (20.10.2010), прочитаем в старом
	{
		if (t[0] & 1)
			world[room]->dir_option[dir]->exit_info = EX_ISDOOR;
		else if (t[0] & 2)
			world[room]->dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
		else
			world[room]->dir_option[dir]->exit_info = 0;
		if (t[0] & 4)
			world[room]->dir_option[dir]->exit_info |= EX_HIDDEN;

		world[room]->dir_option[dir]->lock_complexity = 0;
	}
	else if (result == 4)
	{
		world[room]->dir_option[dir]->exit_info = t[0];
		world[room]->dir_option[dir]->lock_complexity = t[3];
	}
	else
	{
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}

	world[room]->dir_option[dir]->key = t[1];
	world[room]->dir_option[dir]->to_room = t[2];
}


// make sure the start rooms exist & resolve their vnums to rnums
void check_start_rooms(void)
{
	if ((r_mortal_start_room = real_room(mortal_start_room)) == NOWHERE)
	{
		log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d", mortal_start_room);
		exit(1);
	}

	if ((r_immort_start_room = real_room(immort_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}

	if ((r_frozen_start_room = real_room(frozen_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}

	if ((r_helled_start_room = real_room(helled_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
		r_helled_start_room = r_mortal_start_room;
	}

	if ((r_named_start_room = real_room(named_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
		r_named_start_room = r_mortal_start_room;
	}

	if ((r_unreg_start_room = real_room(unreg_start_room)) == NOWHERE)
	{
		log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
		r_unreg_start_room = r_mortal_start_room;
	}
}

// resolve all vnums into rnums in the world
void renum_world(void)
{
	int room, door;

	for (room = FIRST_ROOM; room <= top_of_world; room++)
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (world[room]->dir_option[door])
				if (world[room]->dir_option[door]->to_room != NOWHERE)
					world[room]->dir_option[door]->to_room =
						real_room(world[room]->dir_option[door]->to_room);
}

// Установка принадлежности к зоне в прототипах
void renum_obj_zone(void)
{
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		obj_proto.zone(i, real_zone(obj_proto[i]->get_vnum()));
	}
}

// Установкапринадлежности к зоне в индексе
void renum_mob_zone(void)
{
	int i;
	for (i = 0; i <= top_of_mobt; ++i)
	{
		mob_index[i].zone = real_zone(mob_index[i].vnum);
	}
}

#define ZCMD zone_table[zone].cmd[cmd_no]
#define ZCMD_CMD(cmd_nom) zone_table[zone].cmd[cmd_nom]

void renum_single_table(int zone)
{
	int cmd_no, a, b, c, olda, oldb, oldc;
	char buf[128];

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	{
		a = b = c = 0;
		olda = ZCMD.arg1;
		oldb = ZCMD.arg2;
		oldc = ZCMD.arg3;
		switch (ZCMD.command)
		{
		case 'M':
			a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
			c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'F':
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			b = ZCMD.arg2 = real_mobile(ZCMD.arg2);
			c = ZCMD.arg3 = real_mobile(ZCMD.arg3);
			break;
		case 'O':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			if (ZCMD.arg3 != NOWHERE)
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'G':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			break;
		case 'E':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			break;
		case 'P':
			a = ZCMD.arg1 = real_object(ZCMD.arg1);
			c = ZCMD.arg3 = real_object(ZCMD.arg3);
			break;
		case 'D':
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			break;
		case 'R':	// rem obj from room
			a = ZCMD.arg1 = real_room(ZCMD.arg1);
			b = ZCMD.arg2 = real_object(ZCMD.arg2);
			break;
		case 'Q':
			a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
			break;
		case 'T':	// a trigger
			// designer's choice: convert this later
			// b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
			b = real_trigger(ZCMD.arg2);	// leave this in for validation
			if (ZCMD.arg1 == WLD_TRIGGER)
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
			break;
		case 'V':	// trigger variable assignment
			if (ZCMD.arg1 == WLD_TRIGGER)
				b = ZCMD.arg2 = real_room(ZCMD.arg2);
			break;
		}
		if (a < 0 || b < 0 || c < 0)
		{
			sprintf(buf, "Invalid vnum %d, cmd disabled", (a < 0) ? olda : ((b < 0) ? oldb : oldc));
			log_zone_error(zone, cmd_no, buf);
			ZCMD.command = '*';
		}
	}
}

// resove vnums into rnums in the zone reset tables
void renum_zone_table(void)
{
	zone_rnum zone;
	for (zone = 0; zone <= top_of_zone_table; zone++)
	{
		renum_single_table(zone);
	}
}


void parse_simple_mob(FILE * mob_f, int i, int nr)
{
	int j, t[10];
	char line[256];

	mob_proto[i].set_str(11);
	mob_proto[i].set_int(11);
	mob_proto[i].set_wis(11);
	mob_proto[i].set_dex(11);
	mob_proto[i].set_con(11);
	mob_proto[i].set_cha(11);

	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
		exit(1);
	}
	if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
			   t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9)
	{
		log("SYSERR: Format error in mob #%d, 1th line\n" "...expecting line of form '# # # #d#+# #d#+#'", nr);
		exit(1);
	}

	mob_proto[i].set_level(t[0]);
	mob_proto[i].real_abils.hitroll = 20 - t[1];
	mob_proto[i].real_abils.armor = 10 * t[2];

	// max hit = 0 is a flag that H, M, V is xdy+z
	GET_MAX_HIT(mob_proto + i) = 0;
	GET_MEM_TOTAL(mob_proto + i) = t[3];
	GET_MEM_COMPLETED(mob_proto + i) = t[4];
	mob_proto[i].points.hit = t[5];

	mob_proto[i].points.move = 100;
	mob_proto[i].points.max_move = 100;

	mob_proto[i].mob_specials.damnodice = t[6];
	mob_proto[i].mob_specials.damsizedice = t[7];
	mob_proto[i].real_abils.damroll = t[8];

	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error in mob #%d, 2th line\n"
			"...expecting line of form '#d#+# #', but file ended!", nr);
		exit(1);
	}
	if (sscanf(line, " %dd%d+%d %d", t, t + 1, t + 2, t + 3) != 4)
	{
		log("SYSERR: Format error in mob #%d, 2th line\n" "...expecting line of form '#d#+# #'", nr);
		exit(1);
	}

	mob_proto[i].set_gold(t[2], false);
	GET_GOLD_NoDs(mob_proto + i) = t[0];
	GET_GOLD_SiDs(mob_proto + i) = t[1];
	mob_proto[i].set_exp(t[3]);

	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error in 3th line of mob #%d\n"
			"...expecting line of form '# # #', but file ended!", nr);
		exit(1);
	}

	switch (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3))
	{
	case 3:
		mob_proto[i].mob_specials.speed = -1;
		break;
	case 4:
		mob_proto[i].mob_specials.speed = t[3];
		break;
	default:
		log("SYSERR: Format error in 3th line of mob #%d\n" "...expecting line of form '# # # #'", nr);
		exit(1);
	}

	mob_proto[i].char_specials.position = t[0];
	mob_proto[i].mob_specials.default_pos = t[1];
	mob_proto[i].player_data.sex = static_cast<ESex>(t[2]);

	mob_proto[i].player_data.Race = NPC_RACE_BASIC;
	mob_proto[i].set_class(NPC_CLASS_BASE);
	mob_proto[i].player_data.weight = 200;
	mob_proto[i].player_data.height = 198;

	/*
	 * these are now save applies; base save numbers for MOBs are now from
	 * the warrior save table.
	 */
	for (j = 0; j < SAVING_COUNT; j++)
		GET_SAVE(mob_proto + i, j) = 0;
}



/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
	struct helper_data_type *helper;
	int k, num_arg, matched = 0, t[7];

	num_arg = atoi(value);

//Added by Adept
	CASE("Resistances")
	{
		if (sscanf(value, "%d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6) != 7)
		{
			log("SYSERROR : Excepted format <# # # # # # #> for RESISTANCES in MOB #%d", i);
			return;
		}
		for (k = 0; k < MAX_NUMBER_RESISTANCE; k++)
			GET_RESIST(mob_proto + i, k) = MIN(300, MAX(-1000, t[k]));
	}

	CASE("Saves")
	{
		if (sscanf(value, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4)
		{
			log("SYSERROR : Excepted format <# # # #> for SAVES in MOB #%d", i);
			return;
		}
		for (k = 0; k < SAVING_COUNT; k++)
			GET_SAVE(mob_proto + i, k) = MIN(200, MAX(-200, t[k]));
	}

	CASE("HPReg")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.hitreg = num_arg;
	}

	CASE("Armour")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.armour = num_arg;
	}

	CASE("PlusMem")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.manareg = num_arg;
	}

	CASE("CastSuccess")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.cast_success = num_arg;
	}

	CASE("Success")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.morale_add = num_arg;
	}

	CASE("Initiative")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.initiative_add = num_arg;
	}

	CASE("Absorbe")
	{
		RANGE(-200, 200);
		mob_proto[i].add_abils.absorb = num_arg;
	}
	CASE("AResist")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.aresist = num_arg;
	}
	CASE("MResist")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.mresist = num_arg;
	}
//End of changed
	// added by WorM (Видолюб) поглощение физ.урона в %
	CASE("PResist")
	{
		RANGE(0, 100);
		mob_proto[i].add_abils.presist = num_arg;
	}
	// end by WorM
	CASE("BareHandAttack")
	{
		RANGE(0, 99);
		mob_proto[i].mob_specials.attack_type = num_arg;
	}

	CASE("Destination")
	{
		if (mob_proto[i].mob_specials.dest_count < MAX_DEST)
		{
			mob_proto[i].mob_specials.dest[mob_proto[i].mob_specials.dest_count] = num_arg;
			mob_proto[i].mob_specials.dest_count++;
		}
	}

	CASE("Str")
	{
		mob_proto[i].set_str(num_arg);
	}

	CASE("StrAdd")
	{
		mob_proto[i].set_str_add(num_arg);
	}

	CASE("Int")
	{
		RANGE(3, 50);
		mob_proto[i].set_int(num_arg);
	}

	CASE("Wis")
	{
		RANGE(3, 50);
		mob_proto[i].set_wis(num_arg);
	}

	CASE("Dex")
	{
		mob_proto[i].set_dex(num_arg);
	}

	CASE("Con")
	{
		mob_proto[i].set_con(num_arg);
	}

	CASE("Cha")
	{
		RANGE(3, 50);
		mob_proto[i].set_cha(num_arg);
	}

	CASE("Size")
	{
		RANGE(0, 100);
		mob_proto[i].real_abils.size = num_arg;
	}
	// *** Extended for Adamant
	CASE("LikeWork")
	{
		RANGE(0, 200);
		mob_proto[i].mob_specials.LikeWork = num_arg;
	}

	CASE("MaxFactor")
	{
		RANGE(0, 255);
		mob_proto[i].mob_specials.MaxFactor = num_arg;
	}

	CASE("ExtraAttack")
	{
		RANGE(0, 255);
		mob_proto[i].mob_specials.ExtraAttack = num_arg;
	}

	CASE("Class")
	{
		RANGE(NPC_CLASS_BASE, NPC_CLASS_LAST);
		mob_proto[i].set_class(num_arg);
	}


	CASE("Height")
	{
		RANGE(0, 200);
		mob_proto[i].player_data.height = num_arg;
	}

	CASE("Weight")
	{
		RANGE(0, 200);
		mob_proto[i].player_data.weight = num_arg;
	}

	CASE("Race")
	{
		RANGE(NPC_RACE_BASIC, NPC_RACE_NEXT - 1);
		mob_proto[i].player_data.Race = num_arg;
	}

	CASE("Special_Bitvector")
	{
		mob_proto[i].mob_specials.npc_flags.from_string((char *) value);
		// *** Empty now
	}

	// Gorrah
	CASE("Feat")
	{
		if (sscanf(value, "%d", t) != 1)
		{
			log("SYSERROR : Excepted format <#> for FEAT in MOB #%d", i);
			return;
		}
		if (t[0] >= MAX_FEATS || t[0] <= 0)
		{
			log("SYSERROR : Unknown feat No %d for MOB #%d", t[0], i);
			return;
		}
		SET_FEAT(mob_proto + i, t[0]);
	}
	// End of changes

	CASE("Skill")
	{
		if (sscanf(value, "%d %d", t, t + 1) != 2)
		{
			log("SYSERROR : Excepted format <# #> for SKILL in MOB #%d", i);
			return;
		}
		if (t[0] > MAX_SKILL_NUM || t[0] < 1)
		{
			log("SYSERROR : Unknown skill No %d for MOB #%d", t[0], i);
			return;
		}
		t[1] = MIN(200, MAX(0, t[1]));
		(mob_proto + i)->set_skill(static_cast<ESkill>(t[0]), t[1]);
	}

	CASE("Spell")
	{
		if (sscanf(value, "%d", t + 0) != 1)
		{
			log("SYSERROR : Excepted format <#> for SPELL in MOB #%d", i);
			return;
		}
		if (t[0] > MAX_SPELLS || t[0] < 1)
		{
			log("SYSERROR : Unknown spell No %d for MOB #%d", t[0], i);
			return;
		}
		GET_SPELL_MEM(mob_proto + i, t[0]) += 1;
		GET_CASTER(mob_proto + i) += (IS_SET(spell_info[t[0]].routines, NPC_CALCULATE) ? 1 : 0);
		// log("Set spell %d to %d(%s)", t[0], GET_SPELL_MEM(mob_proto + i, t[0]), GET_NAME(mob_proto + i));
	}

	CASE("Helper")
	{
		CREATE(helper, 1);
		helper->mob_vnum = num_arg;
		helper->next_helper = GET_HELPER(mob_proto + i);
		GET_HELPER(mob_proto + i) = helper;
	}

	CASE("Role")
	{
		if (value && *value)
		{
			std::string str(value);
			boost::dynamic_bitset<> tmp(str);
			tmp.resize(mob_proto[i].role_.size());
			mob_proto[i].role_ = tmp;
		}
	}

	if (!matched)
	{
		log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d", keyword, nr);
	}
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
	char *ptr;

	if ((ptr = strchr(buf, ':')) != NULL)
	{
		*(ptr++) = '\0';
		while (a_isspace(*ptr))
			ptr++;
#if 0				// Need to evaluate interpret_espec()'s NULL handling.
	}
#else
	}
	else
	{
		ptr = NULL;
	}
#endif
	interpret_espec(buf, ptr, i, nr);
}

void parse_enhanced_mob(FILE * mob_f, int i, int nr)
{
	char line[256];

	parse_simple_mob(mob_f, i, nr);

	while (get_line(mob_f, line))
	{
		if (!strcmp(line, "E"))	// end of the enhanced section
			return;
		else if (*line == '#')  	// we've hit the next mob, maybe?
		{
			log("SYSERR: Unterminated E section in mob #%d", nr);
			exit(1);
		}
		else
			parse_espec(line, i, nr);
	}

	log("SYSERR: Unexpected end of file reached after mob #%d", nr);
	exit(1);
}

// Make own name by process aliases
int trans_obj_name(OBJ_DATA * obj, CHAR_DATA * ch)
{
	// ищем метку @p , @p1 ... и заменяем на падежи.
	int i, k;
	for (i = 0; i < OBJ_DATA::NUM_PADS; i++)
	{
		std::string obj_pad = GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i);
		size_t j = obj_pad.find("@p");
		if (std::string::npos != j && 0 < j)
		{
			// Родитель найден прописываем его.
			k = atoi(obj_pad.substr(j + 2, j + 3).c_str());
			obj_pad.replace(j, 3, GET_PAD(ch, k));

			obj->set_PName(i, obj_pad);
			// Если имя в именительном то дублируем запись
			if (i == 0)
			{
				obj->set_short_description(obj_pad);
				obj->set_aliases(obj_pad); // ставим алиасы
			}
		}
	};
	obj->set_is_rename(true); // присвоим флажок что у шмотки заменены падежи
	return (TRUE);
}

void dl_list_copy(load_list * *pdst, load_list * src)
{
	load_list::iterator p;
	if (src == NULL)
	{
		*pdst = NULL;
		return;
	}
	else
	{
		*pdst = new load_list;
		p = src->begin();
		while (p != src->end())
		{
			(*pdst)->push_back(*p);
			p++;
		}
	}
}


// Dead load list object load
int dl_load_obj(OBJ_DATA * corpse, CHAR_DATA * ch, CHAR_DATA * chr, int DL_LOAD_TYPE)
{
	bool last_load = true;
	bool load = false;
	bool miw;
	load_list::iterator p;
	OBJ_DATA *tobj;

	if (mob_proto[GET_MOB_RNUM(ch)].dl_list == NULL)
		return FALSE;

	p = mob_proto[GET_MOB_RNUM(ch)].dl_list->begin();

	while (p != mob_proto[GET_MOB_RNUM(ch)].dl_list->end())
	{
		switch ((*p)->load_type)
		{
		case DL_LOAD_ANYWAY:
			last_load = load;
		case DL_LOAD_ANYWAY_NC:
			break;
		case DL_LOAD_IFLAST:
			last_load = load && last_load;
		case DL_LOAD_IFLAST_NC:
			load = load && last_load;
			break;
		}
		// Блокируем лоад в зависимости от значения смецпараметра
		if ((*p)->spec_param != DL_LOAD_TYPE)
			load = false;
		else
			load = true;
		if (load)
		{
			tobj = read_object((*p)->obj_vnum, VIRTUAL);
			if (tobj == NULL)
			{
				sprintf(buf,
						"Попытка загрузки в труп (VNUM:%d) несуществующего объекта (VNUM:%d).",
						GET_MOB_VNUM(ch), (*p)->obj_vnum);
				mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			}
			else
			{
				// Проверяем мах_ин_ворлд и вероятность загрузки, если это необходимо для такого DL_LOAD_TYPE
				if (GET_OBJ_MIW(tobj) >= obj_proto.actual_count(tobj->get_rnum())
					|| GET_OBJ_MIW(tobj) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
					|| check_unlimited_timer(tobj))
				{
					miw = true;
				}
				else
				{
					miw = false;
				}
				switch (DL_LOAD_TYPE)
				{
				case DL_ORDINARY:	//Обычная загрузка - без выкрутасов
					if (miw && (number(1, 100) <= (*p)->load_prob))
						load = true;
					else
						load = false;
					break;
				case DL_PROGRESSION:	//Загрузка с убывающей до 0.01 вероятностью
					if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
						load = true;
					else
						load = false;
					break;
				case DL_SKIN:	//Загрузка по применению "освежевать"
					if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
						load = true;
					else
						load = false;
					if (chr == NULL)
						load = false;
					break;
				}
				if (load)
				{
					tobj->set_zone(world[ch->in_room]->zone);
					tobj->set_parent(GET_MOB_VNUM(ch));
					if (DL_LOAD_TYPE == DL_SKIN)
					{
						trans_obj_name(tobj, ch);
					}
					// Добавлена проверка на отсутствие трупа
					if (MOB_FLAGGED(ch, MOB_CORPSE))
					{
						act("На земле остал$U лежать $o.", FALSE, ch, tobj, 0, TO_ROOM);
						obj_to_room(tobj, ch->in_room);
					}
					else
					{
						if ((DL_LOAD_TYPE == DL_SKIN) && (corpse->get_carried_by() == chr))
						{
							can_carry_obj(chr, tobj);
						}
						else
						{
							obj_to_obj(tobj, corpse);
						}
					}
				}
				else
				{
					extract_obj(tobj);
					load = false;
				}

			}
		}
		p++;
	}
	return (TRUE);
}

// Dead load list object parse
int dl_parse(load_list ** dl_list, char *line)
{
	// Формат парсинга D {номер прототипа} {вероятность загрузки} {спец поле - тип загрузки}
	int vnum, prob, type, spec;
	struct load_data *new_item;

	if (sscanf(line, "%d %d %d %d", &vnum, &prob, &type, &spec) != 4)
	{
		// Ошибка чтения.
		log("SYSERR: Parse death load list (bad param count).");
		return FALSE;
	};
	// проверяем существование прототипа в мире (предметы уже должны быть загружены)
	if (*dl_list == NULL)
	{
		// Создаем новый список.
		*dl_list = new load_list;
	}

	CREATE(new_item, 1);
	new_item->obj_vnum = vnum;
	new_item->load_prob = prob;
	new_item->load_type = type;
	new_item->spec_param = spec;

	(*dl_list)->push_back(new_item);
	return TRUE;
}

namespace {

int test_levels[] = {
	100,
	300,
	600,
	1000,
	1500,
	2100,
	2900,
	3900,
	5100,
	6500, // 10
	8100,
	10100,
	12500,
	15300,
	18500,
	22500,
	27300,
	32900,
	39300,
	46500, // 20
	54500,
	64100,
	75300,
	88100,
	102500,
	117500,
	147500,
	192500,
	252500,
	327500, // 30
	417500,
	522500,
	642500,
	777500,
	927500,
	1127500,
	1427500,
	1827500,
	2327500,
	2927500, // 40
	3627500,
	4427500,
	5327500,
	6327500,
	7427500,
	8627500,
	9927500,
	11327500,
	12827500,
	14427500
};

} // namespace

int calc_boss_value(CHAR_DATA *mob, int num)
{
	if (mob->get_role(MOB_ROLE_BOSS))
	{
		num += num * 25 / 100;
	}
	return num;
}

void set_test_data(CHAR_DATA *mob)
{
	if (!mob)
	{
		log("SYSERROR: null mob (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (GET_EXP(mob) == 0)
	{
		return;
	}

	if (GET_EXP(mob) > test_levels[49])
	{
		// log("test1: %s - %d -> %d", mob->get_name(), mob->get_level(), 50);
		mob->set_level(50);
	}
	else
	{
		if (mob->get_level() == 0)
		{
			for (int i = 0; i < 50; ++i)
			{
				if (test_levels[i] >= GET_EXP(mob))
				{
					// log("test2: %s - %d -> %d", mob->get_name(), mob->get_level(), i + 1);
					mob->set_level(i + 1);
					break;
				}
			}
		}
	}

	if (mob->get_level() > 30)
	{
		// -10..-86
		int min_save = -(10 + 4 * (mob->get_level() - 31));
		min_save = calc_boss_value(mob, min_save);

		for (int i = 0; i < 4; ++i)
		{
			if (GET_SAVE(mob, i) > min_save)
			{
				//log("test3: %s - %d -> %d", mob->get_name(), GET_SAVE(mob, i), min_save);
				GET_SAVE(mob, i) = min_save;
			}
		}
		// 20..77
		int min_cast = 20 + 3 * (mob->get_level() - 31);
		min_cast = calc_boss_value(mob, min_cast);

		if (GET_CAST_SUCCESS(mob) < min_cast)
		{
			//log("test4: %s - %d -> %d", mob->get_name(), GET_CAST_SUCCESS(mob), min_cast);
			GET_CAST_SUCCESS(mob) = min_cast;
		}
	}

	// поглощение пока принудительно всем
	GET_ABSORBE(mob) = calc_boss_value(mob, mob->get_level());
}

// read all objects from obj file; generate index and prototypes
char *parse_object(FILE * obj_f, const int vnum)
{
	static int i = 0;
	static char line[256];
	int t[10], j = 0, retval;
	char *tmpptr;
	char f0[256], f1[256], f2[256];

	OBJ_DATA *tobj = new OBJ_DATA(vnum);

	tobj->set_rnum(i);

	// *** Add some initialization fields
	tobj->set_maximum_durability(OBJ_DATA::DEFAULT_MAXIMUM_DURABILITY);
	tobj->set_current_durability(OBJ_DATA::DEFAULT_CURRENT_DURABILITY);
	tobj->set_sex(DEFAULT_SEX);
	tobj->set_timer(OBJ_DATA::DEFAULT_TIMER);
	tobj->set_level(1);
	tobj->set_destroyer(OBJ_DATA::DEFAULT_DESTROYER);

	sprintf(buf2, "object #%d", vnum);

	// *** string data ***
	const char* aliases = fread_string(obj_f, buf2);
	if (aliases == nullptr)
	{
		log("SYSERR: Null obj name or format error at or near %s", buf2);
		exit(1);
	}
	tobj->set_aliases(aliases);
	tmpptr = fread_string(obj_f, buf2);
	*tmpptr = LOWER(*tmpptr);
	tobj->set_short_description(tmpptr);

	tobj->set_PName(0, tobj->get_short_description());

	for (j = 1; j < OBJ_DATA::NUM_PADS; j++)
	{
		char* str = fread_string(obj_f, buf2);
		*str = LOWER(*str);
		tobj->set_PName(j, str);
	}

	tmpptr = fread_string(obj_f, buf2);
	if (tmpptr && *tmpptr)
	{
		CAP(tmpptr);
	}
	tobj->set_description(tmpptr ? tmpptr : "");

	auto action_description = fread_string(obj_f, buf2);
	tobj->set_action_description(action_description ? action_description : "");

	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting *1th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %s %d %d %d", f0, t + 1, t + 2, t + 3)) != 4)
	{
		log("SYSERR: Format error in *1th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}

	int skill = 0;
	asciiflag_conv(f0, &skill);
	tobj->set_skill(skill);

	tobj->set_maximum_durability(t[1]);
	tobj->set_current_durability(MIN(t[1], t[2]));
	tobj->set_material(static_cast<OBJ_DATA::EObjectMaterial>(t[3]));
	
	if (tobj->get_current_durability() > tobj->get_maximum_durability())
	{
		log("SYSERR: Obj_cur > Obj_Max, vnum: %d", vnum);
	}
	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting *2th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) != 4)
	{
		log("SYSERR: Format error in *2th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->set_sex(static_cast<ESex>(t[0]));
	int timer = t[1] > 0 ? t[1] : OBJ_DATA::SEVEN_DAYS;
	// шмоток с бесконечным таймером проставленным через olc или текстовый редактор
	// не должно быть
	if (timer == OBJ_DATA::UNLIMITED_TIMER)
	{
	    timer--;
		tobj->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);
	}
	tobj->set_timer(timer);
	tobj->set_spell(t[2]);
	tobj->set_level(t[3]);

	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %s %s %s", f0, f1, f2)) != 3)
	{
		log("SYSERR: Format error in *3th* numeric line (expecting 3 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->load_affect_flags(f0);
	// ** Affects
	tobj->load_anti_flags(f1);
	// ** Miss for ...
	tobj->load_no_flags(f2);
	// ** Deny for ...

	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting *3th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %d %s %s", t, f1, f2)) != 3)
	{
		log("SYSERR: Format error in *3th* misc line (expecting 3 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->set_type(static_cast<OBJ_DATA::EObjectType>(t[0]));	    // ** What's a object
	tobj->load_extra_flags(f1);
	// ** Its effects
	int wear_flags = 0;
	asciiflag_conv(f2, &wear_flags);
	tobj->set_wear_flags(wear_flags);
	// ** Wear on ...

	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting *5th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, "%s %d %d %d", f0, t + 1, t + 2, t + 3)) != 4)
	{
		log("SYSERR: Format error in *5th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	int first_value = 0;
	asciiflag_conv(f0, &first_value);
	tobj->set_val(0, first_value);
	tobj->set_val(1, t[1]);
	tobj->set_val(2, t[2]);
	tobj->set_val(3, t[3]);

	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting *6th* numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4)
	{
		log("SYSERR: Format error in *6th* numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}
	tobj->set_weight(t[0]);
	tobj->set_cost(t[1]);
	tobj->set_rent_off(t[2]);
	tobj->set_rent_on(t[3]);

	// check to make sure that weight of containers exceeds curr. quantity
	if (tobj->get_type() == OBJ_DATA::ITEM_DRINKCON
		|| tobj->get_type() == OBJ_DATA::ITEM_FOUNTAIN)
	{
		if (tobj->get_weight() < tobj->get_val(1))
		{
			tobj->set_weight(tobj->get_val(1) + 5);
		}
	}

	// *** extra descriptions and affect fields ***
	strcat(buf2, ", after numeric constants\n" "...expecting 'E', 'A', '$', or next object number");
	j = 0;

	for (;;)
	{
		if (!get_line(obj_f, line))
		{
			log("SYSERR: Format error in %s", buf2);
			exit(1);
		}
		switch (*line)
		{
		case 'E':
			{
				const EXTRA_DESCR_DATA::shared_ptr new_descr(new EXTRA_DESCR_DATA());
				new_descr->keyword = fread_string(obj_f, buf2);
				new_descr->description = fread_string(obj_f, buf2);
				if (new_descr->keyword && new_descr->description)
				{
					new_descr->next = tobj->get_ex_description();
					tobj->set_ex_description(new_descr);
				}
				else
				{
					sprintf(buf, "SYSERR: Format error in %s (Corrupt extradesc)", buf2);
					log("%s", buf);
				}
			}
			break;

		case 'A':
			if (j >= MAX_OBJ_AFFECT)
			{
				log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
				exit(1);
			}
			if (!get_line(obj_f, line))
			{
				log("SYSERR: Format error in 'A' field, %s\n"
					"...expecting 2 numeric constants but file ended!", buf2);
				exit(1);
			}
			if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2)
			{
				log("SYSERR: Format error in 'A' field, %s\n"
					"...expecting 2 numeric arguments, got %d\n"
					"...offending line: '%s'", buf2, retval, line);
				exit(1);
			}
			tobj->set_affected(j, static_cast<EApplyLocation>(t[0]), t[1]);
			j++;
			break;

		case 'T':	// DG triggers
			dg_obj_trigger(line, tobj);
			break;

		case 'M':
			tobj->set_max_in_world(atoi(line + 1));
			break;

		case 'R':
			tobj->set_minimum_remorts(atoi(line + 1));
			break;

		case 'S':
			if (!get_line(obj_f, line))
			{
				log("SYSERR: Format error in 'S' field, %s\n"
					"...expecting 2 numeric constants but file ended!", buf2);
				exit(1);
			}
			if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2)
			{
				log("SYSERR: Format error in 'S' field, %s\n"
					"...expecting 2 numeric arguments, got %d\n"
					"...offending line: '%s'", buf2, retval, line);
				exit(1);
			}
			tobj->set_skill(t[0], t[1]);
			break;
		case 'V':
			tobj->init_values_from_zone(line + 1);
			break;

		case '$':
		case '#':
			check_object(tobj);		// Anton Gorev (2015/12/29): do we need the result of this check?
			obj_proto.add(tobj, vnum);
			return line;

		default:
			log("SYSERR: Format error in %s", buf2);
			exit(1);
		}
	}
}

#define Z       zone_table[zone]

// load the zone table and command tables
void load_zones(FILE * fl, char *zonename)
{
	static zone_rnum zone = 0;
	int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, a_number = 0, b_number = 0;
	char *ptr, buf[256], zname[256];
	char t1[80], t2[80];
//MZ.load
	Z.level = 1;
	Z.type = 0;
//-MZ.load
	Z.typeA_count = 0;
	Z.typeB_count = 0;
	Z.locked = FALSE;
	Z.reset_idle = FALSE;
	Z.used = FALSE;
	Z.activity = 0;
	Z.comment = 0;
	Z.location = 0;
	Z.description = 0;
	Z.group = false;
	Z.count_reset = 0;
	strcpy(zname, zonename);

	while (get_line(fl, buf))
	{
		ptr = buf;
		skip_spaces(&ptr);
		if (*ptr == 'A')
			Z.typeA_count++;
		if (*ptr == 'B')
			Z.typeB_count++;
		num_of_cmds++;	// this should be correct within 3 or so
	}
	rewind(fl);
	if (Z.typeA_count)
	{
		CREATE(Z.typeA_list, Z.typeA_count);
	}
	if (Z.typeB_count)
	{
		CREATE(Z.typeB_list, Z.typeB_count);
		CREATE(Z.typeB_flag, Z.typeB_count);
		// сбрасываем все флаги
		for (b_number = Z.typeB_count; b_number > 0; b_number--)
			Z.typeB_flag[b_number - 1] = FALSE;
	}

	if (num_of_cmds == 0)
	{
		log("SYSERR: %s is empty!", zname);
		exit(1);
	}
	else
	{
		CREATE(Z.cmd, num_of_cmds);
	}

	line_num += get_line(fl, buf);

	if (sscanf(buf, "#%d", &Z.number) != 1)
	{
		log("SYSERR: Format error in %s, line %d", zname, line_num);
		exit(1);
	}
	sprintf(buf2, "beginning of zone #%d", Z.number);

	line_num += get_line(fl, buf);
	if ((ptr = strchr(buf, '~')) != NULL)	// take off the '~' if it's there
		*ptr = '\0';
	Z.name = str_dup(buf);
	line_num += get_line(fl, buf);
	if (*buf == '^')
	{
		std::string comment(buf);
		boost::trim_if(comment, boost::is_any_of(std::string("^~")));
		Z.comment = str_dup(comment.c_str());
		line_num += get_line(fl, buf);
	}
	if (*buf == '&')
	{
		std::string location(buf);
		boost::trim_if(location, boost::is_any_of(std::string("&~")));
		Z.location = str_dup(location.c_str());
		line_num += get_line(fl, buf);
	}
	if (*buf == '$')
	{
		std::string description(buf);
		boost::trim_if(description, boost::is_any_of(std::string("$~")));
		Z.description = str_dup(description.c_str());
		line_num += get_line(fl, buf);
	}

	if (*buf == '#')
	{
		int group = 0;
		if (sscanf(buf, "#%d %d %d", &Z.level, &Z.type, &group) < 3)
		{
			if (sscanf(buf, "#%d %d", &Z.level, &Z.type) < 2)
			{
				log("SYSERR: ошибка чтения z.level, z.type, z.group: %s", buf);
				exit(1);
			}
		}
		Z.group = (group == 0)? 1: group; //группы в 0 рыл не бывает
		line_num += get_line(fl, buf);
	}
	*t1 = 0;
	*t2 = 0;
	int tmp_reset_idle = 0;
	if (sscanf(buf, " %d %d %d %d %s %s", &Z.top, &Z.lifespan, &Z.reset_mode, &tmp_reset_idle, t1, t2) < 4)
	{
		// если нет четырех констант, то, возможно, это старый формат -- попробуем прочитать три
		if (sscanf(buf, " %d %d %d %s %s", &Z.top, &Z.lifespan, &Z.reset_mode, t1, t2) < 3)
		{
			log("SYSERR: Format error in 3-constant line of %s", zname);
			exit(1);
		}
	}
	Z.reset_idle = 0 != tmp_reset_idle;
	Z.under_construction = !str_cmp(t1, "test");
	Z.locked = !str_cmp(t2, "locked");
	cmd_no = 0;

	for (;;)
	{
		if ((tmp = get_line(fl, buf)) == 0)
		{
			log("SYSERR: Format error in %s - premature end of file", zname);
			exit(1);
		}
		line_num += tmp;
		ptr = buf;
		skip_spaces(&ptr);

		if ((ZCMD.command = *ptr) == '*')
			continue;
		ptr++;
		// Новые параметры формата файла:
		// A номер_зоны -- зона типа A из списка
		// B номер_зоны -- зона типа B из списка
		if (ZCMD.command == 'A')
		{
			sscanf(ptr, " %d", &Z.typeA_list[a_number]);
			a_number++;
			continue;
		}
		if (ZCMD.command == 'B')
		{
			sscanf(ptr, " %d", &Z.typeB_list[b_number]);
			b_number++;
			continue;
		}

		if (ZCMD.command == 'S' || ZCMD.command == '$')
		{
			ZCMD.command = 'S';
			break;
		}
		error = 0;
		ZCMD.arg4 = -1;
		if (strchr("MOEGPDTVQF", ZCMD.command) == NULL)  	// a 3-arg command
		{
			if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
				error = 1;
		}
		else if (ZCMD.command == 'V')  	// a string-arg command
		{
			if (sscanf(ptr, " %d %d %d %d %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, t1, t2) != 6)
				error = 1;
			else
			{
				ZCMD.sarg1 = str_dup(t1);
				ZCMD.sarg2 = str_dup(t2);
			}
		}
		else if (ZCMD.command == 'Q')  	// a number command
		{
			if (sscanf(ptr, " %d %d", &tmp, &ZCMD.arg1) != 2)
				error = 1;
			else
				tmp = 0;
		}
		else
		{
			if (sscanf(ptr, " %d %d %d %d %d", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4) < 4)
				error = 1;
		}

		ZCMD.if_flag = tmp;

		if (error)
		{
			log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
			exit(1);
		}
		ZCMD.line = line_num;
		cmd_no++;
	}
	top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE * fl, char *buf)
{
	if (fgets(buf, READ_SIZE, fl) == NULL)
	{
		mudlog("SYSERR: error reading help file: not terminated with $?", DEF, LVL_IMMORT, SYSLOG, TRUE);
		buf[0] = '$';
		buf[1] = 0;
		return;
	}
	buf[strlen(buf) - 1] = '\0';	// take off the trailing \n
}

void load_help(FILE * fl)
{
#if defined(CIRCLE_MACINTOSH)
	static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];	// ?
#else
	char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
	char line[READ_SIZE + 1], *scan;

	// get the first keyword line
	get_one_line(fl, key);
	while (*key != '$')  	// read in the corresponding help entry
	{
		strcpy(entry, strcat(key, "\r\n"));
		get_one_line(fl, line);
		while (*line != '#')
		{
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(fl, line);
		}
		// Assign read level
		int min_level = 0;
		if ((*line == '#') && (*(line + 1) != 0))
		{
			min_level = atoi((line + 1));
		}
		min_level = MAX(0, MIN(min_level, LVL_IMPL));
		// now, add the entry to the index with each keyword on the keyword line
		std::string entry_str(entry);
		scan = one_word(key, next_key);
		while (*next_key)
		{
			std::string key_str(next_key);
			HelpSystem::add_static(key_str, entry_str, min_level);
			scan = one_word(scan, next_key);
		}

		// get next keyword line (or $)
		get_one_line(fl, key);
	}
}

int csort(const void *a, const void *b)
{
	const struct social_keyword *a1, *b1;

	a1 = (const struct social_keyword *) a;
	b1 = (const struct social_keyword *) b;

	return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/



int vnum_mobile(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_mobt; nr++)
	{
		if (isname(searchname, mob_proto[nr].get_pc_name()))
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum, mob_proto[nr].get_npc_name().c_str());
			send_to_char(buf, ch);
		}
	}
	return (found);
}

int vnum_object(char *searchname, CHAR_DATA * ch)
{
	int found = 0;

	for (size_t nr = 0; nr < obj_proto.size(); nr++)
	{
		if (isname(searchname, obj_proto[nr]->get_aliases()))
		{
			++found;
			sprintf(buf, "%3d. [%5d] %s\r\n",
				found, obj_proto[nr]->get_vnum(),
				obj_proto[nr]->get_short_description().c_str());
			send_to_char(buf, ch);
		}
	}
	return (found);
}

int vnum_flag(char *searchname, CHAR_DATA * ch)
{
	int found = 0, plane = 0, counter = 0, plane_offset = 0;
	bool f = FALSE;
// type:
// 0 -- неизвестный тип
// 1 -- объекты
// 2 -- мобы
// 4 -- комнаты
// Ищем для объектов в списках: extra_bits[], apply_types[], weapon_affects[]
// Ищем для мобов в списках  action_bits[], function_bits[],  affected_bits[], preference_bits[]
// Ищем для комнат в списках room_bits[]
	std::string out;

	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++)
	{
		if (*extra_bits[counter] == '\n')
		{
			plane++;
			plane_offset = 0;
			continue;
		};
		if (is_abbrev(searchname, extra_bits[counter]))
		{
			f = TRUE;
			break;
		}
		plane_offset++;
	}

	if (f)
	{
		for (const auto i : obj_proto)
		{
			if (i->get_extra_flag(plane, 1 << plane_offset))
			{
				snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s :   %s\r\n",
					++found, i->get_vnum(), i->get_short_description().c_str(), extra_bits[counter]);
				out += buf;
			}
		}
	}
	f = FALSE;
// ---------------------
	for (counter = 0; *apply_types[counter] != '\n'; counter++)
	{
		if (is_abbrev(searchname, apply_types[counter]))
		{
			f = TRUE;
			break;
		}
	}
	if (f)
	{
		for (const auto i : obj_proto)
		{
			for (plane = 0; plane < MAX_OBJ_AFFECT; plane++)
			{
				if (i->get_affected(plane).location == counter)
				{
					snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s :   %s\r\n",
						++found, i->get_vnum(),
						i->get_short_description().c_str(),
						apply_types[counter]);
					out += buf;
					continue;
				}
			}
		}
	}
	f = FALSE;
// ---------------------
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++)
	{
		if (*weapon_affects[counter] == '\n')
		{
			plane++;
			plane_offset = 0;
			continue;
		};
		if (is_abbrev(searchname, weapon_affects[counter]))
		{
			f = TRUE;
			break;
		}
		plane_offset++;
	}
	if (f)
	{
		for (const auto i : obj_proto)
		{
			if (i->get_extra_flag(plane, 1 << (plane_offset)))
			{
				snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s :   %s\r\n",
					++found, i->get_vnum(),
					i->get_short_description().c_str(),
					weapon_affects[counter]);
				out += buf;
			}
		}
	}

	if (!out.empty())
	{
		page_string(ch->desc, out);
	}
	return found;
}

int vnum_room(char *searchname, CHAR_DATA * ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_world; nr++)
	{
		if (isname(searchname, world[nr]->name))
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found, world[nr]->number, world[nr]->name);
			send_to_char(buf, ch);
		}
	}
	return (found);
}

namespace {

int test_hp[] = {
	20,
	40,
	60,
	80,
	100,
	120,
	140,
	160,
	180,
	200,
	220,
	240,
	260,
	280,
	300,
	340,
	380,
	420,
	460,
	520,
	580,
	640,
	700,
	760,
	840,
	1000,
	1160,
	1320,
	1480,
	1640,
	1960,
	2280,
	2600,
	2920,
	3240,
	3880,
	4520,
	5160,
	5800,
	6440,
	7720,
	9000,
	10280,
	11560,
	12840,
	15400,
	17960,
	20520,
	23080,
	25640
};

} // namespace

int get_test_hp(int lvl)
{
	if (lvl > 0 && lvl <= 50)
	{
		return test_hp[lvl - 1];
	}
	return 1;
}

// create a new mobile from a prototype
CHAR_DATA *read_mobile(mob_vnum nr, int type)
{				// and mob_rnum
	int is_corpse = 0;
	mob_rnum i;

	if (nr < 0)
	{
		is_corpse = 1;
		nr = -nr;
	}

	if (type == VIRTUAL)
	{
		if ((i = real_mobile(nr)) < 0)
		{
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (NULL);
		}
	}
	else
		i = nr;

	CHAR_DATA *mob = new CHAR_DATA;
	*mob = mob_proto[i]; //чет мне кажется что конструкции типа этой не принесут нам щастья...
	mob->set_normal_morph();
	mob->proto_script.clear();
	mob->set_next(character_list);
	character_list = mob;
//	CharacterAlias::add(mob);

	if (!mob->points.max_hit)
	{
		mob->points.max_hit = std::max(1,
			dice(GET_MEM_TOTAL(mob), GET_MEM_COMPLETED(mob)) + mob->points.hit);
	}
	else
	{
		mob->points.max_hit = std::max(1,
			number(mob->points.hit, GET_MEM_TOTAL(mob)));
	}

	int test_hp = get_test_hp(GET_LEVEL(mob));
	if (GET_EXP(mob) > 0 && mob->points.max_hit < test_hp)
	{
//		log("hp: (%s) %d -> %d", GET_NAME(mob), mob->points.max_hit, test_hp);
		mob->points.max_hit = test_hp;
	}

	mob->points.hit = mob->points.max_hit;
	GET_MEM_TOTAL(mob) = GET_MEM_COMPLETED(mob) = 0;
	GET_HORSESTATE(mob) = 800;
	GET_LASTROOM(mob) = NOWHERE;
	if (mob->mob_specials.speed <= -1)
		GET_ACTIVITY(mob) = number(0, PULSE_MOBILE - 1);
	else
		GET_ACTIVITY(mob) = number(0, mob->mob_specials.speed);
	EXTRACT_TIMER(mob) = 0;
	mob->points.move = mob->points.max_move;
	mob->add_gold(dice(GET_GOLD_NoDs(mob), GET_GOLD_SiDs(mob)));

	mob->player_data.time.birth = time(0);
	mob->player_data.time.played = 0;
	mob->player_data.time.logon = time(0);

	mob->id = max_id++;

	if (!is_corpse)
	{
		mob_index[i].number++;
		assign_triggers(mob, MOB_TRIGGER);
	}

	i = mob_index[i].zone;
	if (i != -1 && zone_table[i].under_construction)
	{
		// mobile принадлежит тестовой зоне
		MOB_FLAGS(mob).set(MOB_NOSUMMON);
	}

//Polud - поставим флаг стражнику
	guardian_type::iterator it = guardian_list.find(GET_MOB_VNUM(mob));
	if (it != guardian_list.end())
	{
		MOB_FLAGS(mob).set(MOB_GUARDIAN);
	}

	return (mob);
}

// create an object, and add it to the object list
/**
* \param alias - строка алиасов объекта (нужна уже здесь, т.к.
* сразу идет добавление в ObjectAlias). На данный момент актуально
* для трупов, остальное вроде не особо и надо видеть.
*/
OBJ_DATA *create_obj(const std::string& alias)
{
	OBJ_DATA *obj;
	NEWCREATE(obj, -1);

	obj->set_next(object_list);
	object_list = obj;
	obj->set_id(max_id++);
	obj->set_aliases(alias);

	return (obj);
}

/**
// никакая это не копия, строковые и остальные поля с выделением памяти остаются общими
// мы просто отдаем константный указатель на прототип
 * \param type по дефолту VIRTUAL
 */
CObjectPrototype::shared_ptr get_object_prototype(obj_vnum nr, int type)
{
	unsigned i = nr;
	if (type == VIRTUAL)
	{
		const int rnum = real_object(nr);
		if (rnum < 0)
		{
			log("Object (V) %d does not exist in database.", nr);
			return nullptr;
		}
		else
		{
			i = rnum;
		}
	}

	if (i > obj_proto.size())
	{
		return 0;
	}
	return obj_proto[i];
}

// create a new object from a prototype
OBJ_DATA *read_object(obj_vnum nr, int type)
{				// and obj_rnum
	OBJ_DATA *obj;
	obj_rnum i;

	if (nr < 0)
	{
		log("SYSERR: Trying to create obj with negative (%d) num!", nr);
		return (NULL);
	}
	if (type == VIRTUAL)
	{
		if ((i = real_object(nr)) < 0)
		{
			log("Object (V) %d does not exist in database.", nr);
			return (NULL);
		}
	}
	else
		i = nr;

	NEWCREATE(obj, *obj_proto[i]);
	obj_proto.inc_number(i);
	i = obj_proto.zone(i);
	if (i != -1 && zone_table[i].under_construction)
	{
		// модификация объектов тестовой зоны
		obj->set_timer(TEST_OBJECT_TIMER);
		obj->set_extra_flag(EExtraFlag::ITEM_NOLOCATE);
	}
	obj->clear_proto_script();
	obj->set_next(object_list);
	object_list = obj;
//	ObjectAlias::add(obj);
	obj->set_id(max_id++);
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
	{
		if (GET_OBJ_VAL(obj, 1)
			&& GET_OBJ_VAL(obj, 2))
		{
			name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2));
		}
	}
	assign_triggers(obj, OBJ_TRIGGER);

	return (obj);
}

// пробегаем по всем клеткам зоны и находим там чаров/чармисов, если они есть, ставит used на true
void after_reset_zone(int nr_zone)
{
	// пробегаем по дескрипторам, ибо это быстрее и проще, т.к. в зоне может быть и 200 и 300 мобов
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		// Чар должен быть в игре
		if (STATE(d) == CON_PLAYING)
		{
			if (world[d->character->in_room]->zone == nr_zone)
			{
				zone_table[nr_zone].used = true;
				return;
			}
		}
	}
}

#define ZO_DEAD  9999

// update zone ages, queue for reset if necessary, and dequeue when possible
void zone_update(void)
{
	int i, j, k = 0;
	struct reset_q_element *update_u, *temp;
	static int timer = 0;

	// jelson 10/22/92
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)  	// one minute has passed
	{
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		// since one minute has passed, increment zone ages
		for (i = 0; i <= top_of_zone_table; i++)
		{
			if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode &&
					(zone_table[i].reset_idle || zone_table[i].used))
				(zone_table[i].age)++;

			if (zone_table[i].age >= zone_table[i].lifespan &&
					zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode &&
					(zone_table[i].reset_idle || zone_table[i].used))  	// enqueue zone
			{

				CREATE(update_u, 1);
				update_u->zone_to_reset = i;
				update_u->next = 0;

				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else
				{
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}

				zone_table[i].age = ZO_DEAD;
			}
		}
	}

	// end - one minute has passed
	// dequeue zones (if possible) and reset
	// this code is executed every 10 seconds (i.e. PULSE_ZONE)
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if (zone_table[update_u->zone_to_reset].reset_mode == 2
				|| (is_empty(update_u->zone_to_reset) && zone_table[update_u->zone_to_reset].reset_mode != 3)
				|| can_be_reset(update_u->zone_to_reset))
		{
			reset_zone(update_u->zone_to_reset);
			char tmp[128];
			snprintf(tmp, sizeof(tmp), "Auto zone reset: %s (%d)",
				zone_table[update_u->zone_to_reset].name,
				zone_table[update_u->zone_to_reset].number);
			std::string out(tmp);
			if (zone_table[update_u->zone_to_reset].reset_mode == 3)
			{
				for (i = 0; i < zone_table[update_u->zone_to_reset].typeA_count; i++)
				{
					//Ищем real_zone по vnum
					for (j = 0; j <= top_of_zone_table; j++)
					{
						if (zone_table[j].number ==
							zone_table[update_u->zone_to_reset].typeA_list[i])
						{
							reset_zone(j);
							snprintf(tmp, sizeof(tmp),
								" ]\r\n[ Also resetting: %s (%d)",
								zone_table[j].name, zone_table[j].number);
							out += tmp;
							break;
						}
					}
				}
			}
			mudlog(out.c_str(), LGH, LVL_GOD, SYSLOG, FALSE);
			// dequeue
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else
			{
				for (temp = reset_q.head; temp->next != update_u; temp = temp->next);

				if (!update_u->next)
					reset_q.tail = temp;

				temp->next = update_u->next;
			}

			free(update_u);
			k++;
			if (k >= ZONES_RESET)
				break;
		}
}

bool can_be_reset(zone_rnum zone)
{
	int i = 0, j = 0;
	if (zone_table[zone].reset_mode != 3)
		return FALSE;
// проверяем себя
	if (!is_empty(zone))
		return FALSE;
// проверяем список B
	for (i = 0; i < zone_table[zone].typeB_count; i++)
	{
		//Ищем real_zone по vnum
		for (j = 0; j <= top_of_zone_table; j++)
		{
			if (zone_table[j].number == zone_table[zone].typeB_list[i])
			{
				if (!zone_table[zone].typeB_flag[i] || !is_empty(j))
					return FALSE;
				break;
			}
		}
	}
// проверяем список A
	for (i = 0; i < zone_table[zone].typeA_count; i++)
	{
		//Ищем real_zone по vnum
		for (j = 0; j <= top_of_zone_table; j++)
		{
			if (zone_table[j].number == zone_table[zone].typeA_list[i])
			{
				if (!is_empty(j))
					return FALSE;
				break;
			}
		}
	}
	return TRUE;
}

void paste_mob(CHAR_DATA *ch, room_rnum room)
{
	if (!IS_NPC(ch) || ch->get_fighting() || GET_POS(ch) < POS_STUNNED)
		return;
	if (IS_CHARMICE(ch)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD)
		|| (EXTRACT_TIMER(ch) > 0))
	{
		return;
	}
//	if (MOB_FLAGGED(ch, MOB_CORPSE))
//		return;
	if (room == NOWHERE)
		return;

	bool time_ok = FALSE;
	bool month_ok = FALSE;
	bool need_move = FALSE;
	bool no_month = TRUE;
	bool no_time = TRUE;

	if (MOB_FLAGGED(ch, MOB_LIKE_DAY))
	{
		if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
			time_ok = TRUE;
		need_move = TRUE;
		no_time = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_NIGHT))
	{
		if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
			time_ok = TRUE;
		need_move = TRUE;
		no_time = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_FULLMOON))
	{
		if ((weather_info.sunlight == SUN_SET ||
				weather_info.sunlight == SUN_DARK) &&
				(weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
			time_ok = TRUE;
		need_move = TRUE;
		no_time = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_WINTER))
	{
		if (weather_info.season == SEASON_WINTER)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_SPRING))
	{
		if (weather_info.season == SEASON_SPRING)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_SUMMER))
	{
		if (weather_info.season == SEASON_SUMMER)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (MOB_FLAGGED(ch, MOB_LIKE_AUTUMN))
	{
		if (weather_info.season == SEASON_AUTUMN)
			month_ok = TRUE;
		need_move = TRUE;
		no_month = FALSE;
	}
	if (need_move)
	{
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok)
		{
			if (world[room]->number != zone_table[world[room]->zone].top)
				return;
			if (GET_LASTROOM(ch) == NOWHERE)
			{
				extract_mob(ch);
				return;
			}
			char_from_room(ch);
			char_to_room(ch, real_room(GET_LASTROOM(ch)));
		}
		else
		{
			if (world[room]->number == zone_table[world[room]->zone].top)
				return;
			GET_LASTROOM(ch) = GET_ROOM_VNUM(room);
			char_from_room(ch);
			room = real_room(zone_table[world[room]->zone].top);
			if (room == NOWHERE)
				room = real_room(GET_LASTROOM(ch));
			char_to_room(ch, room);
		}
	}
}

void paste_obj(OBJ_DATA *obj, room_rnum room)
{
	if (obj->get_carried_by()
		|| obj->get_worn_by()
		|| room == NOWHERE)
	{
		return;
	}

	bool time_ok = FALSE;
	bool month_ok = FALSE;
	bool need_move = FALSE;
	bool no_time = TRUE;
	bool no_month = TRUE;

	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_DAY))
	{
		if (weather_info.sunlight == SUN_RISE
			|| weather_info.sunlight == SUN_LIGHT)
		{
			time_ok = TRUE;
		}
		need_move = TRUE;
		no_time = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NIGHT))
	{
		if (weather_info.sunlight == SUN_SET
			|| weather_info.sunlight == SUN_DARK)
		{
			time_ok = TRUE;
		}
		need_move = TRUE;
		no_time = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_FULLMOON))
	{
		if ((weather_info.sunlight == SUN_SET
				|| weather_info.sunlight == SUN_DARK)
			&& weather_info.moon_day >= 12
			&& weather_info.moon_day <= 15)
		{
			time_ok = TRUE;
		}
		need_move = TRUE;
		no_time = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_WINTER))
	{
		if (weather_info.season == SEASON_WINTER)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SPRING))
	{
		if (weather_info.season == SEASON_SPRING)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_SUMMER))
	{
		if (weather_info.season == SEASON_SUMMER)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_AUTUMN))
	{
		if (weather_info.season == SEASON_AUTUMN)
		{
			month_ok = TRUE;
		}
		need_move = TRUE;
		no_month = FALSE;
	}
	if (need_move)
	{
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok)
		{
			if (world[room]->number != zone_table[world[room]->zone].top)
			{
				return;
			}
			if (OBJ_GET_LASTROOM(obj) == NOWHERE)
			{
				extract_obj(obj);
				return;
			}
			obj_from_room(obj);
			obj_to_room(obj, real_room(OBJ_GET_LASTROOM(obj)));
		}
		else
		{
			if (world[room]->number == zone_table[world[room]->zone].top)
			{
				return;
			}
			obj->set_room_was_in(GET_ROOM_VNUM(room));
			obj_from_room(obj);
			room = real_room(zone_table[world[room]->zone].top);
			if (room == NOWHERE)
			{
				room = real_room(OBJ_GET_LASTROOM(obj));
			}
			obj_to_room(obj, room);
		}
	}
}

void paste_mobiles()
{
	CHAR_DATA *ch_next;
	for (CHAR_DATA *ch = character_list; ch; ch = ch_next)
	{
		ch_next = ch->get_next();
		paste_mob(ch, ch->in_room);
	}

	OBJ_DATA *obj_next;
	for (OBJ_DATA *obj = object_list; obj; obj = obj_next)
	{
		obj_next = obj->get_next();
		paste_obj(obj, obj->get_in_room());
	}
}

void paste_on_reset(ROOM_DATA *to_room)
{
	CHAR_DATA *ch_next;
	for (CHAR_DATA *ch = to_room->people; ch != NULL; ch = ch_next)
	{
		ch_next = ch->next_in_room;
		paste_mob(ch, ch->in_room);
	}

	OBJ_DATA *obj_next;
	for (OBJ_DATA *obj = to_room->contents; obj; obj = obj_next)
	{
		obj_next = obj->get_next_content();
		paste_obj(obj, obj->get_in_room());
	}
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
	char buf[256];

	sprintf(buf, "SYSERR: zone file %d.zon: %s", zone_table[zone].number, message);
	mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);

	sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
			ZCMD.command, zone_table[zone].number, ZCMD.line);
	mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
}

void process_load_celebrate(Celebrates::CelebrateDataPtr celebrate, int vnum)
{
	Celebrates::CelebrateRoomsList::iterator room;
	Celebrates::LoadList::iterator load, load_in;

	log("Processing celebrate %s load section for zone %d", celebrate->name.c_str(), vnum);

	if (celebrate->rooms.find(vnum) != celebrate->rooms.end())
	{
		for (room = celebrate->rooms[vnum].begin();room != celebrate->rooms[vnum].end(); ++room)
		{
			room_rnum rn = real_room((*room)->vnum);
			if ( rn != NOWHERE)
			{
				if (!(world[rn]->script))
					CREATE(world[rn]->script, 1);

				for (Celebrates::TrigList::iterator it = (*room)->triggers.begin();
						it != (*room)->triggers.end(); ++it)
					add_trigger(world[rn]->script,read_trigger(real_trigger(*it)), -1);
			}

			for (load = (*room)->mobs.begin(); load != (*room)->mobs.end(); ++load)
			{
				CHAR_DATA* mob;
				OBJ_DATA* obj;
				int i = real_mobile((*load)->vnum);
				if (i > 0 && mob_index[i].number < (*load)->max)
				{
					mob = read_mobile(i, REAL);
					if (mob)
					{
						if (!SCRIPT(mob))
							CREATE(SCRIPT(mob), 1);
						for (Celebrates::TrigList::iterator it = (*load)->triggers.begin();
							it != (*load)->triggers.end(); ++it)
							add_trigger(SCRIPT(mob), read_trigger(real_trigger(*it)), -1);
						load_mtrigger(mob);
						char_to_room(mob, real_room((*room)->vnum));
						Celebrates::add_mob_to_load_list(mob->id, mob);
						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in)
						{
							obj_rnum rnum = real_object((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
							{
								obj = read_object(real_object((*load_in)->vnum), REAL);
								if (obj)
			 					{
									obj_to_char(obj, mob);
									obj->set_zone(world[IN_ROOM(mob)]->zone);

									if (!obj->get_script())
									{
										obj->set_script(new SCRIPT_DATA());
									}
									for (Celebrates::TrigList::iterator it = (*load_in)->triggers.begin();
											it != (*load_in)->triggers.end(); ++it)
									{
										add_trigger(obj->get_script().get(), read_trigger(real_trigger(*it)), -1);
									}

									load_otrigger(obj);
									Celebrates::add_obj_to_load_list(obj->get_uid(), obj);
								}
								else
								{
									log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load_in)->vnum);
								}
							}
						}
					}
					else
					{
						log("{Error] Processing celebrate %s while loading mob %d", celebrate->name.c_str(), (*load)->vnum);
					}
				}
			}
			for (load = (*room)->objects.begin(); load != (*room)->objects.end(); ++load)
			{
				OBJ_DATA *obj, *obj_in, *obj_room;
				obj_rnum rnum = real_object((*load)->vnum);
				if (rnum == -1)
				{
					log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load)->vnum);
					return;
				}
				int obj_in_room = 0;

				for (obj_room = world[rn]->contents; obj_room; obj_room = obj_room->get_next_content())
				{
					if (rnum == GET_OBJ_RNUM(obj_room))
					{
						obj_in_room++;
					}
				}

				if ((obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
					&& (obj_in_room < (*load)->max))
				{
					obj = read_object(real_object((*load)->vnum), REAL);
					if (obj)
					{
						if (!obj->get_script())
						{
							obj->set_script(new SCRIPT_DATA());
						}
						for (Celebrates::TrigList::iterator it = (*load)->triggers.begin();
							it != (*load)->triggers.end(); ++it)
						{
							add_trigger(obj->get_script().get(), read_trigger(real_trigger(*it)), -1);
						}
						load_otrigger(obj);
						Celebrates::add_obj_to_load_list(obj->get_uid(), obj);

						obj_to_room(obj, real_room((*room)->vnum));

						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in)
						{
							obj_rnum rnum = real_object((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
							{
								obj_in = read_object(real_object((*load_in)->vnum), REAL);
								if (obj_in
									&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER)
			 					{
									obj_to_obj(obj_in, obj);
									obj_in->set_zone(GET_OBJ_ZONE(obj));

									if (!obj_in->get_script())
									{
										obj_in->set_script(new SCRIPT_DATA());
									}
									for (Celebrates::TrigList::iterator it = (*load_in)->triggers.begin();
											it != (*load_in)->triggers.end(); ++it)
									{
										add_trigger(obj_in->get_script().get(), read_trigger(real_trigger(*it)), -1);
									}

									load_otrigger(obj_in);
									Celebrates::add_obj_to_load_list(obj->get_uid(), obj);
								}
								else
								{
									log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load_in)->vnum);
								}
							}
						}
					}
					else
					{
						log("{Error] Processing celebrate %s while loading mob %d", celebrate->name.c_str(), (*load)->vnum);
					}
				}
			}
		}
	}
}

void process_attach_celebrate(Celebrates::CelebrateDataPtr celebrate, int zone_vnum)
{
	log("Processing celebrate %s attach section for zone %d", celebrate->name.c_str(), zone_vnum);

	if (celebrate->mobsToAttach.find(zone_vnum) != celebrate->mobsToAttach.end())
	{
		//поскольку единственным доступным способом получить всех мобов одного внума является
		//обход всего списка мобов в мире, то будем хотя бы 1 раз его обходить
		Celebrates::AttachList list = celebrate->mobsToAttach[zone_vnum];
		for (CHAR_DATA *ch = character_list; ch; ch=ch->get_next())
		{
			if (ch->nr > 0 && list.find(mob_index[ch->nr].vnum) != list.end())
			{
				if (!SCRIPT(ch))
					CREATE(SCRIPT(ch), 1);
				for (Celebrates::TrigList::iterator it = list[mob_index[ch->nr].vnum].begin();
						it != list[mob_index[ch->nr].vnum].end(); ++it)
					add_trigger(SCRIPT(ch), read_trigger(real_trigger(*it)), -1);
				Celebrates::add_mob_to_attach_list(ch->id, ch);
			}
		}
	}

	if (celebrate->objsToAttach.find(zone_vnum) != celebrate->objsToAttach.end())
	{
		Celebrates::AttachList list = celebrate->objsToAttach[zone_vnum];
		for (OBJ_DATA *o = object_list; o; o = o->get_next())
		{
			if (o->get_rnum() > 0 && list.find(o->get_rnum()) != list.end())
			{
				if (!o->get_script())
				{
					o->set_script(new SCRIPT_DATA());
				}

				for (Celebrates::TrigList::iterator it = list[o->get_rnum()].begin(); it != list[o->get_rnum()].end(); ++it)
				{
					add_trigger(o->get_script().get(), read_trigger(real_trigger(*it)), -1);
				}

				Celebrates::add_obj_to_attach_list(o->get_uid(), o);
			}
		}
	}
}

void process_celebrates(int vnum)
{
	Celebrates::CelebrateDataPtr mono = Celebrates::get_mono_celebrate();
	Celebrates::CelebrateDataPtr poly = Celebrates::get_poly_celebrate();
	Celebrates::CelebrateDataPtr real = Celebrates::get_real_celebrate();

	if (mono)
	{
		process_load_celebrate(mono, vnum);
		process_attach_celebrate(mono, vnum);
	}

	if (poly)
	{
		process_load_celebrate(poly, vnum);
		process_attach_celebrate(poly, vnum);
	}
	if (real)
	{
		process_load_celebrate(real, vnum);
		process_attach_celebrate(real, vnum);
	}
}

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); }

// Выполить команду, только если предыдущая успешна
#define		CHECK_SUCCESS		1
// Команда не должна изменить флаг
#define		FLAG_PERSIST		2

// execute the reset command table of a given zone
void reset_zone(zone_rnum zone)
{
	int cmd_no;
	int cmd_tmp, obj_in_room_max, obj_in_room = 0;
	CHAR_DATA *mob = NULL, *leader = NULL, *ch;
	OBJ_DATA *obj, *obj_to, *obj_room;
	int rnum_start, rnum_stop;
	CHAR_DATA *tmob = NULL;	// for trigger assignment
	OBJ_DATA *tobj = NULL;	// for trigger assignment

	int last_state, curr_state;	// статус завершения последней и текущей команды

	log("[Reset] Start zone %s", zone_table[zone].name);
	repop_decay(zone);	// рассыпание обьектов ITEM_REPOP_DECAY

	//----------------------------------------------------------------------------
	last_state = 1;		// для первой команды считаем, что все ок

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	{
		if (ZCMD.command == '*')
		{
			// комментарий - ни на что не влияет
			continue;
		}

		curr_state = 0;	// по умолчанию - неудачно
		if (!(ZCMD.if_flag & CHECK_SUCCESS) || last_state)
		{
			// Выполняем команду, если предыдущая успешна или не нужно проверять
			switch (ZCMD.command)
			{
			case 'M':
				// read a mobile
				// 'M' <flag> <mob_vnum> <max_in_world> <room_vnum> <max_in_room|-1>
				mob = NULL;	//Добавлено Ладником
				if (mob_index[ZCMD.arg1].number < ZCMD.arg2 &&
						(ZCMD.arg4 < 0 || mobs_in_room(ZCMD.arg1, ZCMD.arg3) < ZCMD.arg4))
				{
					mob = read_mobile(ZCMD.arg1, REAL);
					char_to_room(mob, ZCMD.arg3);
					load_mtrigger(mob);
					tmob = mob;
					curr_state = 1;
				}
				tobj = NULL;
				break;

			case 'F':
				// Follow mobiles
				// 'F' <flag> <room_vnum> <leader_vnum> <mob_vnum>
				leader = NULL;
				if (ZCMD.arg1 >= FIRST_ROOM && ZCMD.arg1 <= top_of_world)
				{
					for (ch = world[ZCMD.arg1]->people; ch && !leader; ch = ch->next_in_room)
						if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg2)
							leader = ch;
					for (ch = world[ZCMD.arg1]->people; ch && leader; ch = ch->next_in_room)
						if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg3
								&& leader != ch && ch->master != leader)
						{
							if (ch->master)
								stop_follower(ch, SF_EMPTY);
							add_follower(ch, leader);
							curr_state = 1;
						}
				}
				break;

			case 'Q':
				// delete all mobiles
				// 'Q' <flag> <mob_vnum>
				for (ch = character_list; ch; ch = leader)
				{
					leader = ch->get_next();
					// Карачун. Поднятые мобы не должны уничтожаться.
					if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg1 && !MOB_FLAGGED(ch, MOB_RESURRECTED))
					{
						// Карачун. Мобы должны оставлять стафф.
						// Тока чужой стаф, а не свой же при резете зоны. -- Krodo
						extract_char(ch, FALSE, 1);
						//extract_mob(ch);
						curr_state = 1;
					}
				}
				tobj = NULL;
				tmob = NULL;
				break;

			case 'O':
				// read an object
				// 'O' <flag> <obj_vnum> <max_in_world> <room_vnum|-1> <load%|-1>
				// Проверка  - сколько всего таких же обьектов надо на эту клетку
				for (cmd_tmp = 0, obj_in_room_max = 0; ZCMD_CMD(cmd_tmp).command != 'S'; cmd_tmp++)
					if ((ZCMD_CMD(cmd_tmp).command == 'O')
							&& (ZCMD.arg1 == ZCMD_CMD(cmd_tmp).arg1)
							&& (ZCMD.arg3 == ZCMD_CMD(cmd_tmp).arg3))
						obj_in_room_max++;
				// Теперь считаем склько их на текущей клетке
				if (ZCMD.arg3 >= 0)
				{
					for (obj_room = world[ZCMD.arg3]->contents, obj_in_room = 0; obj_room; obj_room = obj_room->get_next_content())
					{
						if (ZCMD.arg1 == GET_OBJ_RNUM(obj_room))
						{
							obj_in_room++;
						}
					}
				}
				// Теперь грузим обьект если надо
				if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4)
					&& (obj_in_room < obj_in_room_max))
				{
					obj = read_object(ZCMD.arg1, REAL);
					if (ZCMD.arg3 >= 0)
					{
						obj->set_zone(world[ZCMD.arg3]->zone);
						if (!obj_to_room(obj, ZCMD.arg3))
						{
							extract_obj(obj);
							break;
						}
						load_otrigger(obj);
					}
					else
					{
						obj->set_in_room(NOWHERE);
					}
					tobj = obj;
					curr_state = 1;
					if (!OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODECAY))
					{
						sprintf(buf, "&YВНИМАНИЕ&G На землю загружен объект без флага NODECAY : %s (VNUM=%d)",
							GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_VNUM(obj));
						mudlog(buf, BRF, LVL_BUILDER, ERRLOG, TRUE);
					}
				}
				tmob = NULL;
				break;

			case 'P':
				// object to object
				// 'P' <flag> <obj_vnum> <max_in_world> <target_vnum> <load%|-1>
				if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4))
				{
					if (!(obj_to = get_obj_num(ZCMD.arg3)))
					{
						ZONE_ERROR("target obj not found, command omited");
//                 ZCMD.command = '*';
						break;
					}
					if (GET_OBJ_TYPE(obj_to) != OBJ_DATA::ITEM_CONTAINER)
					{
						ZONE_ERROR("attempt put obj to non container, omited");
						ZCMD.command = '*';
						break;
					}
					obj = read_object(ZCMD.arg1, REAL);
					if (obj_to->get_in_room() != NOWHERE)
					{
						obj->set_zone(world[obj_to->get_in_room()]->zone);
					}
					else if (obj_to->get_worn_by())
					{
						obj->set_zone(world[IN_ROOM(obj_to->get_worn_by())]->zone);
					}
					else if (obj_to->get_carried_by())
					{
						obj->set_zone(world[IN_ROOM(obj_to->get_carried_by())]->zone);
					}
					obj_to_obj(obj, obj_to);
					load_otrigger(obj);
					tobj = obj;
					curr_state = 1;
				}
				tmob = NULL;
				break;

			case 'G':
				// obj_to_char
				// 'G' <flag> <obj_vnum> <max_in_world> <-> <load%|-1>
				if (!mob)
					//Изменено Ладником
				{
					// ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
					// ZCMD.command = '*';
					break;
				}
				if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4))
				{
					obj = read_object(ZCMD.arg1, REAL);
					obj_to_char(obj, mob);
					obj->set_zone(world[IN_ROOM(mob)]->zone);
					tobj = obj;
					load_otrigger(obj);
					curr_state = 1;
				}
				tmob = NULL;
				break;

			case 'E':
				// object to equipment list
				// 'E' <flag> <obj_vnum> <max_in_world> <wear_pos> <load%|-1>
				//Изменено Ладником
				if (!mob)
				{
					//ZONE_ERROR("trying to equip non-existant mob, command disabled");
					// ZCMD.command = '*';
					break;
				}
				if ((obj_proto.actual_count(ZCMD.arg1) < obj_proto[ZCMD.arg1]->get_max_in_world()
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == OBJ_DATA::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
					&& (ZCMD.arg4 <= 0
						|| number(1, 100) <= ZCMD.arg4))
				{
					if (ZCMD.arg3 < 0
						|| ZCMD.arg3 >= NUM_WEARS)
					{
						ZONE_ERROR("invalid equipment pos number");
					}
					else
					{
						obj = read_object(ZCMD.arg1, REAL);
						obj->set_zone(world[IN_ROOM(mob)]->zone);
						obj->set_in_room(IN_ROOM(mob));
						load_otrigger(obj);
						if (wear_otrigger(obj, mob, ZCMD.arg3))
						{
							obj->set_in_room(NOWHERE);
							equip_char(mob, obj, ZCMD.arg3);
						}
						else
						{
							obj_to_char(obj, mob);
						}
						if (!(obj->get_carried_by() == mob)
							&& !(obj->get_worn_by() == mob))
						{
							extract_obj(obj);
							tobj = NULL;
						}
						else
						{
							tobj = obj;
							curr_state = 1;
						}
					}
				}
				tmob = NULL;
				break;

			case 'R':
				// rem obj from room
				// 'R' <flag> <room_vnum> <obj_vnum>
				if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1]->contents)) != NULL)
				{
					obj_from_room(obj);
					extract_obj(obj);
					curr_state = 1;
				}
				tmob = NULL;
				tobj = NULL;
				break;

			case 'D':
				// set state of door
				// 'D' <flag> <room_vnum> <door_pos> <door_state>
				if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
						(world[ZCMD.arg1]->dir_option[ZCMD.arg2] == NULL))
				{
					ZONE_ERROR("door does not exist, command disabled");
					ZCMD.command = '*';
				}
				else
				{
					REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_BROKEN);
					switch (ZCMD.arg3)
					{
					case 0:
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_LOCKED);
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_CLOSED);
						break;
					case 1:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_LOCKED);
						break;
					case 2:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
						break;
					case 3:
						SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
						break;
					case 4:
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
								   exit_info, EX_HIDDEN);
						break;
					}
					curr_state = 1;
				}
				tmob = NULL;
				tobj = NULL;
				break;

			case 'T':
				// trigger command; details to be filled in later
				// 'T' <flag> <trigger_type> <trigger_vnum> <room_vnum, для WLD_TRIGGER>
				if (ZCMD.arg1 == MOB_TRIGGER && tmob)
				{
					if (!SCRIPT(tmob))
						CREATE(SCRIPT(tmob), 1);
					add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
					curr_state = 1;
				}
				else if (ZCMD.arg1 == OBJ_TRIGGER && tobj)
				{
					if (!tobj->get_script())
					{
						tobj->set_script(new SCRIPT_DATA());
					}
					add_trigger(tobj->get_script().get(), read_trigger(real_trigger(ZCMD.arg2)), -1);
					curr_state = 1;
				}
				else if (ZCMD.arg1 == WLD_TRIGGER)
				{
					if (ZCMD.arg3 != NOWHERE)
					{
						if (!(world[ZCMD.arg3]->script))
						{
							CREATE(world[ZCMD.arg3]->script, 1);
						}
						add_trigger(world[ZCMD.arg3]->script,
									read_trigger(real_trigger(ZCMD.arg2)), -1);
						curr_state = 1;
					}
				}
				break;

			case 'V':
				// 'V' <flag> <trigger_type> <room_vnum> <context> <var_name> <var_value>
				if (ZCMD.arg1 == MOB_TRIGGER && tmob)
				{
					if (!SCRIPT(tmob))
					{
						ZONE_ERROR("Attempt to give variable to scriptless mobile");
					}
					else
					{
						add_var_cntx(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
						curr_state = 1;
					}
				}
				else if (ZCMD.arg1 == OBJ_TRIGGER && tobj)
				{
					if (!tobj->get_script())
					{
						ZONE_ERROR("Attempt to give variable to scriptless object");
					}
					else
					{
						add_var_cntx(&(tobj->get_script()->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
						curr_state = 1;
					}
				}
				else if (ZCMD.arg1 == WLD_TRIGGER)
				{
					if (ZCMD.arg2 < FIRST_ROOM || ZCMD.arg2 > top_of_world)
					{
						ZONE_ERROR("Invalid room number in variable assignment");
					}
					else
					{
						if (!(world[ZCMD.arg2]->script))
						{
							ZONE_ERROR("Attempt to give variable to scriptless object");
						}
						else
						{
							add_var_cntx(&(world[ZCMD.arg2]->script->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
							curr_state = 1;
						}
					}
				}
				break;

			default:
				ZONE_ERROR("unknown cmd in reset table; cmd disabled");
				ZCMD.command = '*';
				break;
			}
		}
		// if ( (ZCMD.if_flag&CHECK_SUCCESS) && !last_state )
		if (!(ZCMD.if_flag & FLAG_PERSIST))
		{
			// команда изменяет флаг
			last_state = curr_state;
		}

	}
	if (zone_table[zone].used)
	{
		zone_table[zone].count_reset++;
	}
	zone_table[zone].age = 0;
	zone_table[zone].used = FALSE;

	if (get_zone_rooms(zone, &rnum_start, &rnum_stop))
	{
		ROOM_DATA* room;
		ROOM_DATA* gate_room;
		// все внутренние резеты комнат зоны теперь идут за один цикл
		// резет порталов теперь тут же и переписан, чтобы не гонять по всем румам, ибо жрал половину времени резета -- Krodo
		for (int rnum = rnum_start; rnum <= rnum_stop; rnum++)
		{
			room = world[rnum];
			reset_wtrigger(room);
			im_reset_room(room, zone_table[zone].level, zone_table[zone].type);
			gate_room = OneWayPortal::get_from_room(room);
			if (gate_room)   // случай врат
			{
				gate_room->portal_time = 0;
				OneWayPortal::remove(room);
			}
			else if (room->portal_time > 0)   // случай двусторонней пенты
			{
				world[room->portal_room]->portal_time = 0;
				room->portal_time = 0;
			}
			paste_on_reset(room);
		}
	}

	process_celebrates(zone_table[zone].number);

	for (rnum_start = 0; rnum_start <= top_of_zone_table; rnum_start++)
	{
		// проверяем, не содержится ли текущая зона в чьем-либо typeB_list
		for (curr_state = zone_table[rnum_start].typeB_count; curr_state > 0; curr_state--)
		{
			if (zone_table[rnum_start].typeB_list[curr_state - 1] == zone_table[zone].number)
			{
				zone_table[rnum_start].typeB_flag[curr_state - 1] = TRUE;
//				log("[Reset] Adding TRUE for zone %d in the array contained by zone %d",
//				    zone_table[zone].number, zone_table[rnum_start].number);
				break;
			}
		}
	}
	//Если это ведущая зона, то при ее сбросе обнуляем typeB_flag
	for (rnum_start = zone_table[zone].typeB_count; rnum_start > 0; rnum_start--)
		zone_table[zone].typeB_flag[rnum_start - 1] = FALSE;
	log("[Reset] Stop zone %s", zone_table[zone].name);
	after_reset_zone(zone);
}

// Ищет RNUM первой и последней комнаты зоны
// Еси возвращает 0 - комнат в зоне нету
int get_zone_rooms(int zone_nr, int *start, int *stop)
{
	int first_room_vnum, rnum;
	first_room_vnum = zone_table[zone_nr].top;
	rnum = real_room(first_room_vnum);
	if (rnum == NOWHERE)
		return 0;
	*stop = rnum;
	rnum = NOWHERE;
	while (zone_nr)
	{
		first_room_vnum = zone_table[--zone_nr].top;
		rnum = real_room(first_room_vnum);
		if (rnum != NOWHERE)
		{
			++rnum;
			break;
		}
	}
	if (rnum == NOWHERE)
		rnum = 1;	// самая первая зона начинается с 1
	*start = rnum;
	return 1;
}


// for use in reset_zone; return TRUE if zone 'nr' is free of PC's
int is_empty(zone_rnum zone_nr)
{
	DESCRIPTOR_DATA *i;
	int rnum_start, rnum_stop;
	CHAR_DATA *c;
	//char *buf_tmp;
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == NOWHERE)
			continue;
		if (GET_LEVEL(i->character) >= LVL_IMMORT)
			continue;
		if (world[i->character->in_room]->zone != zone_nr)
			continue;
		return (0);
	}

	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!get_zone_rooms(zone_nr, &rnum_start, &rnum_stop))
		return 1;	// в зоне нет комнат :)

	for (; rnum_start <= rnum_stop; rnum_start++)
	{
// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		for (c = world[rnum_start]->people; c; c = c->next_in_room)
			if (!IS_NPC(c) && (GET_LEVEL(c) < LVL_IMMORT))
			{
				return 0;
			}
	}

// теперь проверю всех товарищей в void комнате STRANGE_ROOM
	for (c = world[STRANGE_ROOM]->people; c; c = c->next_in_room)
	{
		int was = c->get_was_in_room();
		if (was == NOWHERE)
			continue;
		if (GET_LEVEL(c) >= LVL_IMMORT)
			continue;
		if (world[was]->zone != zone_nr)
			continue;
		return 0;
	}

//Проверим, нет ли в зоне метки для врат, чтоб не абузили.
    for (auto it = RoomSpells::aff_room_list.begin(); it != RoomSpells::aff_room_list.end(); ++it)
	{
		if ((*it)->zone == zone_nr
			&& find_room_affect(*it, SPELL_RUNE_LABEL) != (*it)->affected.end())
		{
			// если в зоне метка
			return 0;
		}
	}

	return 1;
}

int mobs_in_room(int m_num, int r_num)
{
	CHAR_DATA *ch;
	int count = 0;

	for (ch = world[r_num]->people; ch; ch = ch->next_in_room)
		if (m_num == GET_MOB_RNUM(ch) && !MOB_FLAGGED(ch, MOB_RESURRECTED))
			count++;

	return count;
}


/*************************************************************************
*  stuff related to the save/load player system                          *
*************************************************************************/

long cmp_ptable_by_name(char *name, int len)
{
	int i;

	len = MIN(len, static_cast<int>(strlen(name)));
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): I am not sure but I guess that linear search is not the best solution here. TODO: make map helper (MAPHELPER). */
	for (i = 0; i <= top_of_p_table; i++)
	{
		const char* pname = player_table[i].name;
		if (!strn_cmp(pname, arg, MIN(len, static_cast<int>(strlen(pname)))))
		{
			return i;
		}
	}
	return -1;
}



long get_ptable_by_name(const char *name)
{
	int i;

	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (i = 0; i <= top_of_p_table; i++)
	{
		const char* pname = player_table[i].name;
		if (!str_cmp(pname, arg))
		{
			return (i);
		}
	}
	sprintf(buf, "Char %s(%s) not found !!!", name, arg);
	mudlog(buf, LGH, LVL_IMMORT, SYSLOG, FALSE);
	return (-1);
}

long get_ptable_by_unique(long unique)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == unique)
		{
			return i;
		}
	}
	return 0;
}


long get_id_by_name(char *name)
{
	int i;

	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (!str_cmp(player_table[i].name, arg))
		{
			return (player_table[i].id);
		}
	}

	return (-1);
}

long get_id_by_uid(long uid)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == uid)
		{
			return player_table[i].id;
		}
	}
	return -1;
}

int get_uid_by_id(int id)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].id == id)
		{
			return player_table[i].unique;
		}
	}
	return -1;
}

const char *get_name_by_id(long id)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].id == id)
		{
			return player_table[i].name;
		}
	}
	return "";
}

char* get_name_by_unique(int unique)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == unique)
		{
			return player_table[i].name;
		}
	}
	return 0;
}

int get_level_by_unique(long unique)
{
	int level = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; ++i)
	{
		if (player_table[i].unique == unique)
		{
			level = player_table[i].level;
		}
	}
	return level;
}

long get_lastlogon_by_unique(long unique)
{
	long time = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; ++i)
	{
		if (player_table[i].unique == unique)
		{
			time = player_table[i].last_logon;
		}
	}
	return time;
}

int correct_unique(int unique)
{
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (int i = 0; i <= top_of_p_table; i++)
	{
		if (player_table[i].unique == unique)
		{
			return TRUE;
		}
	}

	return FALSE;
}

int create_unique(void)
{
	int unique;

	do
	{
		unique = (number(0, 64) << 24) + (number(0, 255) << 16) + (number(0, 255) << 8) + (number(0, 255));
	}
	while (correct_unique(unique));
	return (unique);
}

// shapirus
struct ignore_data *parse_ignore(char *buf)
{
	struct ignore_data *result;

	CREATE(result, 1);

	if (sscanf(buf, "[%ld]%ld", &result->mode, &result->id) < 2)
	{
		free(result);
		return NULL;
	}
	else
	{
		result->next = NULL;
		return result;
	}
}

// возвращает истину, если чар с заданным id
// существует, ложь в противном случае
bool ign_plr_exists(long id)
{
	int i;

	if (id == -1)
		return TRUE;

	for (i = 0; i <= top_of_p_table; i++)
		if (id == player_table[i].id)
			return TRUE;
	return FALSE;
}

// можно вызывать много раз по разу, а можно один раз
// вызывать и скормить в одной строке всех
void load_ignores(CHAR_DATA * ch, char *line)
{
	struct ignore_data *cur_ign, *ignore_list;
	char *buf;
	unsigned int i, k, done = 0;

// найдем последний элемент списка на случай, если функцию
// хотят вызывать многократно
	for (ignore_list = IGNORE_LIST(ch); ignore_list && ignore_list->next; ignore_list = ignore_list->next);
	buf = str_dup(line);
	for (i = k = 0;;)
	{
		if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == 0)
		{
			if (buf[i] == 0)
				done = 1;
			buf[i] = 0;
			cur_ign = parse_ignore(&(buf[k]));
			if (cur_ign)
			{
				if (!ignore_list)
				{
					IGNORE_LIST(ch) = cur_ign;
				}
				else
				{
					ignore_list->next = cur_ign;
				}
				ignore_list = cur_ign;
				// удаленных игроков из листа нафиг
				if (!ign_plr_exists(ignore_list->id))
					ignore_list->id = 0;
			}
			else
			{
				log("WARNING: could not parse ignore list " "of %s: invalid format", GET_NAME(ch));
				return;
			}
			// skip whitespace
			for (k = i + 1; buf[k] == ' ' || buf[k] == '\t'; k++);
			i = k;
			if (done || buf[k] == 0)
				break;
		}
		else
		{
			i++;
		}
	}
	free(buf);
}

/**
* Можно канеш просто в get_skill иммам возвращать 100 или там 200, но тут зато можно
* потестить че-нить с возможностью покачать скилл во время игры иммом.
*/
void set_god_skills(CHAR_DATA *ch)
{
	for (const auto i : AVAILABLE_SKILLS)
	{
		ch->set_skill(i, 150);
	}
}

#define NUM_OF_SAVE_THROWS	5

// по умолчанию reboot = 0 (пользуется только при ребуте)
int load_char(const char *name, CHAR_DATA * char_element, bool reboot)
{
	int player_i;

	player_i = char_element->load_char_ascii(name, reboot);
	if (player_i > -1)
	{
		char_element->set_pfilepos(player_i);
	}
	return (player_i);
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(const char *name)
{
	int i, pos;

	if (top_of_p_table == -1)  	// no table
	{
		CREATE(player_table, 1);
		pos = top_of_p_table = 0;
	}
	else if ((pos = get_ptable_by_name(name)) == -1)  	// new name
	{
		i = ++top_of_p_table + 1;
		RECREATE(player_table, i);
		pos = top_of_p_table;
	}

	CREATE(player_table[pos].name, strlen(name) + 1);

	// copy lowercase equivalent of name to table field
	for (i = 0, player_table[pos].name[i] = '\0'; (player_table[pos].name[i] = LOWER(name[i])); i++);
	// create new save activity
	player_table[pos].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
	player_table[pos].timer = NULL;
	player_table[pos].unique = -1;

	return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature                     *
************************************************************************/

// read and allocate space for a '~'-terminated string from a given file
char *fread_string(FILE* fl, char* /*error*/)
{
	char buffer[1 + MAX_STRING_LENGTH];
	char* to = buffer;

	bool done = false;
	int remained = MAX_STRING_LENGTH;
	const char* end = buffer + MAX_STRING_LENGTH;
	while (!done
		&& fgets(to, remained, fl))
	{
		const char* chunk_beginning = to;
		const char* from = to;
		while (end != from)
		{
			if ('~' == from[0])
			{
				if (1 + from != end
					&& '~' == from[1])
				{
					++from;	//	skip escaped '~'
				}
				else
				{
					done = true;
					break;
				}
			}

			const char c = *(from++);
			if ('\0' == c)
			{
				break;
			}
			*(to++) = c;
		}

		remained -= to - chunk_beginning;
	}
	*to = '\0';

	return strdup(buffer);
}

// release memory allocated for an obj struct
void free_obj(OBJ_DATA * obj)
{
	// delete obj;
	obj->purge();
}

/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int file_to_string_alloc(const char *name, char **buf)
{
	char temp[MAX_EXTEND_LENGTH];
	DESCRIPTOR_DATA *in_use;

	for (in_use = descriptor_list; in_use; in_use = in_use->next)
		if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
			return (-1);

	// Lets not free() what used to be there unless we succeeded.
	if (file_to_string(name, temp) < 0)
		return (-1);

	if (*buf)
		free(*buf);

	*buf = str_dup(temp);
	return (0);
}


// read contents of a text file, and place in buf
int file_to_string(const char *name, char *buf)
{
	FILE *fl;
	char tmp[READ_SIZE + 3];

	*buf = '\0';

	if (!(fl = fopen(name, "r")))
	{
		log("SYSERR: reading %s: %s", name, strerror(errno));
		sprintf(buf+strlen(buf), "Error: file '%s' is empty.\r\n", name);
		return (0);
	}
	do
	{
		const char* dummy = fgets(tmp, READ_SIZE, fl);
		UNUSED_ARG(dummy);

		tmp[strlen(tmp) - 1] = '\0';	// take off the trailing \n
		strcat(tmp, "\r\n");

		if (!feof(fl))
		{
			if (strlen(buf) + strlen(tmp) + 1 > MAX_EXTEND_LENGTH)
			{
				log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
				*buf = '\0';
				return (-1);
			}
			strcat(buf, tmp);
		}
	}
	while (!feof(fl));

	fclose(fl);

	return (0);
}

void clear_char_skills(CHAR_DATA * ch)
{
	int i;
	ch->real_abils.Feats.reset();
	for (i = 0; i < MAX_SPELLS + 1; i++)
		ch->real_abils.SplKnw[i] = 0;
	for (i = 0; i < MAX_SPELLS + 1; i++)
		ch->real_abils.SplMem[i] = 0;
	ch->clear_skills();
}

// initialize a new character only if class is set
void init_char(CHAR_DATA * ch)
{
	int i;

#ifdef TEST_BUILD
	if (top_of_p_table == 0)
	{
		// При собирании через make test первый чар в маде становится иммом 34
		ch->set_level(LVL_IMPL);
	}
#endif

	GET_PORTALS(ch) = NULL;
	CREATE(GET_LOGS(ch), 1 + LAST_LOG);
	ch->set_npc_name(0);
	ch->player_data.long_descr = NULL;
	ch->player_data.description = NULL;
	ch->player_data.time.birth = time(0);
	ch->player_data.time.played = 0;
	ch->player_data.time.logon = time(0);

	// make favors for sex
	if (ch->player_data.sex == ESex::SEX_MALE)
	{
		ch->player_data.weight = number(120, 180);
		ch->player_data.height = number(160, 200);
	}
	else
	{
		ch->player_data.weight = number(100, 160);
		ch->player_data.height = number(150, 180);
	}

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 82;
	ch->points.move = GET_MAX_MOVE(ch);
	ch->real_abils.armor = 100;

	if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
	{
		ch->set_idnum(++top_idnum);
		player_table[i].id = ch->get_idnum();
		ch->set_uid(create_unique());
		player_table[i].unique = ch->get_uid();
		player_table[i].level = 0;
		player_table[i].last_logon = -1;
		player_table[i].mail = NULL;//added by WorM mail
		player_table[i].last_ip = NULL;//added by WorM последний айпи
	}
	else
	{
		log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));
	}

	if (GET_LEVEL(ch) == LVL_IMPL)
	{
		set_god_skills(ch);
		set_god_morphs(ch);
	}

	for (i = 1; i <= MAX_SPELLS; i++)
	{
		if (GET_LEVEL(ch) < LVL_GRGOD)
			GET_SPELL_TYPE(ch, i) = 0;
		else
			GET_SPELL_TYPE(ch, i) = SPELL_KNOW;
	}

	ch->char_specials.saved.affected_by = clear_flags;
	for (i = 0; i < SAVING_COUNT; i++)
		GET_SAVE(ch, i) = 0;
	for (i = 0; i < MAX_NUMBER_RESISTANCE; i++)
		GET_RESIST(ch, i) = 0;

	if (GET_LEVEL(ch) == LVL_IMPL)
	{
		ch->set_str(25);
		ch->set_int(25);
		ch->set_wis(25);
		ch->set_dex(25);
		ch->set_con(25);
		ch->set_cha(25);
	}
	ch->real_abils.size = 50;

	for (i = 0; i < 3; i++)
	{
		GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : i == DRUNK ? 0 : 24);
	}
	GET_LASTIP(ch)[0] = 0;
//	GET_LOADROOM(ch) = start_room;
	PRF_FLAGS(ch).set(PRF_DISPHP);
	PRF_FLAGS(ch).set(PRF_DISPMANA);
	PRF_FLAGS(ch).set(PRF_DISPEXITS);
	PRF_FLAGS(ch).set(PRF_DISPMOVE);
	PRF_FLAGS(ch).set(PRF_DISPEXP);
	PRF_FLAGS(ch).set(PRF_DISPFIGHT);
	PRF_FLAGS(ch).unset(PRF_SUMMONABLE);
	STRING_LENGTH(ch) = 80;
	STRING_WIDTH(ch) = 30;
	NOTIFY_EXCH_PRICE(ch) = 0;

	ch->save_char();
}

const char *remort_msg =
	"  Если вы так настойчивы в желании начать все заново -\r\n" "наберите <перевоплотиться> полностью.\r\n";

void do_remort(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	int i, place_of_destination,load_room = NOWHERE;
	const char *remort_msg2 = "$n вспыхнул$g ослепительным пламенем и пропал$g!\r\n";


	if (IS_NPC(ch) || IS_IMMORTAL(ch))
	{
		send_to_char("Вам это, похоже, совсем ни к чему.\r\n", ch);
		return;
	}
	if (GET_EXP(ch) < level_exp(ch, LVL_IMMORT) - 1)
	{
		send_to_char("ЧАВО???\r\n", ch);
		return;
	}
	if (Remort::need_torc(ch) && !PRF_FLAGGED(ch, PRF_CAN_REMORT))
	{
		send_to_char(ch,
			"Вы должны подтвердить свои заслуги, пожертвовав Богам достаточное количество гривен.\r\n"
			"%s\r\n", Remort::WHERE_TO_REMORT_STR.c_str(), ch);
		return;
	}
	if (RENTABLE(ch))
	{
		send_to_char("Вы не можете перевоплотиться в связи с боевыми действиями.\r\n", ch);
		return;
	}
	if (!subcmd)
	{
		send_to_char(remort_msg, ch);
		return;
	}

    one_argument(argument, arg);
    if (!*arg)
    {
        sprintf(buf, "Укажите, где вы хотите заново начать свой путь:\r\n");
        sprintf(buf+strlen(buf), "%s", string(BirthPlace::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(ch),GET_RACE(ch)))).c_str());
        send_to_char(buf, ch);
        return;
    } else {
        // Сначала проверим по словам - может нам текстом сказали?
        place_of_destination = BirthPlace::ParseSelect(arg);
        if (place_of_destination == BIRTH_PLACE_UNDEFINED)
        {
            //Нет, значит или ерунда в аргументе, или цифирь, смотрим
            place_of_destination = PlayerRace::CheckBirthPlace(GET_KIN(ch), GET_RACE(ch), arg);
            if (!BirthPlace::CheckId(place_of_destination))
            {
                send_to_char("Багдад далече, выберите себе местечко среди родных осин.\r\n", ch);
                return;
            }
        }
    }

	log("Remort %s", GET_NAME(ch));
	ch->remort();
	act(remort_msg2, FALSE, ch, 0, 0, TO_ROOM);

	if (ch->is_morphed()) ch->reset_morph();
	ch->set_remort(ch->get_remort() + 1);
	CLR_GOD_FLAG(ch, GF_REMORT);
	ch->inc_str(1);
	ch->inc_dex(1);
	ch->inc_con(1);
	ch->inc_wis(1);
	ch->inc_int(1);
	ch->inc_cha(1);

	if (ch->get_fighting())
		stop_fighting(ch, TRUE);

	die_follower(ch);

	while (ch->helpers)
	{
		REMOVE_FROM_LIST(ch->helpers, ch->helpers, [](auto list) -> auto& { return list->next_helper; });
	}

	while (!ch->affected.empty())
	{
		ch->affect_remove(ch->affected.begin());
	}

// Снимаем весь стафф
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			obj_to_char(unequip_char(ch, i), ch);
		}
	}

	while (ch->timed)
	{
		timed_from_char(ch, ch->timed);
	}

	ch->clear_skills();

	for (i = 1; i <= MAX_SPELLS; i++)
	{
		GET_SPELL_TYPE(ch, i) = (GET_CLASS(ch) == CLASS_DRUID ? SPELL_RUNES : 0);
		GET_SPELL_MEM(ch, i) = 0;
	}

	while (ch->timed_feat)
	{
		timed_feat_from_char(ch, ch->timed_feat);
	}

	for (i = 1; i < MAX_FEATS; i++)
	{
		UNSET_FEAT(ch, i);
	}

	GET_HIT(ch) = GET_MAX_HIT(ch) = 10;
	GET_MOVE(ch) = GET_MAX_MOVE(ch) = 82;
	GET_MEM_TOTAL(ch) = GET_MEM_COMPLETED(ch) = 0;
	ch->set_level(0);
	GET_WIMP_LEV(ch) = 0;
	GET_AC(ch) = 100;
	GET_LOADROOM(ch) = calc_loadroom(ch, place_of_destination);
	PRF_FLAGS(ch).unset(PRF_SUMMONABLE);
	PRF_FLAGS(ch).unset(PRF_AWAKE);
	PRF_FLAGS(ch).unset(PRF_PUNCTUAL);
	PRF_FLAGS(ch).unset(PRF_POWERATTACK);
	PRF_FLAGS(ch).unset(PRF_GREATPOWERATTACK);
	PRF_FLAGS(ch).unset(PRF_AWAKE);
	PRF_FLAGS(ch).unset(PRF_IRON_WIND);
	// Убираем все заученные порталы
	check_portals(ch);

    //Обновляем статистику рипов для текущего перевоплощения
    GET_RIP_DTTHIS(ch) = 0;
    GET_EXP_DTTHIS(ch) = 0;
    GET_RIP_MOBTHIS(ch) = 0;
    GET_EXP_MOBTHIS(ch) = 0;
    GET_RIP_PKTHIS(ch) = 0;
    GET_EXP_PKTHIS(ch) = 0;
    GET_RIP_OTHERTHIS(ch) = 0;
    GET_EXP_OTHERTHIS(ch) = 0;

	do_start(ch, FALSE);
	ch->save_char();
	if (PLR_FLAGGED(ch, PLR_HELLED))
		load_room = r_helled_start_room;
	else if (PLR_FLAGGED(ch, PLR_NAMED))
		load_room = r_named_start_room;
	else if (PLR_FLAGGED(ch, PLR_FROZEN))
		load_room = r_frozen_start_room;
	else
	{
		if ((load_room = GET_LOADROOM(ch)) == NOWHERE)
			load_room = calc_loadroom(ch);
		load_room = real_room(load_room);
	}
	if (load_room == NOWHERE)
	{
		if (GET_LEVEL(ch) >= LVL_IMMORT)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}
	char_from_room(ch);
	char_to_room(ch, load_room);
	look_at_room(ch, 0);
	PLR_FLAGS(ch).set(PLR_NODELETE);
	remove_rune_label(ch);

	// сброс всего, связанного с гривнами (замакс сохраняем)
	PRF_FLAGS(ch).unset(PRF_CAN_REMORT);
	ch->set_ext_money(ExtMoney::TORC_GOLD, 0);
	ch->set_ext_money(ExtMoney::TORC_SILVER, 0);
	ch->set_ext_money(ExtMoney::TORC_BRONZE, 0);

	snprintf(buf, sizeof(buf),
		"remort from %d to %d", ch->get_remort() - 1, ch->get_remort());
	snprintf(buf2, sizeof(buf2), "dest=%d", place_of_destination);
	add_karma(ch, buf, buf2);

	act("$n вступил$g в игру.", TRUE, ch, 0, 0, TO_ROOM);
	act("Вы перевоплотились! Желаем удачи!", FALSE, ch, 0, 0, TO_CHAR);
}

// returns the real number of the room with given virtual number
room_rnum real_room(room_vnum vnum)
{
	room_rnum bot, top, mid;

	bot = 1;		// 0 - room is NOWHERE
	top = top_of_world;
	// perform binary search on world-table
	for (;;)
	{
		mid = (bot + top) / 2;

		if (world[mid]->number == vnum)
			return (mid);
		if (bot >= top)
			return (NOWHERE);
		if (world[mid]->number > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}



// returns the real number of the monster with given virtual number
mob_rnum real_mobile(mob_vnum vnum)
{
	mob_rnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	// perform binary search on mob-table
	for (;;)
	{
		mid = (bot + top) / 2;

		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
bool check_object(OBJ_DATA* obj)
{
	bool error = false;

	if (GET_OBJ_WEIGHT(obj) < 0)
	{
		error = true;
		log("SYSERR: Object #%d (%s) has negative weight (%d).",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_WEIGHT(obj));
	}

	if (GET_OBJ_RENT(obj) <=0 )
	{
		error = true;
		log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_RENT(obj));
	}

	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
	if (strstr(buf, "UNDEFINED"))
	{
		error = true;
		log("SYSERR: Object #%d (%s) has unknown wear flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf, ",");
	if (strstr(buf, "UNDEFINED"))
	{
		error = true;
		log("SYSERR: Object #%d (%s) has unknown extra flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	obj->get_affect_flags().sprintbits(affected_bits, buf, ",");

	if (strstr(buf, "UNDEFINED"))
	{
		error = true;
		log("SYSERR: Object #%d (%s) has unknown affection flags.", GET_OBJ_VNUM(obj), obj->get_short_description().c_str());
	}

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
		if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0))
		{
			error = true;
			log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
				GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
		}
		break;

	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		error = error || check_object_level(obj, 0);
		error = error || check_object_spell_number(obj, 1);
		error = error || check_object_spell_number(obj, 2);
		error = error || check_object_spell_number(obj, 3);
		break;

	case OBJ_DATA::ITEM_BOOK:
		error = error || check_object_spell_number(obj, 1);
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		error = error || check_object_level(obj, 0);
		error = error || check_object_spell_number(obj, 3);
		if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1))
		{
			error = true;
			log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
				GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
		}
		break;

	default:
		break;
	}

	obj->remove_incorrect_values_keys(GET_OBJ_TYPE(obj));
	return error;
}

bool check_object_spell_number(OBJ_DATA * obj, unsigned val)
{
	if (val >= OBJ_DATA::VALS_COUNT)
	{
		log("SYSERROR : val=%d (%s:%d)", val, __FILE__, __LINE__);
		return true;
	}

	bool error = false;
	const char *spellname;

	if (GET_OBJ_VAL(obj, val) == -1)	// i.e.: no spell
		return error;

	/*
	 * Check for negative spells, spells beyond the top define, and any
	 * spell which is actually a skill.
	 */
	if (GET_OBJ_VAL(obj, val) < 0)
	{
		error = true;
	}
	if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
	{
		error = true;
	}
	if (error)
	{
		log("SYSERR: Object #%d (%s) has out of range spell #%d.",
			GET_OBJ_VNUM(obj), obj->get_short_description().c_str(), GET_OBJ_VAL(obj, val));
	}

	if (scheck)		// Spell names don't exist in syntax check mode.
	{
		return error;
	}

	// Now check for unnamed spells.
	spellname = spell_name(GET_OBJ_VAL(obj, val));

	if (error
		&& (spellname == unused_spellname
			|| !str_cmp("UNDEFINED", spellname)))
	{
		log("SYSERR: Object #%d (%s) uses '%s' spell #%d.", GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str(), spellname, GET_OBJ_VAL(obj, val));
	}

	return error;
}

bool check_object_level(OBJ_DATA * obj, int val)
{
	bool error = false;

	if (GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL)
	{
		error = true;
		log("SYSERR: Object #%d (%s) has out of range level #%d.",
			GET_OBJ_VNUM(obj),
			obj->get_short_description().c_str(),
			GET_OBJ_VAL(obj, val));
	}
	return error;
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
int must_be_deleted(CHAR_DATA * short_ch)
{
	int ci, timeout;

	if (PLR_FLAGS(short_ch).get(PLR_NODELETE))
	{
		return 0;
	}

	if (GET_REMORT(short_ch))
		return (0);

	if (PLR_FLAGS(short_ch).get(PLR_DELETED))
	{
		return 1;
	}

	timeout = -1;
	for (ci = 0; ci == 0 || pclean_criteria[ci].level > pclean_criteria[ci - 1].level; ci++)
	{
		if (GET_LEVEL(short_ch) <= pclean_criteria[ci].level)
		{
			timeout = pclean_criteria[ci].days;
			break;
		}
	}
	if (timeout >= 0)
	{
		timeout *= SECS_PER_REAL_DAY;
		if ((time(0) - LAST_LOGON(short_ch)) > timeout)
		{
			return (1);
		}
	}

	return (0);
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void entrycount(char *name)
{
	int i, deleted;
	char filename[MAX_STRING_LENGTH];

	if (get_filename(name, filename, PLAYERS_FILE))
	{
		Player t_short_ch;
		Player *short_ch = &t_short_ch;
		deleted = 1;
		// персонаж загружается неполностью
		if (load_char(name, short_ch, 1) > -1)
		{
			// если чар удален или им долго не входили, то не создаем для него запись
			if (!must_be_deleted(short_ch))
			{
				deleted = 0;
				// new record
				if (player_table)
					RECREATE(player_table, top_of_p_table + 2);
				else
					CREATE(player_table, 1);
				top_of_p_file++;
				top_of_p_table++;

				CREATE(player_table[top_of_p_table].name, strlen(GET_NAME(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].name[i] = '\0';
						(player_table[top_of_p_table].name[i] = LOWER(GET_NAME(short_ch)[i])); i++);
				//added by WorM 2010.08.27 в индексе чистим мыло и ip
				CREATE(player_table[top_of_p_table].mail, strlen(GET_EMAIL(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].mail[i] = '\0';
						(player_table[top_of_p_table].mail[i] = LOWER(GET_EMAIL(short_ch)[i])); i++);
				CREATE(player_table[top_of_p_table].last_ip, strlen(GET_LASTIP(short_ch)) + 1);
				for (i = 0, player_table[top_of_p_table].last_ip[i] = '\0';
						(player_table[top_of_p_table].last_ip[i] = GET_LASTIP(short_ch)[i]); i++);
				//end by WorM
				player_table[top_of_p_table].id = GET_IDNUM(short_ch);
				player_table[top_of_p_table].unique = GET_UNIQUE(short_ch);
				player_table[top_of_p_table].level = (GET_REMORT(short_ch) && !IS_IMMORTAL(short_ch)) ? 30 : GET_LEVEL(short_ch);
				player_table[top_of_p_table].timer = NULL;
				if (PLR_FLAGS(short_ch).get(PLR_DELETED))
				{
					player_table[top_of_p_table].last_logon = -1;
					player_table[top_of_p_table].activity = -1;
					/*//added by WorM 2010.08.27 в индексе чистим мыло и ip
					if(player_table[top_of_p_table].mail)
					{
						free(player_table[top_of_p_table].mail);
					}
					player_table[top_of_p_table].mail = NULL;
					if(player_table[top_of_p_table].last_ip)
					{
						free(player_table[top_of_p_table].last_ip);
					}
					player_table[top_of_p_table].last_ip = NULL;
					//end by WorM*/
				}
				else
				{
					player_table[top_of_p_table].last_logon = LAST_LOGON(short_ch);
					player_table[top_of_p_table].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
				}
				#ifdef TEST_BUILD
				log("entry: char:%s level:%d mail:%s ip:%s", player_table[top_of_p_table].name, player_table[top_of_p_table].level, player_table[top_of_p_table].mail, player_table[top_of_p_table].last_ip);
				#endif
				top_idnum = MAX(top_idnum, GET_IDNUM(short_ch));
				TopPlayer::Refresh(short_ch, 1);
				log("Add new player %s", player_table[top_of_p_table].name);
			}
		}
		// если чар уже удален, то стираем с диска его файл
		if (deleted)
		{
			log("Player %s already deleted - kill player file", name);
			remove(filename);
			// 2) Remove all other files
			get_filename(name, filename, ALIAS_FILE);
			remove(filename);
			get_filename(name, filename, SCRIPT_VARS_FILE);
			remove(filename);
			get_filename(name, filename, PERS_DEPOT_FILE);
			remove(filename);
			get_filename(name, filename, SHARE_DEPOT_FILE);
			remove(filename);
			get_filename(name, filename, PURGE_DEPOT_FILE);
			remove(filename);
			get_filename(name, filename, TEXT_CRASH_FILE);
			remove(filename);
			get_filename(name, filename, TIME_CRASH_FILE);
			remove(filename);
		}
	}
	return;
}

void new_build_player_index(void)
{
	FILE *players;
	char name[MAX_INPUT_LENGTH], playername[MAX_INPUT_LENGTH];
	int c;

	player_table = NULL;
	top_of_p_file = top_of_p_table = -1;
	if (!(players = fopen(LIB_PLRS "players.lst", "r")))
	{
		log("Players list empty...");
		return;
	}

	now_entrycount = TRUE;
	while (get_line(players, name))
	{
		if (!*name || *name == ';')
			continue;
		if (sscanf(name, "%s ", playername) == 0)
			continue;
		for (c = 0; c <= top_of_p_table; c++)
			if (!str_cmp(playername, player_table[c].name))
				break;
		if (c <= top_of_p_table)
			continue;
		entrycount(playername);
	}
	fclose(players);
	now_entrycount = FALSE;
}

void flush_player_index(void)
{
	FILE *players;
	char name[MAX_STRING_LENGTH];
	int i;

	if (!(players = fopen(LIB_PLRS "players.lst", "w+")))
	{
		log("Cann't save players list...");
		return;
	}
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (!player_table[i].name || !*player_table[i].name)
			continue;

		// check double
		// for (c = 0; c < i; c++)
		//     if (!str_cmp(player_table[c].name, player_table[i].name))
		//         break;
		// if (c < i)
		//    continue;

		sprintf(name, "%s %d %d %d %d\n",
				player_table[i].name,
				player_table[i].id, player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Сохранено индексов %d (считано при загрузке %d)", i, top_of_p_file + 1);
}

void dupe_player_index(void)
{
	FILE *players;
	char name[MAX_STRING_LENGTH];
	int i, c;

	sprintf(name, LIB_PLRS "players.dup");

	if (!(players = fopen(name, "w+")))
	{
		log("Cann't save players list...");
		return;
	}
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (!player_table[i].name || !*player_table[i].name)
			continue;

		// check double
		for (c = 0; c < i; c++)
			if (!str_cmp(player_table[c].name, player_table[i].name))
				break;
		if (c < i)
			continue;

		sprintf(name, "%s %d %d %d %d\n",
				player_table[i].name,
				player_table[i].id, player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Продублировано индексов %d (считано при загрузке %d)", i, top_of_p_file + 1);
}

void rename_char(CHAR_DATA * ch, char *oname)
{
	char filename[MAX_INPUT_LENGTH], ofilename[MAX_INPUT_LENGTH];

	// 1) Rename(if need) char and pkill file - directly
	log("Rename char %s->%s", GET_NAME(ch), oname);
	get_filename(oname, ofilename, PLAYERS_FILE);
	get_filename(GET_NAME(ch), filename, PLAYERS_FILE);
	rename(ofilename, filename);

	ch->save_char();

	// 2) Rename all other files
	get_filename(oname, ofilename, TEXT_CRASH_FILE);
	get_filename(GET_NAME(ch), filename, TEXT_CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, TIME_CRASH_FILE);
	get_filename(GET_NAME(ch), filename, TIME_CRASH_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, ALIAS_FILE);
	get_filename(GET_NAME(ch), filename, ALIAS_FILE);
	rename(ofilename, filename);

	get_filename(oname, ofilename, SCRIPT_VARS_FILE);
	get_filename(GET_NAME(ch), filename, SCRIPT_VARS_FILE);
	rename(ofilename, filename);

	// хранилища
	Depot::rename_char(ch);
	get_filename(oname, ofilename, PERS_DEPOT_FILE);
	get_filename(GET_NAME(ch), filename, PERS_DEPOT_FILE);
	rename(ofilename, filename);
	get_filename(oname, ofilename, PURGE_DEPOT_FILE);
	get_filename(GET_NAME(ch), filename, PURGE_DEPOT_FILE);
	rename(ofilename, filename);
}

// * Добровольное удаление персонажа через игровое меню.
void delete_char(const char *name)
{
	Player t_st;
	Player *st = &t_st;
	int id = load_char(name, st);

	if (id >= 0)
	{
		PLR_FLAGS(st).set(PLR_DELETED);
		NewNameRemove(st);
		Clan::remove_from_clan(GET_UNIQUE(st));
		st->save_char();

		Crash_clear_objects(id);
		player_table[id].unique = -1;
		player_table[id].level = -1;
		player_table[id].last_logon = -1;
		player_table[id].activity = -1;
		if(player_table[id].mail)
			free(player_table[id].mail);
		player_table[id].mail = NULL;
		if(player_table[id].last_ip)
			free(player_table[id].last_ip);
		player_table[id].last_ip = NULL;
	}
}

void room_copy(ROOM_DATA * dst, ROOM_DATA * src)
/*++
   Функция делает создает копию комнаты.
   После вызова этой функции создается полностью независимая копия комнты src
   за исключением полей track, contents, people, affected.
   Все поля имеют те же значения, но занимают свои области памяти.
      dst - "чистый" указатель на структуру room_data.
      src - исходная комната
   Примечание: Неочищенный указатель dst приведет к утечке памяти.
               Используйте redit_room_free() для очистки содержимого комнаты
--*/
{
	int i;
	{
		// Сохраняю track, contents, people, аффекты
		struct track_data *track = dst->track;
		OBJ_DATA *contents = dst->contents;
		CHAR_DATA *people = dst->people;
		auto affected = dst->affected;

		// Копирую все поверх
		*dst = *src;

		// Восстанавливаю track, contents, people, аффекты
		dst->track = track;
		dst->contents = contents;
		dst->people = people;
		dst->affected = affected;
	}

	// Теперь нужно выделить собственные области памяти

	// Название и описание
	dst->name = str_dup(src->name ? src->name : "неопределено");
	dst->temp_description = 0; // так надо

	// Выходы и входы
	for (i = 0; i < NUM_OF_DIRS; ++i)
	{
		const auto& rdd = src->dir_option[i];
		if (rdd)
		{
			dst->dir_option[i].reset(new EXIT_DATA());
			// Копируем числа
			*dst->dir_option[i] = *rdd;
			// Выделяем память
			dst->dir_option[i]->general_description = rdd->general_description;
			dst->dir_option[i]->keyword = (rdd->keyword ? str_dup(rdd->keyword) : NULL);
			dst->dir_option[i]->vkeyword = (rdd->vkeyword ? str_dup(rdd->vkeyword) : NULL);
		}
	}

	// Дополнительные описания, если есть
	EXTRA_DESCR_DATA::shared_ptr* pddd = &dst->ex_description;
	EXTRA_DESCR_DATA::shared_ptr sdd = src->ex_description;
	*pddd = nullptr;

	while (sdd)
	{
		pddd->reset(new EXTRA_DESCR_DATA());
		(*pddd)->keyword = sdd->keyword ? str_dup(sdd->keyword) : nullptr;
		(*pddd)->description = sdd->description ? str_dup(sdd->description) : nullptr;
		pddd = &((*pddd)->next);
		sdd = sdd->next;
	}

	// Копирую скрипт и прототипы
	SCRIPT(dst) = nullptr;
	dst->proto_script.clear();
	dst->proto_script = src->proto_script;

	im_inglist_copy(&dst->ing_list, src->ing_list);
}

void free_script(SCRIPT_DATA * sc);

void room_free(ROOM_DATA * room)
/*++
   Функция полностью освобождает память, занимаемую данными комнаты.
   ВНИМАНИЕ. Память самой структуры room_data не освобождается.
             Необходимо дополнительно использовать delete()
--*/
{
	int i;

	// Название и описание
	if (room->name)
		free(room->name);
	if (room->temp_description)
	{
		free(room->temp_description);
		room->temp_description = 0;
	}

	// Выходы и входы
	for (i = 0; i < NUM_OF_DIRS; i++)
	{
		if (room->dir_option[i])
		{
			if (room->dir_option[i]->keyword)
				free(room->dir_option[i]->keyword);
			if (room->dir_option[i]->vkeyword)
				free(room->dir_option[i]->vkeyword);
			room->dir_option[i].reset();
		}
	}

	// Скрипт
	free_script(SCRIPT(room));

	if (room->ing_list)
	{
		free(room->ing_list);
		room->ing_list = NULL;
	}

	room->affected.clear();
}

void LoadGlobalUID(void)
{
	FILE *guid;
	char buffer[256];

	global_uid = 0;

	if (!(guid = fopen(LIB_MISC "globaluid", "r")))
	{
		log("Can't open global uid file...");
		return;
	}
	get_line(guid, buffer);
	global_uid = atoi(buffer);
	fclose(guid);
	return;
}

void SaveGlobalUID(void)
{
	FILE *guid;

	if (!(guid = fopen(LIB_MISC "globaluid", "w")))
	{
		log("Can't write global uid file...");
		return;
	}

	fprintf(guid, "%d\n", global_uid);
	fclose(guid);
	return;
}

void load_guardians()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"guards.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node xMainNode = doc.child("guardians");

	if (!xMainNode)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...guards.xml read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	guardian_list.clear();

	int num_wars_global = atoi(xMainNode.child_value("wars"));

	struct mob_guardian tmp_guard;
	for (pugi::xml_node xNodeGuard = xMainNode.child("guard");xNodeGuard; xNodeGuard = xNodeGuard.next_sibling("guard"))
	{
		int guard_vnum = xNodeGuard.attribute("vnum").as_int();

		if (guard_vnum <= 0)
		{
			log("ERROR: Ошибка загрузки файла %s - некорректное значение VNUM: %d", LIB_MISC"guards.xml", guard_vnum);
			continue;
		}
		//значения по умолчанию
		tmp_guard.max_wars_allow = num_wars_global;
		tmp_guard.agro_all_agressors = false;
		tmp_guard.agro_argressors_in_zones.clear();
		tmp_guard.agro_killers = true;

		int num_wars = xNodeGuard.attribute("wars").as_int();

		if (num_wars && (num_wars != num_wars_global))
			tmp_guard.max_wars_allow = num_wars;

		if (!strcmp(xNodeGuard.attribute("killer").value(),"no"))
			tmp_guard.agro_killers = false;

		if (!strcmp(xNodeGuard.attribute("agressor").value(),"yes"))
			tmp_guard.agro_all_agressors = true;

		if (!strcmp(xNodeGuard.attribute("agressor").value(),"list"))
		{
			for (pugi::xml_node xNodeZone = xNodeGuard.child("zone"); xNodeZone; xNodeZone = xNodeZone.next_sibling("zone"))
			{
				tmp_guard.agro_argressors_in_zones.push_back(atoi(xNodeZone.child_value()));
			}
		}
		guardian_list[guard_vnum] = tmp_guard;
	}

}

//Polud тестовый класс для хранения параметров различных рас мобов
//Читает данные из файла
const char *MOBRACE_FILE = LIB_MISC_MOBRACES "mobrace.xml";

MobRaceListType mobraces_list;

//загрузка рас из файла
void load_mobraces()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOBRACE_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node node_list = doc.child("mobraces");

	if (!node_list)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...mobraces read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	for (pugi::xml_node  race = node_list.child("mobrace");race; race = race.next_sibling("mobrace"))
	{
		MobRacePtr tmp_mobrace(new MobRace);
		tmp_mobrace->race_name = race.attribute("name").value();
		int race_num = race.attribute("key").as_int();

		pugi::xml_node imList = race.child("imlist");

		for (pugi::xml_node im = imList.child("im");im;im = im.next_sibling("im"))
		{
			struct ingredient tmp_ingr;
			tmp_ingr.imtype = im.attribute("type").as_int();
			tmp_ingr.imname = string(im.attribute("name").value());
			boost::trim(tmp_ingr.imname);
			int cur_lvl=1;
			int prob_value = 1;
			for (pugi::xml_node prob = im.child("prob"); prob; prob = prob.next_sibling("prob"))
			{
				int next_lvl = prob.attribute("lvl").as_int();
				if (next_lvl>0)
				{
					for (int lvl=cur_lvl; lvl < next_lvl; lvl++)
						tmp_ingr.prob[lvl-1] = prob_value;
				}
				else
				{
					log("SYSERROR: Неверный уровень lvl=%d для ингредиента %s расы %s", next_lvl, tmp_ingr.imname.c_str(), tmp_mobrace->race_name.c_str());
					return;
				}
				prob_value = atoi(prob.child_value());
				cur_lvl = next_lvl;
			}
			for (int lvl=cur_lvl; lvl <= MAX_MOB_LEVEL; lvl++)
				tmp_ingr.prob[lvl-1] = prob_value;

			tmp_mobrace->ingrlist.push_back(tmp_ingr);
		}
		mobraces_list[race_num] = tmp_mobrace;
	}
}


MobRace::MobRace()
{
	ingrlist.clear();
}

MobRace::~MobRace()
{
	ingrlist.clear();
}
//-Polud

////////////////////////////////////////////////////////////////////////////////
namespace OfftopSystem
{

const char* BLOCK_FILE = LIB_MISC"offtop.lst";
std::vector<std::string> block_list;

/// Проверка на наличие чара в стоп-списке и сет флага
void set_flag(CHAR_DATA* ch)
{
	std::string mail(GET_EMAIL(ch));
	lower_convert(mail);
	auto i = std::find(block_list.begin(), block_list.end(), mail);
	if (i != block_list.end())
	{
		PRF_FLAGS(ch).set(PRF_IGVA_PRONA);
	}
	else
	{
		PRF_FLAGS(ch).unset(PRF_IGVA_PRONA);
	}
}

/// Лоад/релоад списка нежелательных для оффтопа товарисчей.
void init()
{
	block_list.clear();
	std::ifstream file(BLOCK_FILE);
	if (!file.is_open())
	{
		log("SYSERROR: не удалось открыть файл на чтение: %s", BLOCK_FILE);
		return;
	}
	std::string buffer;
	while (file >> buffer)
	{
		lower_convert(buffer);
		block_list.push_back(buffer);
	}
	for (DESCRIPTOR_DATA* d = descriptor_list; d; d = d->next)
	{
		if (d->character)
		{
			set_flag(d->character);
		}
	}
}

} // namespace OfftopSystem
////////////////////////////////////////////////////////////////////////////////

size_t CObjectPrototypes::add(CObjectPrototype* prototype, const obj_vnum vnum)
{
	return add(CObjectPrototype::shared_ptr(prototype), vnum);
}

size_t CObjectPrototypes::add(const CObjectPrototype::shared_ptr& prototype, const obj_vnum vnum)
{
	const auto index = m_index.size();
	prototype->set_rnum(static_cast<int>(index));
	m_vnum2index[vnum] = index;
	m_prototypes.push_back(prototype);
	m_index.push_back(SPrototypeIndex());
	return index;
}

int CObjectPrototypes::rnum(const obj_vnum vnum) const
{
	vnum2index_t::const_iterator i = m_vnum2index.find(vnum);
	return i == m_vnum2index.end() ? -1 : static_cast<int>(i->second);
}

void CObjectPrototypes::set(const size_t index, CObjectPrototype* new_value)
{
	new_value->set_rnum(static_cast<int>(index));
	m_prototypes[index].reset(new_value);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
