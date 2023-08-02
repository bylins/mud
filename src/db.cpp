
/************************************************************************
*   File: db.cpp                                        Part of Bylins    *
*  Usage: Loading/saving entities, booting/resetting world, internal funcs   *
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

#define DB_C__

#include "administration/accounts.h"
#include "cmd_god/ban.h"
#include "boards/boards.h"
#include "boot/boot_data_files.h"
#include "boot/boot_index.h"
#include "communication/social.h"
#include "game_crafts/jewelry.h"
#include "game_crafts/mining.h"
#include "entities/player_races.h"
#include "cmd/follow.h"
#include "corpse.h"
#include "game_mechanics/deathtrap.h"
#include "description.h"
#include "depot.h"
#include "game_economics/ext_money.h"
#include "game_mechanics/bonus.h"
#include "game_fight/fight.h"
#include "game_fight/mobact.h"
#include "utils/file_crc.h"
#include "structs/global_objects.h"
#include "game_mechanics/glory.h"
#include "game_mechanics/glory_const.h"
#include "game_mechanics/glory_misc.h"
#include "handler.h"
#include "help.h"
#include "house.h"
#include "game_crafts/item_creation.h"
#include "liquid.h"
#include "communication/mail.h"
#include "statistics/mob_stat.h"
#include "modify.h"
#include "game_mechanics/named_stuff.h"
#include "administration/names.h"
#include "noob.h"
#include "obj_prototypes.h"
#include "olc/olc.h"
#include "communication/parcel.h"
#include "administration/privilege.h"
#include "game_mechanics/sets_drop.h"
#include "game_economics/shop_ext.h"
#include "game_skills/townportal.h"
#include "stuff.h"
#include "title.h"
#include "statistics/top.h"
#include "backtrace.h"

#include <third_party_libs/fmt/include/fmt/format.h>
#include <sys/stat.h>

#define CRITERION_FILE "criterion.xml"
#define CASES_FILE "cases.xml"
#define RANDOMOBJ_FILE "randomobj.xml"
#define SPEEDWALKS_FILE "speedwalks.xml"
#define CITIES_FILE "cities.xml"
#define QUESTBODRICH_FILE "quest_bodrich.xml"


/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
long beginning_of_time = -1561789232;
#else
long beginning_of_time = 650336715;
#endif

Rooms &world = GlobalObjects::world();

RoomRnum top_of_world = 0;    // ref to top element of world

void add_trig_index_entry(int nr, Trigger *trig) {
	IndexData *index;
	CREATE(index, 1);
	index->vnum = nr;
	index->total_online = 0;
	index->func = nullptr;
	index->proto = trig;

	trig_index[top_of_trigt++] = index;
}

IndexData **trig_index;    // index table for triggers
int top_of_trigt = 0;        // top of trigger index table

IndexData *mob_index;        // index table for mobile file
MobRnum top_of_mobt = 0;    // top of mobile index table
std::unordered_map<MobVnum, std::vector<long>> mob_id_by_vnum;
std::unordered_map<long, CharData *> mob_by_uid;

void Load_Criterion(pugi::xml_node XMLCriterion, int type);
void load_speedwalk();
void load_class_limit();
int global_uid = 0;

//extern struct AttackMessages fight_messages[kMaxMessages];    // fighting messages
PlayersIndex &player_table = GlobalObjects::player_table();    // index to plr file

bool player_exists(const long id) { return player_table.player_exists(id); }

FILE *player_fl = nullptr;        // file desc of player file
long top_idnum = 0;        // highest idnum in use

int circle_restrict = 0;    // level of game restriction
RoomRnum r_mortal_start_room;    // rnum of mortal start room
RoomRnum r_immort_start_room;    // rnum of immort start room
RoomRnum r_frozen_start_room;    // rnum of frozen start room
RoomRnum r_helled_start_room;
RoomRnum r_named_start_room;
RoomRnum r_unreg_start_room;

char *credits = nullptr;        // game credits
char *motd = nullptr;        // message of the day - mortals
char *rules = nullptr;        // rules for immorts
char *GREETINGS = nullptr;        // opening credits screen
char *help = nullptr;        // help screen
char *info = nullptr;        // info page
char *immlist = nullptr;        // list of peon gods
char *background = nullptr;    // background story
char *handbook = nullptr;        // handbook for new immortals
char *policies = nullptr;        // policies page
char *name_rules = nullptr;        // rules of character's names

TimeInfoData time_info;    // the infomation about the time
//struct Weather weather_info;    // the infomation about the weather
struct reset_q_type reset_q;    // queue of zones to be reset

const FlagData clear_flags;

struct Portal *portals_list;    // Список проталов для townportal

const char *ZONE_TRAFFIC_FILE = LIB_PLRSTUFF"zone_traffic.xml";
time_t zones_stat_date;

guardian_type guardian_list;

GameLoader::GameLoader() {
}

GameLoader world_loader;

QuestBodrich qb;

// local functions
void LoadGlobalUID();
void parse_room(FILE *fl, int virtual_nr, int virt);
void parse_object(FILE *obj_f, const int nr);
void load_help(FILE *fl);
void assign_mobiles();
void assign_objects();
void assign_rooms();
void init_spec_procs();
void build_player_index();
void reset_zone(ZoneRnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void do_reboot(CharData *ch, char *argument, int cmd, int subcmd);
void check_start_rooms();
void add_vrooms_to_all_zones();
void renum_world();
void renum_zone_table();
void log_zone_error(ZoneRnum zone, int cmd_no, const char *message);
void reset_time();
int mobs_in_room(int m_num, int r_num);
void new_build_player_index();
void renum_obj_zone();
void renum_mob_zone();
//int GetZoneRooms(int, int *, int *);
//int GetZoneRooms1(int, int *, int *);
void init_basic_values();
void init_portals();
void init_im();
void init_zone_types();
void LoadGuardians();

TimeInfoData *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages();
void sort_commands();
void Read_Invalid_List();
int find_name(const char *name);
int csort(const void *a, const void *b);
void prune_crlf(char *txt);
int Crash_read_timer(const std::size_t index, int temp);
void Crash_clear_objects(const std::size_t index);
int exchange_database_load(void);

void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void calc_easter(void);
void do_start(CharData *ch, int newbie);
long GetExpUntilNextLvl(CharData *ch, int level);
//extern char *fread_action(FILE *fl, int nr);
void load_mobraces();

// external vars
extern int number_of_social_messages;
extern int number_of_social_commands;
extern int no_specials;
extern RoomVnum mortal_start_room;
extern RoomVnum immort_start_room;
extern RoomVnum frozen_start_room;
extern RoomVnum helled_start_room;
extern RoomVnum named_start_room;
extern RoomVnum unreg_start_room;
extern DescriptorData *descriptor_list;
extern struct MonthTemperature year_temp[];
extern char *house_rank[];
extern struct PCCleanCriteria pclean_criteria[];
extern void LoadProxyList();
extern void add_karma(CharData *ch, const char *punish, const char *reason);
extern void RepopDecay(std::vector<ZoneRnum> zone_list);    // рассыпание обьектов ITEM_REPOP_DECAY
extern void extract_trigger(Trigger *trig);

char *fread_action(FILE *fl, int nr) {
	char buf[kMaxStringLength];

	const char *result = fgets(buf, kMaxStringLength, fl);
	UNUSED_ARG(result);

	if (feof(fl)) {
		log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
		exit(1);
	}
	if (*buf == '#')
		return (nullptr);

	buf[strlen(buf) - 1] = '\0';
	return (str_dup(buf));
}

int strchrn(const char *s, int c) {

	int n = -1;
	while (*s) {
		n++;
		if (*s == c) return n;
		s++;
	}
	return -1;
}

// поиск подстроки
int strchrs(const char *s, const char *t) {
	while (*t) {
		int r = strchrn(s, *t);
		if (r > -1) return r;
		t++;
	}
	return -1;
}

struct Item_struct {
	// для параметров
	std::map<std::string, double> params;
	// для аффектов
	std::map<std::string, double> affects;
};

// массив для критерии, каждый элемент массива это отдельный слот
Item_struct items_struct[17]; // = new Item_struct[16];

// определение степени двойки
int exp_two_implementation(int number) {
	int count = 0;
	while (true) {
		const int tmp = 1 << count;
		if (number < tmp) {
			return -1;
		}
		if (number == tmp) {
			return count;
		}
		count++;
	}
}

template<typename EnumType>
inline int exp_two(EnumType number) {
	return exp_two_implementation(to_underlying(number));
}

template<>
inline int exp_two(int number) { return exp_two_implementation(number); }

bool check_obj_in_system_zone(int vnum) {
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

bool check_unlimited_timer(const CObjectPrototype *obj) {
	char buf_temp[kMaxStringLength];
	char buf_temp1[kMaxStringLength];
	//sleep(15);
	// куда одевается наш предмет
	int item_wear = -1;
	bool type_item = false;
	if (GET_OBJ_TYPE(obj) == EObjType::kArmor
		|| GET_OBJ_TYPE(obj) == EObjType::kStaff
		|| GET_OBJ_TYPE(obj) == EObjType::kWorm
		|| GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		type_item = true;
	}
	// сумма для статов
	double sum = 0;
	// сумма для аффектов
	double sum_aff = 0;
	// по другому чот не получилось
	if (obj->has_wear_flag(EWearFlag::kFinger)) {
		item_wear = exp_two(EWearFlag::kFinger);
	}

	if (obj->has_wear_flag(EWearFlag::kNeck)) {
		item_wear = exp_two(EWearFlag::kNeck);
	}

	if (obj->has_wear_flag(EWearFlag::kBody)) {
		item_wear = exp_two(EWearFlag::kBody);
	}

	if (obj->has_wear_flag(EWearFlag::kHead)) {
		item_wear = exp_two(EWearFlag::kHead);
	}

	if (obj->has_wear_flag(EWearFlag::kLegs)) {
		item_wear = exp_two(EWearFlag::kLegs);
	}

	if (obj->has_wear_flag(EWearFlag::kFeet)) {
		item_wear = exp_two(EWearFlag::kFeet);
	}

	if (obj->has_wear_flag(EWearFlag::kHands)) {
		item_wear = exp_two(EWearFlag::kHands);
	}

	if (obj->has_wear_flag(EWearFlag::kArms)) {
		item_wear = exp_two(EWearFlag::kArms);
	}

	if (obj->has_wear_flag(EWearFlag::kShield)) {
		item_wear = exp_two(EWearFlag::kShield);
	}

	if (obj->has_wear_flag(EWearFlag::kShoulders)) {
		item_wear = exp_two(EWearFlag::kShoulders);
	}

	if (obj->has_wear_flag(EWearFlag::kWaist)) {
		item_wear = exp_two(EWearFlag::kWaist);
	}

	if (obj->has_wear_flag(EWearFlag::kQuiver)) {
		item_wear = exp_two(EWearFlag::kQuiver);
	}

	if (obj->has_wear_flag(EWearFlag::kWrist)) {
		item_wear = exp_two(EWearFlag::kWrist);
	}

	if (obj->has_wear_flag(EWearFlag::kBoth)) {
		item_wear = exp_two(EWearFlag::kBoth);
	}

	if (obj->has_wear_flag(EWearFlag::kWield)) {
		item_wear = exp_two(EWearFlag::kWield);
	}

	if (obj->has_wear_flag(EWearFlag::kHold)) {
		item_wear = exp_two(EWearFlag::kHold);
	}

	if (!type_item) {
		return false;
	}
	// находится ли объект в системной зоне
	if (check_obj_in_system_zone(obj->get_vnum())) {
		return false;
	}
	// если объект никуда не надевается, то все, облом
	if (item_wear == -1) {
		return false;
	}
	// если шмотка магическая или энчантнута таймер обычный
	if (obj->has_flag(EObjFlag::kMagic)) {
		return false;
	}
	// если это сетовый предмет
	if (obj->has_flag(EObjFlag::KSetItem)) {
		return false;
	}
	// !нерушима
	if (obj->has_flag(EObjFlag::KLimitedTimer)) {
		return false;
	}
	// рассыпется вне зоны
	if (obj->has_flag(EObjFlag::kZonedacay)) {
		return false;
	}
	// рассыпется на репоп зоны
	if (obj->has_flag(EObjFlag::kRepopDecay)) {
		return false;
	}

	// если предмет требует реморты, то он явно овер
	if (obj->get_auto_mort_req() > 0)
		return false;
	if (obj->get_minimum_remorts() > 0)
		return false;
	// проверяем дырки в предмете
	if (obj->has_flag(EObjFlag::kHasOneSlot)
		|| obj->has_flag(EObjFlag::kHasTwoSlots)
		|| obj->has_flag(EObjFlag::kHasThreeSlots)) {
		return false;
	}
	// если у объекта таймер ноль, то облом.
	if (obj->get_timer() == 0) {
		return false;
	}
	// проходим по всем характеристикам предмета
	for (int i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).modifier) {
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// проходим по нашей таблице с критериями
			for (std::map<std::string, double>::iterator it = items_struct[item_wear].params.begin();
				 it != items_struct[item_wear].params.end(); it++) {
				if (strcmp(it->first.c_str(), buf_temp) == 0) {
					if (obj->get_affected(i).modifier > 0) {
						sum += it->second * obj->get_affected(i).modifier;
					}
				}
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");

	// проходим по всем аффектам в нашей таблице
	for (std::map<std::string, double>::iterator it = items_struct[item_wear].affects.begin();
		 it != items_struct[item_wear].affects.end(); it++) {
		// проверяем, есть ли наш аффект на предмете
		if (strstr(buf_temp1, it->first.c_str()) != nullptr) {
			sum_aff += it->second;
		}
		//std::cout << it->first << " " << it->second << std::endl;
	}

	// если сумма больше или равна единице
	if (sum >= 1) {
		return false;
	}

	// тоже самое для аффектов на объекте
	if (sum_aff >= 1) {
		return false;
	}

	// иначе все норм
	return true;
}

float count_koef_obj(const CObjectPrototype *obj, int item_wear) {
	double sum = 0.0;
	double sum_aff = 0.0;
	char buf_temp[kMaxStringLength];
	char buf_temp1[kMaxStringLength];

	// проходим по всем характеристикам предмета
	for (int i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).modifier) {
			sprinttype(obj->get_affected(i).location, apply_types, buf_temp);
			// проходим по нашей таблице с критериями
			for (std::map<std::string, double>::iterator it = items_struct[item_wear].params.begin();
				 it != items_struct[item_wear].params.end(); it++) {

				if (strcmp(it->first.c_str(), buf_temp) == 0) {
					if (obj->get_affected(i).modifier > 0) {
						sum += it->second * obj->get_affected(i).modifier;
					}
				}

				//std::cout << it->first << " " << it->second << std::endl;
			}
		}
	}
	obj->get_affect_flags().sprintbits(weapon_affects, buf_temp1, ",");

	// проходим по всем аффектам в нашей таблице
	for (std::map<std::string, double>::iterator it = items_struct[item_wear].affects.begin();
		 it != items_struct[item_wear].affects.end(); it++) {
		// проверяем, есть ли наш аффект на предмете
		if (strstr(buf_temp1, it->first.c_str()) != nullptr) {
			sum_aff += it->second;
		}
		//std::cout << it->first << " " << it->second << std::endl;
	}
	sum += sum_aff;
	return sum;
}

float count_unlimited_timer(const CObjectPrototype *obj) {
	float result = 0.0;
	bool type_item = false;
	if (GET_OBJ_TYPE(obj) == EObjType::kArmor
		|| GET_OBJ_TYPE(obj) == EObjType::kStaff
		|| GET_OBJ_TYPE(obj) == EObjType::kWorm
		|| GET_OBJ_TYPE(obj) == EObjType::kWeapon) {
		type_item = true;
	}
	// сумма для статов

	result = 0.0;

	if (CAN_WEAR(obj, EWearFlag::kFinger)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kFinger));
	}

	if (CAN_WEAR(obj, EWearFlag::kNeck)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kNeck));
	}

	if (CAN_WEAR(obj, EWearFlag::kBody)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kBody));
	}

	if (CAN_WEAR(obj, EWearFlag::kHead)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kHead));
	}

	if (CAN_WEAR(obj, EWearFlag::kLegs)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kLegs));
	}

	if (CAN_WEAR(obj, EWearFlag::kFeet)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kFeet));
	}

	if (CAN_WEAR(obj, EWearFlag::kHands)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kHands));
	}

	if (CAN_WEAR(obj, EWearFlag::kArms)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kArms));
	}

	if (CAN_WEAR(obj, EWearFlag::kShield)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kShield));
	}

	if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kShoulders));
	}

	if (CAN_WEAR(obj, EWearFlag::kWaist)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kWaist));
	}

	if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kQuiver));
	}

	if (CAN_WEAR(obj, EWearFlag::kWrist)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kWrist));
	}

	if (CAN_WEAR(obj, EWearFlag::kWield)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kWield));
	}

	if (CAN_WEAR(obj, EWearFlag::kHold)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kHold));
	}

	if (CAN_WEAR(obj, EWearFlag::kBoth)) {
		result += count_koef_obj(obj, exp_two(EWearFlag::kBoth));
	}

	if (!type_item) {
		return 0.0;
	}

	return result;
}

float count_mort_requred(const CObjectPrototype *obj) {
	float result = 0.0;
	const float SQRT_MOD = 1.7095f;
	const int AFF_SHIELD_MOD = 30;
	const int AFF_MAGICGLASS_MOD = 10;
	const int AFF_BLINK_MOD = 10;
	const int AFF_CLOUDLY_MOD = 10;

	result = 0.0;

	if (ObjSystem::is_mob_item(obj) || obj->has_flag(EObjFlag::KSetItem)) {
		return result;
	}

	float total_weight = 0.0;

	// аффекты APPLY_x
	for (int k = 0; k < kMaxObjAffect; k++) {
		if (obj->get_affected(k).location == 0) continue;

		// случай, если один аффект прописан в нескольких полях
		for (int kk = 0; kk < kMaxObjAffect; kk++) {
			if (obj->get_affected(k).location == obj->get_affected(kk).location
				&& k != kk) {
				log("SYSERROR: double affect=%d, ObjVnum=%d",
					obj->get_affected(k).location, GET_OBJ_VNUM(obj));
				return 1000000;
			}
		}
		if ((obj->get_affected(k).modifier > 0) && ((obj->get_affected(k).location != EApply::kAc) &&
			(obj->get_affected(k).location != EApply::kSavingWill) &&
			(obj->get_affected(k).location != EApply::kSavingCritical) &&
			(obj->get_affected(k).location != EApply::kSavingStability) &&
			(obj->get_affected(k).location != EApply::kSavingReflex))) {
			float weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
			total_weight += pow(weight, SQRT_MOD);
		}
			// савесы которые с минусом должны тогда понижать вес если в +
		else if ((obj->get_affected(k).modifier > 0) && ((obj->get_affected(k).location == EApply::kAc) ||
			(obj->get_affected(k).location == EApply::kSavingWill) ||
			(obj->get_affected(k).location == EApply::kSavingCritical) ||
			(obj->get_affected(k).location == EApply::kSavingStability) ||
			(obj->get_affected(k).location == EApply::kSavingReflex))) {
			float weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, 0 - obj->get_affected(k).modifier);
			total_weight -= pow(weight, -SQRT_MOD);
		}
			//Добавленый кусок учет савесов с - значениями
		else if ((obj->get_affected(k).modifier < 0)
			&& ((obj->get_affected(k).location == EApply::kAc) ||
				(obj->get_affected(k).location == EApply::kSavingWill) ||
				(obj->get_affected(k).location == EApply::kSavingCritical) ||
				(obj->get_affected(k).location == EApply::kSavingStability) ||
				(obj->get_affected(k).location == EApply::kSavingReflex))) {
			float weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, obj->get_affected(k).modifier);
			total_weight += pow(weight, SQRT_MOD);
		}
			//Добавленый кусок учет отрицательного значения но не савесов
		else if ((obj->get_affected(k).modifier < 0)
			&& ((obj->get_affected(k).location != EApply::kAc) &&
				(obj->get_affected(k).location != EApply::kSavingWill) &&
				(obj->get_affected(k).location != EApply::kSavingCritical) &&
				(obj->get_affected(k).location != EApply::kSavingStability) &&
				(obj->get_affected(k).location != EApply::kSavingReflex))) {
			float weight =
				ObjSystem::count_affect_weight(obj, obj->get_affected(k).location, 0 - obj->get_affected(k).modifier);
			total_weight -= pow(weight, -SQRT_MOD);
		}
	}
	// аффекты AFF_x через weapon_affect
	for (const auto &m : weapon_affect) {
		if (IS_OBJ_AFF(obj, m.aff_pos)) {
			if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kAirShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kFireShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kIceShield) {
				total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kMagicGlass) {
				total_weight += pow(AFF_MAGICGLASS_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kBlink) {
				total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
			} else if (static_cast<EAffect>(m.aff_bitvector) == EAffect::kCloudly) {
				total_weight += pow(AFF_CLOUDLY_MOD, SQRT_MOD);
			}
		}
	}

	if (total_weight < 1)
		return result;

	result = ceil(pow(total_weight, 1 / SQRT_MOD));

	return result;
}

void Load_Criterion(pugi::xml_node XMLCriterion, const EWearFlag type) {
	int index = exp_two(type);
	pugi::xml_node params, CurNode, affects;
	params = XMLCriterion.child("params");
	affects = XMLCriterion.child("affects");

	// добавляем в массив все параметры, типа силы, ловкости, каст и тд
	for (CurNode = params.child("param"); CurNode; CurNode = CurNode.next_sibling("param")) {
		items_struct[index].params.insert(std::make_pair(CurNode.attribute("name").value(),
														 CurNode.attribute("value").as_double()));
		//log("str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}

	// добавляем в массив все аффекты
	for (CurNode = affects.child("affect"); CurNode; CurNode = CurNode.next_sibling("affect")) {
		items_struct[index].affects.insert(std::make_pair(CurNode.attribute("name").value(),
														  CurNode.attribute("value").as_double()));
		//log("Affects:str:%s, double:%f", CurNode.attribute("name").value(),  CurNode.attribute("value").as_double());
	}
}

// Separate a 4-character id tag from the data it precedes
void tag_argument(char *argument, char *tag) {
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

void go_boot_socials() {
	int i;

	if (soc_mess_list) {
		for (i = 0; i < number_of_social_messages; i++) {
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
	if (soc_keys_list) {
		for (i = 0; i < number_of_social_commands; i++)
			if (soc_keys_list[i].keyword)
				free(soc_keys_list[i].keyword);
		free(soc_keys_list);
	}
	number_of_social_messages = -1;
	number_of_social_commands = -1;
	world_loader.index_boot(DB_BOOT_SOCIAL);
}

void load_sheduled_reboot() {
	FILE *sch;
	int day = 0, hour = 0, minutes = 0, numofreaded = 0, timeOffset = 0;
	char str[10];
	int offsets[7];

	time_t currTime;

	sch = fopen(LIB_MISC "schedule", "r");
	if (!sch) {
		log("Error opening schedule");
		return;
	}
	time(&currTime);
	for (int i = 0; i < 7; i++) {
		if (fgets(str, 10, sch) == nullptr) {
			break;
		}
		numofreaded = sscanf(str, "%i %i:%i", &day, &hour, &minutes);
		if (numofreaded < 1) {
			continue;
		}
		if (day != i) {
			break;
		}
		if (numofreaded == 1) {
			offsets[i] = 24 * 60;
			continue;
		}
		offsets[i] = hour * 60 + minutes;
	}
	fclose(sch);

	timeOffset = (int) difftime(currTime, shutdown_parameters.get_boot_time());

	//set reboot_uptime equal to current uptime and align to the start of the day
	shutdown_parameters.set_reboot_uptime(
		timeOffset / 60 - localtime(&currTime)->tm_min - 60 * (localtime(&currTime)->tm_hour));
	const auto boot_time = shutdown_parameters.get_boot_time();
	const auto local_boot_time = localtime(&boot_time);
	for (int i = local_boot_time->tm_wday; i < 7; i++) {
		//7 empty days was cycled - break with default uptime
		if (shutdown_parameters.get_reboot_uptime() - 1440 * 7 >= 0) {
			shutdown_parameters.set_reboot_uptime(DEFAULT_REBOOT_UPTIME);    //2 days reboot bu default if no schedule
			break;
		}

		//if we get non-1-full-day offset, but server will reboot to early (36 hour is minimum)
		//we are going to find another scheduled day
		if (offsets[i] < 24 * 60 && (shutdown_parameters.get_reboot_uptime() + offsets[i]) < UPTIME_THRESHOLD * 60) {
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + 24 * 60);
		}
		//we've found next point of reboot! :) break cycle
		if (offsets[i] < 24 * 60
			&& (shutdown_parameters.get_reboot_uptime() + offsets[i]) > UPTIME_THRESHOLD * 60) {
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + offsets[i]);
			break;
		}
		//empty day - add 24 hour and go next
		if (offsets[i] == 24 * 60) {
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + offsets[i]);
		}
		// jump to 1st day of the week
		if (i == 6) {
			i = -1;
		}
	}
	log("Setting up reboot_uptime: %i", shutdown_parameters.get_reboot_uptime());
}

// Базовая функция загрузки XML конфигов
pugi::xml_node XMLLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document &Doc) {
	std::ostringstream buffer;
	pugi::xml_parse_result Result;
	pugi::xml_node NodeList;

	// Пробуем прочитать файл
	Result = Doc.load_file(PathToFile);
	// Oops, файла нет
	if (!Result) {
		buffer << "..." << Result.description() << "(file: " << PathToFile << ")";
		mudlog(std::string(buffer.str()).c_str(), CMP, kLvlImmortal, SYSLOG, true);
		return NodeList;
	}

	// Ищем в файле указанный тэг
	NodeList = Doc.child(MainTag);
	// Тэга нет - кляузничаем в сислоге
	if (!NodeList) {
		mudlog(ErrorStr, CMP, kLvlImmortal, SYSLOG, true);
	}

	return NodeList;
};

pugi::xml_node XMLLoad(const std::string &PathToFile, const std::string &MainTag, const std::string &ErrorStr, pugi::xml_document &Doc) {
	return XMLLoad(PathToFile.c_str(), MainTag.c_str(), ErrorStr.c_str(), Doc);
}

std::vector<TreasureCase> cases;
// Заггрузка сундуков в мире
void load_cases() {
	pugi::xml_document doc_cases;
	pugi::xml_node case_, object_, file_case;
	file_case = XMLLoad(LIB_MISC CASES_FILE, "cases", "Error loading cases file: cases.xml", doc_cases);
	for (case_ = file_case.child("casef"); case_; case_ = case_.next_sibling("casef")) {
		TreasureCase __case;
		__case.vnum = case_.attribute("vnum").as_int();
		__case.drop_chance = case_.attribute("drop_chance").as_int();
		for (object_ = case_.child("object"); object_; object_ = object_.next_sibling("object")) {
			__case.vnum_objs.push_back(object_.attribute("vnum").as_int());
		}
		cases.push_back(__case);
	}
}

std::vector<City> cities;
std::string default_str_cities;
/* Загрузка городов из xml файлов */
void load_cities() {
	default_str_cities = "";
	pugi::xml_document doc_cities;
	pugi::xml_node child_, object_, file_;
	file_ = XMLLoad(LIB_MISC CITIES_FILE, "cities", "Error loading cases file: cities.xml", doc_cities);
	for (child_ = file_.child("city"); child_; child_ = child_.next_sibling("city")) {
		City city;
		city.name = child_.child("name").attribute("value").as_string();
		city.rent_vnum = child_.child("rent_vnum").attribute("value").as_int();
		for (object_ = child_.child("ZoneVnum"); object_; object_ = object_.next_sibling("ZoneVnum")) {
			city.vnums.push_back(object_.attribute("value").as_int());
		}
		cities.push_back(city);
		default_str_cities += "0";
	}
}

std::vector<RandomObj> random_objs;

// загрузка параметров для рандомных шмоток
void load_random_obj() {
	pugi::xml_document doc_robj;
	pugi::xml_node robj, object, file_robj;
	int tmp_buf2;
	std::string tmp_str;
	file_robj = XMLLoad(LIB_MISC RANDOMOBJ_FILE, "object", "Error loading cases file: random_obj.xml", doc_robj);
	for (robj = file_robj.child("obj"); robj; robj = robj.next_sibling("obj")) {
		RandomObj tmp_robj;
		tmp_robj.vnum = robj.attribute("vnum").as_int();
		for (object = robj.child("not_to_wear"); object; object = object.next_sibling("not_to_wear")) {
			tmp_str = object.attribute("value").as_string();
			tmp_buf2 = object.attribute("drop_chance").as_int();
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
		for (object = robj.child("affect"); object; object = object.next_sibling("affect")) {
			tmp_str = object.attribute("value").as_string();
			tmp_buf2 = object.attribute("drop_chance").as_int();
			tmp_robj.affects.insert(std::pair<std::string, int>(tmp_str, tmp_buf2));
		}
		for (object = robj.child("extraaffect"); object; object = object.next_sibling("extraaffect")) {
			ExtraAffects extraAffect;
			extraAffect.number = object.attribute("value").as_int();
			extraAffect.chance = object.attribute("drop_chance").as_int();
			extraAffect.min_val = object.attribute("min_val").as_int();
			extraAffect.max_val = object.attribute("max_val").as_int();
			tmp_robj.extraffect.push_back(extraAffect);
		}
		random_objs.push_back(tmp_robj);
	}
}

QuestBodrich::QuestBodrich() {
	this->load_objs();
	this->load_mobs();
	this->load_rewards();
}

void QuestBodrich::load_objs() {
	pugi::xml_document doc_;
	pugi::xml_node class_, file_, object_;
	file_ = XMLLoad(LIB_MISC QUESTBODRICH_FILE, "objects", "Error loading obj file: quest_bodrich.xml", doc_);
	std::vector<int> tmp_array;
	for (class_ = file_.child("class"); class_; class_ = class_.next_sibling("class")) {
		tmp_array.clear();
		for (object_ = object_.child("obj"); object_; object_ = object_.next_sibling("obj")) {
			tmp_array.push_back(object_.attribute("vnum").as_int());
		}
		this->objs.insert(std::pair<int, std::vector<int>>(class_.attribute("id").as_int(), tmp_array));
	}
}

void QuestBodrich::load_mobs() {
	pugi::xml_document doc_;
	pugi::xml_node level_, file_, mob_;
	file_ = XMLLoad(LIB_MISC QUESTBODRICH_FILE, "mobs", "Error loading mobs file: quest_bodrich.xml", doc_);
	std::vector<int> tmp_array;
	for (level_ = file_.child("level"); level_; level_ = level_.next_sibling("level")) {
		tmp_array.clear();
		for (mob_ = mob_.child("mob"); mob_; mob_ = mob_.next_sibling("mob")) {
			tmp_array.push_back(mob_.attribute("vnum").as_int());
		}
		this->mobs.insert(std::pair<int, std::vector<int>>(level_.attribute("value").as_int(), tmp_array));
	}
}

void QuestBodrich::load_rewards() {
	pugi::xml_document doc_;
	pugi::xml_node class_, file_, object_, level_;
	file_ = XMLLoad(LIB_MISC QUESTBODRICH_FILE, "rewards", "Error loading rewards file: quest_bodrich.xml", doc_);
	std::vector<QuestBodrichRewards> tmp_array;
	for (class_ = file_.child("class"); class_; class_ = class_.next_sibling("class")) {
		tmp_array.clear();
		for (level_ = level_.child("level"); level_; level_ = level_.next_sibling("level")) {
			QuestBodrichRewards qbr;
			qbr.level = level_.attribute("level").as_int();
			qbr.vnum = level_.attribute("ObjVnum").as_int();
			qbr.money = level_.attribute("money_value").as_int();
			qbr.exp = level_.attribute("exp_value").as_int();
			tmp_array.push_back(qbr);
		}
		//std::map<int, QuestBodrichRewards> rewards;
		this->rewards.insert(std::pair<int, std::vector<QuestBodrichRewards>>(class_.attribute("id").as_int(),
																			  tmp_array));
	}
}

void load_dquest() {
	log("Loading daily quests...");
	DailyQuest::load_from_file();
}

/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
void do_reboot(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	argument = one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0) {
			prune_crlf(GREETINGS);
		}
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
		ObjData::init_set_table();
		load_mobraces();
		load_morphs();
		GlobalDrop::init();
		OfftopSystem::init();
		Celebrates::load();
		HelpSystem::reload_all();
		Remort::init();
		Noob::init();
		stats_reset::init();
		Bonus::bonus_log_load();
		DailyQuest::load_from_file();
	} else if (!str_cmp(arg, "portals"))
		init_portals();
	else if (!str_cmp(arg, "abilities")) {
		MUD::CfgManager().ReloadCfg("abilities");
	} else if (!str_cmp(arg, "skills")) {
		MUD::CfgManager().ReloadCfg("skills");
	} else if (!str_cmp(arg, "spells")) {
		MUD::CfgManager().ReloadCfg("spells");
	} else if (!str_cmp(arg, "feats")) {
		MUD::CfgManager().ReloadCfg("feats");
	} else if (!str_cmp(arg, "classes")) {
		MUD::CfgManager().ReloadCfg("classes");
	} else if (!str_cmp(arg, "guilds")) {
		MUD::CfgManager().ReloadCfg("guilds");
	} else if (!str_cmp(arg, "currencies")) {
		MUD::CfgManager().ReloadCfg("currencies");
	} else if (!str_cmp(arg, "imagic"))
		init_im();
	else if (!str_cmp(arg, "ztypes"))
		init_zone_types();
	else if (!str_cmp(arg, "oloadtable"))
		oload_table.init();
	else if (!str_cmp(arg, "setstuff")) {
		ObjData::init_set_table();
		HelpSystem::reload(HelpSystem::STATIC);
	} else if (!str_cmp(arg, "immlist"))
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
	else if (!str_cmp(arg, "greetings")) {
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
	} else if (!str_cmp(arg, "xhelp")) {
		HelpSystem::reload_all();
	} else if (!str_cmp(arg, "socials"))
		go_boot_socials();
	else if (!str_cmp(arg, "schedule"))
		load_sheduled_reboot();
	else if (!str_cmp(arg, "clan")) {
		skip_spaces(&argument);
		if (!*argument) {
			Clan::ClanLoad();
			return;
		}
		Clan::ClanListType::iterator clan;
		std::string buffer(argument);
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
			if (CompareParam(buffer, (*clan)->get_abbrev())) {
				CreateFileName(buffer);
				Clan::ClanReload(buffer);
				SendMsgToChar("Перезагрузка клана.\r\n", ch);
				break;
			}
		}
	} else if (!str_cmp(arg, "proxy"))
		LoadProxyList();
	else if (!str_cmp(arg, "boards"))
		Boards::Static::reload_all();
	else if (!str_cmp(arg, "titles"))
		TitleSystem::load_title_list();
	else if (!str_cmp(arg, "emails"))
		RegisterSystem::load();
	else if (!str_cmp(arg, "privilege"))
		privilege::Load();
	else if (!str_cmp(arg, "mobraces"))
		load_mobraces();
	else if (!str_cmp(arg, "morphs"))
		load_morphs();
	else if (!str_cmp(arg, "depot") && PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		skip_spaces(&argument);
		if (*argument) {
			long uid = GetUniqueByName(std::string(argument));
			if (uid > 0) {
				Depot::reload_char(uid, ch);
			} else {
				SendMsgToChar("Указанный чар не найден\r\n"
							 "Формат команды: reload depot <имя чара>.\r\n", ch);
			}
		} else {
			SendMsgToChar("Формат команды: reload depot <имя чара>.\r\n", ch);
		}
	} else if (!str_cmp(arg, "globaldrop")) {
		GlobalDrop::init();
	} else if (!str_cmp(arg, "offtop")) {
		OfftopSystem::init();
	} else if (!str_cmp(arg, "shop")) {
		ShopExt::load(true);
	} else if (!str_cmp(arg, "named")) {
		NamedStuff::load();
	} else if (!str_cmp(arg, "celebrates")) {
		//Celebrates::load(XMLLoad(LIB_MISC CELEBRATES_FILE, CELEBRATES_MAIN_TAG, CELEBRATES_ERROR_STR));
		Celebrates::load();
	} else if (!str_cmp(arg, "setsdrop") && PRF_FLAGGED(ch, EPrf::kCoderinfo)) {
		skip_spaces(&argument);
		if (*argument && is_number(argument)) {
			SetsDrop::reload(atoi(argument));
		} else {
			SetsDrop::reload();
		}
	} else if (!str_cmp(arg, "remort.xml")) {
		Remort::init();
	} else if (!str_cmp(arg, "noob_help.xml")) {
		Noob::init();
	} else if (!str_cmp(arg, "reset_stats.xml")) {
		stats_reset::init();
	} else if (!str_cmp(arg, "obj_sets.xml")) {
		obj_sets::load();
	} else if (!str_cmp(arg, "daily")) {
		DailyQuest::load_from_file(ch);
	} else {
		SendMsgToChar("Неверный параметр для перезагрузки файлов.\r\n", ch);
		return;
	}

	std::string str = fmt::format("{} reload {}.", ch->get_name(), arg);
	mudlog(str.c_str(), NRM, kLvlImmortal, SYSLOG, true);

	SendMsgToChar(OK, ch);
}

void init_portals(void) {
	FILE *portal_f;
	char nm[300], nm2[300], *wrd;
	int rnm = 0, i, level = 0;
	struct Portal *curr, *prev;

	// Сначала освобождаем все порталы
	for (curr = portals_list; curr; curr = prev) {
		prev = curr->next;
		free(curr->wrd);
		free(curr);
	}

	portals_list = nullptr;
	prev = nullptr;

	// читаем файл
	if (!(portal_f = fopen(LIB_MISC "portals.lst", "r"))) {
		log("Can not open portals.lst");
		return;
	}

	while (get_line(portal_f, nm)) {
		if (!nm[0] || nm[0] == ';')
			continue;
		sscanf(nm, "%d %d %s", &rnm, &level, nm2);
		if (real_room(rnm) == kNowhere || nm2[0] == '\0') {
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
		curr->next = nullptr;
		if (!portals_list)
			portals_list = curr;
		else
			prev->next = curr;

		prev = curr;

	}
	fclose(portal_f);
}

/// конверт поля GET_OBJ_SKILL в емкостях TODO: 12.2013
int convert_drinkcon_skill(CObjectPrototype *obj, bool proto) {
	if (GET_OBJ_SKILL(obj) > 0
		&& (GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer
			|| GET_OBJ_TYPE(obj) == EObjType::kFountain)) {
		log("obj_skill: %d - %s (%d)", GET_OBJ_SKILL(obj),
			GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_VNUM(obj));
		// если емскости уже просетили какие-то заклы, то зелье
		// из обж-скилл их не перекрывает, а просто удаляется
		if (obj->GetPotionValueKey(ObjVal::EValueKey::POTION_PROTO_VNUM) < 0) {
			const auto potion = world_objects.create_from_prototype_by_vnum(GET_OBJ_SKILL(obj));
			if (potion
				&& GET_OBJ_TYPE(potion) == EObjType::kPotion) {
				drinkcon::copy_potion_values(potion.get(), obj);
				if (proto) {
					// copy_potion_values сетит до кучи и внум из пошена,
					// поэтому уточним здесь, что зелье не перелито
					// емкости из read_one_object_new идут как перелитые
					obj->SetPotionValueKey(ObjVal::EValueKey::POTION_PROTO_VNUM, 0);
				}
			}
		}
		obj->set_skill(to_underlying(ESkill::kUndefined));

		return 1;
	}
	return 0;
}

/// конверт параметров прототипов ПОСЛЕ лоада всех файлов с прототипами
void convert_obj_values() {
	int save = 0;
	for (const auto &i : obj_proto) {
		save = std::max(save, convert_drinkcon_skill(i.get(), true));
		if (i->has_flag(EObjFlag::k1inlaid)) {
			i->unset_extraflag(EObjFlag::k1inlaid);
			save = 1;
		}
		if (i->has_flag(EObjFlag::k2inlaid)) {
			i->unset_extraflag(EObjFlag::k2inlaid);
			save = 1;
		}
		if (i->has_flag(EObjFlag::k3inlaid)) {
			i->unset_extraflag(EObjFlag::k3inlaid);
			save = 1;
		}
		// ...
		if (save) {
			olc_add_to_save_list(i->get_vnum() / 100, OLC_SAVE_OBJ);
			save = 0;
		}
	}
}

void GameLoader::boot_world() {
	utils::CSteppedProfiler boot_profiler("World booting", 1.1);

	boot_profiler.next_step("Loading zone table");
	log("Loading zone table.");
	world_loader.index_boot(DB_BOOT_ZON);

	boot_profiler.next_step("Loading triggers");
	log("Loading triggers and generating index.");
	world_loader.index_boot(DB_BOOT_TRG);

	boot_profiler.next_step("Loading rooms");
	log("Loading rooms.");
	world_loader.index_boot(DB_BOOT_WLD);

	boot_profiler.next_step("Adding virtual rooms to all zones");
	log("Adding virtual rooms to all zones.");
	add_vrooms_to_all_zones();

	boot_profiler.next_step("Renumbering rooms");
	log("Renumbering rooms.");
	renum_world();

	boot_profiler.next_step("Checking start rooms");
	log("Checking start rooms.");
	check_start_rooms();

	boot_profiler.next_step("Loading mobs and regerating index");
	log("Loading mobs and generating index.");
	world_loader.index_boot(DB_BOOT_MOB);

	boot_profiler.next_step("Counting mob's levels");
	log("Count mob quantity by level");
	MobMax::init();

	boot_profiler.next_step("Loading objects");
	log("Loading objs and generating index.");
	world_loader.index_boot(DB_BOOT_OBJ);

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

void init_zone_types() {
	FILE *zt_file;
	char tmp[1024], dummy[128], name[128], itype_num[128];
	int names = 0;
	int i, j, k, n;

	if (zone_types != nullptr) {
		for (i = 0; *zone_types[i].name != '\n'; i++) {
			if (zone_types[i].ingr_qty > 0)
				free(zone_types[i].ingr_types);

			free(zone_types[i].name);
		}
		free(zone_types[i].name);
		free(zone_types);
		zone_types = nullptr;
	}

	zt_file = fopen(LIB_MISC "ztypes.lst", "r");
	if (!zt_file) {
		log("Can not open ztypes.lst");
		return;
	}

	while (get_line(zt_file, tmp)) {
		if (!strn_cmp(tmp, "ИМЯ", 3)) {
			if (sscanf(tmp, "%s %s", dummy, name) != 2) {
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!get_line(zt_file, tmp)) {
				log("Corrupted file : ztypes.lst");
				return;
			}
			if (!strn_cmp(tmp, "ТИПЫ", 4)) {
				if (tmp[4] != ' ' && tmp[4] != '\0') {
					log("Corrupted file : ztypes.lst");
					return;
				}
				for (i = 4; tmp[i] != '\0'; i++) {
					if (!a_isdigit(tmp[i]) && !a_isspace(tmp[i])) {
						log("Corrupted file : ztypes.lst");
						return;
					}
				}
			} else {
				log("Corrupted file : ztypes.lst");
				return;
			}
			names++;
		} else {
			log("Corrupted file : ztypes.lst");
			return;
		}
	}
	names++;

	CREATE(zone_types, names);
	for (i = 0; i < names; i++) {
		zone_types[i].name = nullptr;
		zone_types[i].ingr_qty = 0;
		zone_types[i].ingr_types = nullptr;
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp)) {
		sscanf(tmp, "%s %s", dummy, name);
		for (j = 0; name[j] != '\0'; j++) {
			if (name[j] == '_') {
				name[j] = ' ';
			}
		}
		zone_types[i].name = str_dup(name);

		get_line(zt_file, tmp);
		for (j = 4; tmp[j] != '\0'; j++) {
			if (a_isspace(tmp[j]))
				continue;
			zone_types[i].ingr_qty++;
			for (; tmp[j] != '\0' && a_isdigit(tmp[j]); j++);
			j--;
		}
		i++;
	}
	zone_types[i].name = str_dup("\n");

	for (i = 0; *zone_types[i].name != '\n'; i++) {
		if (zone_types[i].ingr_qty > 0) {
			CREATE(zone_types[i].ingr_types, zone_types[i].ingr_qty);
		}
	}

	rewind(zt_file);
	i = 0;
	while (get_line(zt_file, tmp)) {
		get_line(zt_file, tmp);
		for (j = 4, n = 0; tmp[j] != '\0'; j++) {
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

void ObjData::init_set_table() {
	std::string cppstr, tag;
	int mode = SETSTUFF_SNUM;
	id_to_set_info_map::iterator snum;
	set_info::iterator vnum;
	qty_to_camap_map::iterator oqty;
	class_to_act_map::iterator clss;
	int appnum = 0;

	ObjData::set_table.clear();

	std::ifstream fp(LIB_MISC "setstuff.lst");

	if (!fp) {
		cppstr = "init_set_table:: Unable open input file 'lib/misc/setstuff.lst'";
		mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
		return;
	}

	while (!fp.eof()) {
		std::istringstream isstream;

		getline(fp, cppstr);

		std::string::size_type i = cppstr.find(';');
		if (i != std::string::npos)
			cppstr.erase(i);
		reverse(cppstr.begin(), cppstr.end());
		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof()) {
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		reverse(cppstr.begin(), cppstr.end());
		isstream.clear();

		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof()) {
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		isstream.clear();

		if (cppstr.empty())
			continue;

		if (cppstr[0] == '#') {
			if (mode != SETSTUFF_SNUM && mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS
				&& mode != SETSTUFF_AFCN) {
				cppstr = "init_set_table:: Wrong position of line '" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				continue;
			}

			cppstr.erase(cppstr.begin());

			if (cppstr.empty() || !a_isdigit(cppstr[0])) {
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', expected set id after #";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				continue;
			}

			int tmpsnum;

			isstream.str(cppstr);
			isstream >> std::noskipws >> tmpsnum;

			if (!isstream.eof()) {
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', expected only set id after #";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				continue;
			}

			std::pair<id_to_set_info_map::iterator, bool>
				p = ObjData::set_table.insert(std::make_pair(tmpsnum, set_info()));

			if (!p.second) {
				cppstr = "init_set_table:: Error in line '#" + cppstr + "', this set already exists";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				continue;
			}

			snum = p.first;
			mode = SETSTUFF_NAME;
			continue;
		}

		if (cppstr.size() < 5 || cppstr[4] != ':') {
			cppstr = "init_set_table:: Format error in line '" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
			continue;
		}

		tag = cppstr.substr(0, 4);
		cppstr.erase(0, 5);
		isstream.str(cppstr);
		cppstr.erase();
		isstream >> std::skipws >> cppstr;
		if (!isstream.eof()) {
			std::string suffix;

			getline(isstream, suffix);
			cppstr += suffix;
		}
		isstream.clear();

		if (cppstr.empty()) {
			cppstr = "init_set_table:: Empty parameter field in line '" + tag + ":" + cppstr + "'";
			mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
			continue;
		}

		switch (tag[0]) {
			case 'A':
				if (tag == "Amsg") {
					if (mode != SETSTUFF_AMSG) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_actmsg(cppstr);
					mode = SETSTUFF_DMSG;
				} else if (tag == "Affs") {
					if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					isstream.str(cppstr);
					cppstr.erase();
					isstream >> std::skipws >> cppstr;
					if (!isstream.eof()) {
						std::string suffix;

						getline(isstream, suffix);
						cppstr += suffix;
						cppstr =
							"init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object affects";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					FlagData tmpaffs = clear_flags;
					tmpaffs.from_string(cppstr.c_str());

					clss->second.set_affects(tmpaffs);
					appnum = 0;
					mode = SETSTUFF_AFCN;
				} else if (tag == "Afcn") {
					if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					isstream.str(cppstr);

					int tmploc, tmpmodi;

					if (!(isstream >> std::skipws >> tmploc >> std::skipws >> tmpmodi)) {
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr
							+ "', expected apply location and modifier";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					obj_affected_type tmpafcn(static_cast<EApply>(tmploc), (sbyte) tmpmodi);

					if (!isstream.eof()) {
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr
							+ "', expected only apply location and modifier";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					if (tmpafcn.location <= EApply::kNone || tmpafcn.location >= EApply::kNumberApplies) {
						cppstr = "init_set_table:: Wrong apply location in line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					if (!tmpafcn.modifier) {
						cppstr = "init_set_table:: Wrong apply modifier in line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					if (mode == SETSTUFF_AMSG || mode == SETSTUFF_AFFS)
						appnum = 0;

					if (appnum >= kMaxObjAffect) {
						cppstr = "init_set_table:: Too many applies - line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					} else
						clss->second.set_affected_i(appnum++, tmpafcn);

					mode = SETSTUFF_AFCN;
				} else if (tag == "Alis") {
					if (mode != SETSTUFF_ALIS) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					snum->second.set_alias(cppstr);
					mode = SETSTUFF_VNUM;
				} else {
					cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'C':
				if (tag == "Clss") {
					if (mode != SETSTUFF_CLSS && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS
						&& mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					unique_bit_flag_data tmpclss;

					if (cppstr == "all")
						tmpclss.set_all();
					else {
						isstream.str(cppstr);

						int i = 0;

						while (isstream >> std::skipws >> i)
							if (i < 0 || i > kNumPlayerClasses * kNumKins) {
								break;
							} else {
								tmpclss.set(flag_data_by_num(i));
							}

						if (i < 0 || i > kNumPlayerClasses * kNumKins) {
							cppstr = "init_set_table:: Wrong class in line '" + tag + ":" + cppstr + "'";
							mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
							continue;
						}

						if (!isstream.eof()) {
							cppstr =
								"init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only class ids";
							mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
							continue;
						}
					}

					std::pair<class_to_act_map::iterator, bool>
						p = oqty->second.insert(std::make_pair(tmpclss, activation()));

					if (!p.second) {
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr +
							"', each class number can occur only once for each object number";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss = p.first;
					mode = SETSTUFF_AMSG;
				} else {
					cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'D':
				if (tag == "Dmsg") {
					if (mode != SETSTUFF_DMSG) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_deactmsg(cppstr);
					mode = SETSTUFF_RAMG;
				} else if (tag == "Dice") {
					if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					isstream.str(cppstr);

					int ndices, nsides;

					if (!(isstream >> std::skipws >> ndices >> std::skipws >> nsides)) {
						cppstr =
							"init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected ndices and nsides";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_dices(ndices, nsides);
				} else {
					cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'N':
				if (tag == "Name") {
					if (mode != SETSTUFF_NAME) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					snum->second.set_name(cppstr);
					mode = SETSTUFF_ALIS;
				} else {
					cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'O':
				if (tag == "Oqty") {
					if (mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS
						&& mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					unsigned int tmpoqty = 0;
					isstream.str(cppstr);
					isstream >> std::skipws >> tmpoqty;

					if (!isstream.eof()) {
						cppstr =
							"init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object number";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					if (!tmpoqty || tmpoqty > EEquipPos::kNumEquipPos) {
						cppstr = "init_set_table:: Wrong object number in line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					std::pair<qty_to_camap_map::iterator, bool>
						p = vnum->second.insert(std::make_pair(tmpoqty, class_to_act_map()));

					if (!p.second) {
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr +
							"', each object number can occur only once for each object";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					oqty = p.first;
					mode = SETSTUFF_CLSS;
				} else {
					cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'R':
				if (tag == "Ramg") {
					if (mode != SETSTUFF_RAMG) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_room_actmsg(cppstr);
					mode = SETSTUFF_RDMG;
				} else if (tag == "Rdmg") {
					if (mode != SETSTUFF_RDMG) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_room_deactmsg(cppstr);
					mode = SETSTUFF_AFFS;
				} else {
					cppstr = "init_set_table:: Format error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'S':
				if (tag == "Skll") {
					if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					isstream.str(cppstr);

					int skillnum, percent;

					if (!(isstream >> std::skipws >> skillnum >> std::skipws >> percent)) {
						cppstr =
							"init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected ndices and nsides";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_skill(static_cast<ESkill>(skillnum), percent);
				} else {
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			case 'V':
				if (tag == "Vnum") {
					if (mode != SETSTUFF_ALIS
						&& mode != SETSTUFF_VNUM
						&& mode != SETSTUFF_OQTY
						&& mode != SETSTUFF_AMSG
						&& mode != SETSTUFF_AFFS
						&& mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					ObjVnum tmpvnum = -1;
					isstream.str(cppstr);
					isstream >> std::skipws >> tmpvnum;

					if (!isstream.eof()) {
						cppstr =
							"init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected only object vnum";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					if (real_object(tmpvnum) < 0) {
						cppstr = "init_set_table:: Wrong object vnum in line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					id_to_set_info_map::iterator it;

					for (it = ObjData::set_table.begin(); it != ObjData::set_table.end(); it++)
						if (it->second.find(tmpvnum) != it->second.end())
							break;

					if (it != ObjData::set_table.end()) {
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr
							+ "', object can exist only in one set";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					vnum = snum->second.insert(std::make_pair(tmpvnum, qty_to_camap_map())).first;
					mode = SETSTUFF_OQTY;
				} else {
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;
			case 'W':
				if (tag == "Wght") {
					if (mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS && mode != SETSTUFF_AFCN) {
						cppstr = "init_set_table:: Wrong position of line '" + tag + ":" + cppstr + "'";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					isstream.str(cppstr);

					int weight;

					if (!(isstream >> std::skipws >> weight)) {
						cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "', expected item weight";
						mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
						continue;
					}

					clss->second.set_weight(weight);
				} else {
					cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
					mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
				}
				break;

			default: cppstr = "init_set_table:: Error in line '" + tag + ":" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
		}
	}

	if (mode != SETSTUFF_SNUM && mode != SETSTUFF_OQTY && mode != SETSTUFF_AMSG && mode != SETSTUFF_AFFS
		&& mode != SETSTUFF_AFCN) {
		cppstr = "init_set_table:: Last set was deleted, because of unexpected end of file";
		ObjData::set_table.erase(snum);
		mudlog(cppstr.c_str(), LGH, kLvlImmortal, SYSLOG, true);
	}
}

void set_zone_mob_level() {
	for (std::size_t i = 0; i < zone_table.size(); ++i) {
		int level = 0;
		int count = 0;

		for (int nr = 0; nr <= top_of_mobt; ++nr) {
			if (mob_index[nr].vnum >= zone_table[i].vnum * 100
				&& mob_index[nr].vnum <= zone_table[i].vnum * 100 + 99) {
				level += mob_proto[nr].GetLevel();
				++count;
			}
		}

		zone_table[i].mob_level = count ? level / count : 0;
	}
}

void set_zone_town() {
	for (ZoneRnum i = 0; i < static_cast<ZoneRnum>(zone_table.size()); ++i) {
		zone_table[i].is_town = false;
		int rnum_start = 0, rnum_end = 0;
		if (!GetZoneRooms(i, &rnum_start, &rnum_end)) {
			continue;
		}

		bool rent_flag = false, bank_flag = false, post_flag = false;
		// зона считается городом, если в ней есть рентер, банкир и почтовик
		for (int k = rnum_start; k <= rnum_end; ++k) {
			for (const auto ch : world[k]->people) {
				if (IS_RENTKEEPER(ch)) {
					rent_flag = true;
				} else if (IS_BANKKEEPER(ch)) {
					bank_flag = true;
				} else if (IS_POSTKEEPER(ch)) {
					post_flag = true;
				}
			}
		}

		if (rent_flag && bank_flag && post_flag) {
			zone_table[i].is_town = true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
namespace {

struct snoop_node {
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
bool can_snoop(CharData *imm, CharData *vict) {
	if (!CLAN(vict)) {
		return true;
	}

	for (ClanMailType::const_iterator i = clan_list.begin(),
			 iend = clan_list.end(); i != iend; ++i) {
		std::set<std::string>::const_iterator k = i->second.find(GET_EMAIL(imm));
		if (k != i->second.end() && CLAN(vict)->CheckPolitics(i->first) == kPoliticsWar) {
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void load_messages() {
	FILE *fl;
	int i, type;
	struct AttackMsgSet *messages;
	char chk[128];

	if (!(fl = fopen(MESS_FILE, "r"))) {
		log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
		exit(1);
	}
	for (i = 0; i < kMaxMessages; i++) {
		fight_messages[i].attack_type = 0;
		fight_messages[i].number_of_attacks = 0;
		fight_messages[i].msg_set = nullptr;
	}

	const char *dummyc = fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
		dummyc = fgets(chk, 128, fl);
	}

	while (*chk == 'M') {
		dummyc = fgets(chk, 128, fl);

		int dummyi = sscanf(chk, " %d\n", &type);
		UNUSED_ARG(dummyi);

		for (i = 0; (i < kMaxMessages) &&
			(fight_messages[i].attack_type != type) && (fight_messages[i].attack_type); i++);
		if (i >= kMaxMessages) {
			log("SYSERR: Too many combat messages.  Increase kMaxMessages and recompile.");
			exit(1);
		}
		//log("BATTLE MESSAGE %d(%d)", i, type); Лишний спам
		CREATE(messages, 1);
		fight_messages[i].number_of_attacks++;
		fight_messages[i].attack_type = type;
		messages->next = fight_messages[i].msg_set;
		fight_messages[i].msg_set = messages;

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
		while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
			dummyc = fgets(chk, 128, fl);
		}
	}
	UNUSED_ARG(dummyc);

	fclose(fl);
}
void ZoneTrafficSave() {
	pugi::xml_document doc;
	doc.append_child().set_name("zone_traffic");
	pugi::xml_node node_list = doc.child("zone_traffic");
	pugi::xml_node time_node = node_list.append_child();
	time_node.set_name("time");
	time_node.append_attribute("date") = static_cast<unsigned long long>(zones_stat_date);

	for (auto i = 0u; i < zone_table.size(); ++i) {
		pugi::xml_node zone_node = node_list.append_child();
		zone_node.set_name("zone");
		zone_node.append_attribute("vnum") = zone_table[i].vnum;
		zone_node.append_attribute("traffic") = zone_table[i].traffic;
	}

	doc.save_file(ZONE_TRAFFIC_FILE);
}
void zone_traffic_load() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(ZONE_TRAFFIC_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("zone_traffic");
	pugi::xml_node time_node = node_list.child("time");
	pugi::xml_attribute xml_date = time_node.attribute("date");
	if (xml_date) {
		zones_stat_date = static_cast<time_t>(xml_date.as_ullong());
	}
	for (pugi::xml_node node = node_list.child("zone"); node; node = node.next_sibling("zone")) {
		const int zone_vnum = atoi(node.attribute("vnum").value());
		ZoneRnum zrn;
		for (zrn = 0; zone_table[zrn].vnum != zone_vnum && zrn < static_cast<ZoneRnum>(zone_table.size()); zrn++) {
			/* empty loop */
		}
		int num = atoi(node.attribute("traffic").value());
		if (zrn >= static_cast<ZoneRnum>(zone_table.size())) {
			snprintf(buf,
					 kMaxStringLength,
					 "zone_traffic: несуществующий номер зоны %d ее траффик %d ",
					 zone_vnum,
					 num);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			continue;
		}
		zone_table[zrn].traffic = atoi(node.attribute("traffic").value());
	}
}

// body of the booting system
void boot_db(void) {
	utils::CSteppedProfiler boot_profiler("MUD booting", 1.1);

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
	MKLETTERS(plrstuff / depot);
	MKDIR("plrstuff/house");
	MKDIR("stat");

#undef MKLETTERS
#undef MKDIR

	boot_profiler.next_step("Initialization of text IDs");
	log("Init TextId list.");
	text_id::Init();

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

	boot_profiler.next_step("Loading currencies cfg.");
	log("Loading currencies cfg.");
	MUD::CfgManager().LoadCfg("currencies");

	boot_profiler.next_step("Loading skills cfg.");
	log("Loading skills cfg.");
	MUD::CfgManager().LoadCfg("skills");

	boot_profiler.next_step("Loading feats cfg.");
	log("Loading feats cfg.");
	MUD::CfgManager().LoadCfg("feats");

	boot_profiler.next_step("Loading spells cfg.");
	log("Loading spells cfg.");
	MUD::CfgManager().LoadCfg("spells");

	boot_profiler.next_step("Loading abilities definitions");
	log("Loading abilities.");
	MUD::CfgManager().LoadCfg("abilities");

	pugi::xml_document doc;

	load_dquest();
	boot_profiler.next_step("Loading criterion");
	log("Loading Criterion...");
	pugi::xml_document doc1;
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "finger", "Error Loading Criterion.xml: <finger>", doc1),
				   EWearFlag::kFinger);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "neck", "Error Loading Criterion.xml: <neck>", doc1),
				   EWearFlag::kNeck);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "body", "Error Loading Criterion.xml: <body>", doc1),
				   EWearFlag::kBody);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "head", "Error Loading Criterion.xml: <head>", doc1),
				   EWearFlag::kHead);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "legs", "Error Loading Criterion.xml: <legs>", doc1),
				   EWearFlag::kLegs);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "feet", "Error Loading Criterion.xml: <feet>", doc1),
				   EWearFlag::kFeet);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "hands", "Error Loading Criterion.xml: <hands>", doc1),
				   EWearFlag::kHands);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "arms", "Error Loading Criterion.xml: <arms>", doc1),
				   EWearFlag::kArms);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "shield", "Error Loading Criterion.xml: <shield>", doc1),
				   EWearFlag::kShield);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "about", "Error Loading Criterion.xml: <about>", doc1),
				   EWearFlag::kShoulders);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "waist", "Error Loading Criterion.xml: <waist>", doc1),
				   EWearFlag::kWaist);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "waist", "Error Loading Criterion.xml: <quiver>", doc1),
				   EWearFlag::kQuiver);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "wrist", "Error Loading Criterion.xml: <wrist>", doc1),
				   EWearFlag::kWrist);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "boths", "Error Loading Criterion.xml: <boths>", doc1),
				   EWearFlag::kBoth);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "wield", "Error Loading Criterion.xml: <wield>", doc1),
				   EWearFlag::kWield);
	Load_Criterion(XMLLoad(LIB_MISC CRITERION_FILE, "hold", "Error Loading Criterion.xml: <hold>", doc1),
				   EWearFlag::kHold);

	boot_profiler.next_step("Loading birthplaces definitions");
	log("Loading birthplaces definitions.");
	Birthplaces::Load(XMLLoad(LIB_MISC BIRTH_PLACES_FILE, BIRTH_PLACE_MAIN_TAG, BIRTH_PLACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading player races definitions");
	log("Loading player races definitions.");
	PlayerRace::Load(XMLLoad(LIB_MISC PLAYER_RACE_FILE, RACE_MAIN_TAG, PLAYER_RACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading ingredients magic");
	log("Booting IM");
	init_im();

	boot_profiler.next_step("Assigning character classs info.");
	log("Assigning character classs info.");
	MUD::CfgManager().LoadCfg("classes");

	InitSpellLevels();

	boot_profiler.next_step("Loading zone types and ingredient for each zone type");
	log("Booting zone types and ingredient types for each zone type.");
	init_zone_types();

	boot_profiler.next_step("Loading insert_wanted.lst");
	log("Booting insert_wanted.lst.");
	iwg.init();

	boot_profiler.next_step("Loading gurdians");
	log("Load guardians.");
	LoadGuardians();

	boot_profiler.next_step("Loading world");
	world_loader.boot_world();

	boot_profiler.next_step("Loading stuff load table");
	log("Booting stuff load table.");
	oload_table.init();

	boot_profiler.next_step("Loading setstuff table");
	log("Booting setstuff table.");
	ObjData::init_set_table();

	boot_profiler.next_step("Loading item levels");
	log("Init item levels.");
	ObjSystem::init_item_levels();

	boot_profiler.next_step("Loading help entries");
	log("Loading help entries.");
	world_loader.index_boot(DB_BOOT_HLP);

	boot_profiler.next_step("Loading social entries");
	log("Loading social entries.");
	world_loader.index_boot(DB_BOOT_SOCIAL);

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
	if (!no_specials) {
		log("   Mobiles.");
		assign_mobiles();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}

	boot_profiler.next_step("Reading skills variables.");
	log("Reading skills variables.");
	InitMiningVars();
	InitJewelryVars();

	boot_profiler.next_step("Sorting command list");
	log("Sorting command list.");
	sort_commands();

	boot_profiler.next_step("Reading banned site, proxy, privileges and invalid-name list.");
	log("Reading banned site, proxy, privileges and invalid-name list.");
	ban = new BanList();
	Read_Invalid_List();

	boot_profiler.next_step("Loading rented objects info");
	log("Booting rented objects info");
	for (std::size_t i = 0; i < player_table.size(); i++) {
		player_table[i].timer = nullptr;
		Crash_read_timer(i, false);
	}

	// последовательность лоада кланов/досок не менять
	boot_profiler.next_step("Loading boards");
	log("Booting boards");
	Boards::Static::BoardInit();

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
	if (grouping.init()) {
		exit(1);
	}

	boot_profiler.next_step("Loading special assignments");
	log("Booting special assignment");
	init_spec_procs();

	boot_profiler.next_step("Assigning guilds info.");
	log("Assigning guilds info.");
	MUD::CfgManager().LoadCfg("guilds");

	boot_profiler.next_step("Loading portals for 'town portal' spell");
	log("Booting portals for 'town portal' spell");
	portals_list = nullptr;
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
	NewNames::load();

	boot_profiler.next_step("Loading global UID timer");
	log("Load global uid counter");
	LoadGlobalUID();

	boot_profiler.next_step("Initializing DT list");
	log("Init DeathTrap list.");
	deathtrap::load();

	boot_profiler.next_step("Loading titles list");
	log("Load Title list.");
	TitleSystem::load_title_list();

	boot_profiler.next_step("Loading emails list");
	log("Load registered emails list.");
	RegisterSystem::load();

	boot_profiler.next_step("Loading privileges and gods list");
	log("Load privilege and god list.");
	privilege::Load();

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
	for (ZoneRnum i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
		log("Resetting %s (rooms %d-%d).", zone_table[i].name,
			(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
		reset_zone(i);
	}
	reset_q.head = reset_q.tail = nullptr;

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

	log("Load zone traffic.");
	zone_traffic_load();

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
	boot_profiler.next_step("Loading crafts system");
	log("Starting crafts system.");
	if (!crafts::start())
	{
		log("ERROR: Failed to start crafts system.\n");
	}
	*/

	boot_profiler.next_step("Loading big sets in rent");
	log("Check big sets in rent.");
	SetSystem::check_rented();

	// сначала сеты, стата мобов, потом дроп сетов
	boot_profiler.next_step("Loading object sets/mob_stat/drop_sets lists");
	obj_sets::load();
	log("Load mob_stat.xml");
	mob_stat::Load();
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
	stats_reset::init();

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
	load_speedwalk();

	boot_profiler.next_step("Loading cities cfg");
	log("Loading cities cfg.");
	load_cities();
	shutdown_parameters.mark_boot_time();
	log("Boot db -- DONE.");

}

// reset the time in the game from file
void reset_time(void) {
	time_info = *mud_time_passed(time(0), beginning_of_time);
	// Calculate moon day
	weather_info.moon_day =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kMoonCycle;
	weather_info.week_day_mono =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kWeekCycle;
	weather_info.week_day_poly =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kPolyWeekCycle;
	// Calculate Easter
	calc_easter();

	if (time_info.hours < sunrise[time_info.month][0])
		weather_info.sunlight = kSunDark;
	else if (time_info.hours == sunrise[time_info.month][0])
		weather_info.sunlight = kSunRise;
	else if (time_info.hours < sunrise[time_info.month][1])
		weather_info.sunlight = kSunLight;
	else if (time_info.hours == sunrise[time_info.month][1])
		weather_info.sunlight = kSunSet;
	else
		weather_info.sunlight = kSunDark;

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);

	weather_info.temperature = (year_temp[time_info.month].med * 4 +
		number(year_temp[time_info.month].min, year_temp[time_info.month].max)) / 5;
	weather_info.pressure = 960;
	if ((time_info.month >= EMonth::kMay) && (time_info.month <= EMonth::kNovember))
		weather_info.pressure += RollDices(1, 50);
	else
		weather_info.pressure += RollDices(1, 80);

	weather_info.change = 0;
	weather_info.weather_type = 0;

	if (time_info.month >= EMonth::kApril && time_info.month <= EMonth::kMay)
		weather_info.season = ESeason::kSpring;
	else if (time_info.month == EMonth::kMarch && weather_info.temperature >= 3)
		weather_info.season = ESeason::kSpring;
	else if (time_info.month >= EMonth::kJune && time_info.month <= EMonth::kAugust)
		weather_info.season = ESeason::kSummer;
	else if (time_info.month >= EMonth::kSeptember && time_info.month <= EMonth::kOctober)
		weather_info.season = ESeason::kAutumn;
	else if (time_info.month == EMonth::kNovember && weather_info.temperature >= 3)
		weather_info.season = ESeason::kAutumn;
	else
		weather_info.season = ESeason::kWinter;

	if (weather_info.pressure <= 980)
		weather_info.sky = kSkyLightning;
	else if (weather_info.pressure <= 1000) {
		weather_info.sky = kSkyRaining;
		if (time_info.month >= EMonth::kApril && time_info.month <= EMonth::kOctober)
			create_rainsnow(&weather_info.weather_type, kWeatherLightrain, 40, 40, 20);
		else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary)
			create_rainsnow(&weather_info.weather_type, kWeatherLightsnow, 50, 40, 10);
		else if (time_info.month == EMonth::kNovember || time_info.month == EMonth::kMarch) {
			if (weather_info.temperature >= 3)
				create_rainsnow(&weather_info.weather_type, kWeatherLightrain, 70, 30, 0);
			else
				create_rainsnow(&weather_info.weather_type, kWeatherLightsnow, 80, 20, 0);
		}
	} else if (weather_info.pressure <= 1020)
		weather_info.sky = kSkyCloudy;
	else
		weather_info.sky = kSkyCloudless;
}

// generate index table for the player file
void build_player_index(void) {
	new_build_player_index();
	return;
}

void GameLoader::index_boot(const EBootType mode) {
	log("Index booting %d", mode);

	auto index = IndexFileFactory::get_index(mode);
	if (!index->open()) {
		exit(1);
	}
	const int rec_count = index->load();

	prepare_global_structures(mode, rec_count);

	const auto data_file_factory = DataFileFactory::create();
	for (const auto &entry: *index) {
		auto data_file = data_file_factory->get_file(mode, entry);
		if (!data_file->open()) {
			continue;    // TODO: we need to react somehow.
		}
		if (!data_file->load()) {
			// TODO: do something
		}
		data_file->close();
	}

	// sort the social index
	if (mode == DB_BOOT_SOCIAL) {
		qsort(soc_keys_list, number_of_social_commands, sizeof(struct SocialKeyword), csort);
	}
}

void GameLoader::prepare_global_structures(const EBootType mode, const int rec_count) {
	// * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	switch (mode) {
		case DB_BOOT_TRG: CREATE(trig_index, rec_count);
			break;

		case DB_BOOT_WLD: {
			// Creating empty world with kNowhere room.
			world.push_back(new RoomData);
			top_of_world = kFirstRoom;
			const size_t rooms_bytes = sizeof(RoomData) * rec_count;
			log("   %d rooms, %zd bytes.", rec_count, rooms_bytes);
		}
			break;

		case DB_BOOT_MOB: {
			mob_proto = new CharData[rec_count]; // TODO: переваять на вектор (+в medit)
			CREATE(mob_index, rec_count);
			const size_t index_size = sizeof(IndexData) * rec_count;
			const size_t characters_size = sizeof(CharData) * rec_count;
			log("   %d mobs, %zd bytes in index, %zd bytes in prototypes.", rec_count, index_size, characters_size);
		}
			break;

		case DB_BOOT_OBJ:
			log("   %d objs, ~%zd bytes in index, ~%zd bytes in prototypes.",
				rec_count,
				obj_proto.index_size(),
				obj_proto.prototypes_size());
			break;

		case DB_BOOT_ZON: {
			zone_table.resize(rec_count);
			const size_t zones_size = sizeof(ZoneData) * rec_count;
			log("   %d zones, %zd bytes.", rec_count, zones_size);
		}
			break;

		case DB_BOOT_HLP: break;

		case DB_BOOT_SOCIAL: {
			CREATE(soc_mess_list, number_of_social_messages);
			CREATE(soc_keys_list, number_of_social_commands);
			const size_t messages_size = sizeof(struct SocialMessages) * (number_of_social_messages);
			const size_t keywords_size = sizeof(struct SocialKeyword) * (number_of_social_commands);
			log("   %d entries(%d keywords), %zd(%zd) bytes.", number_of_social_messages,
				number_of_social_commands, messages_size, keywords_size);
		}
			break;
	}
}

// * Проверки всяких несочетаемых флагов на комнатах.
void check_room_flags(int rnum) {
	if (deathtrap::IsSlowDeathtrap(rnum)) {
		// снятие номагик и прочих флагов, запрещающих чару выбраться из комнаты без выходов при наличии медленного дт
		world[rnum]->unset_flag(ERoomFlag::kNoMagic);
		world[rnum]->unset_flag(ERoomFlag::kNoTeleportOut);
		world[rnum]->unset_flag(ERoomFlag::kNoSummonOut);
	}
	if (world[rnum]->get_flag(ERoomFlag::kHouse)
		&& (SECT(rnum) == ESector::kMountain || SECT(rnum) == ESector::kHills)) {
		// шоб в замках умные не копали
		SECT(rnum) = ESector::kInside;
	}
}

// make sure the start rooms exist & resolve their vnums to rnums
void check_start_rooms(void) {
	if ((r_mortal_start_room = real_room(mortal_start_room)) == kNowhere) {
		log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d", mortal_start_room);
		exit(1);
	}

	if ((r_immort_start_room = real_room(immort_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}

	if ((r_frozen_start_room = real_room(frozen_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}

	if ((r_helled_start_room = real_room(helled_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
		r_helled_start_room = r_mortal_start_room;
	}

	if ((r_named_start_room = real_room(named_start_room)) == kNowhere) {
		log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
		r_named_start_room = r_mortal_start_room;
	}

	if ((r_unreg_start_room = real_room(unreg_start_room)) == kNowhere) {
		log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
		r_unreg_start_room = r_mortal_start_room;
	}
}

void add_vrooms_to_all_zones() {
	for (auto it = zone_table.begin(); it < zone_table.end(); ++it) {
		const auto first_room = it->vnum * 100;
		const auto last_room = first_room + 99;

		const RoomVnum virtual_room_vnum = (it->vnum * 100) + 99;
		const auto vroom_it = std::find_if(world.cbegin(), world.cend(), [virtual_room_vnum](RoomData *room) {
			return room->room_vn == virtual_room_vnum;
		});
		if (vroom_it != world.cend()) {
			log("Zone %d already contains virtual room.", it->vnum);
			continue;
		}

		const ZoneRnum rnum = std::distance(zone_table.begin(), it);

		// ищем место для вставки новой комнаты с конца, чтобы потом не вычитать 1 из результата
		auto insert_reverse_position = std::find_if(world.rbegin(), world.rend(), [rnum](RoomData *room) {
			return rnum >= room->zone_rn;
		});
		auto insert_position = (insert_reverse_position == world.rend()) ? world.begin() : insert_reverse_position.base();

		top_of_world++;
		RoomData *new_room = new RoomData;
		world.insert(insert_position, new_room);
		new_room->zone_rn = rnum;
		new_room->room_vn = last_room;
		new_room->set_name(std::string("Виртуальная комната"));
		new_room->description_num = RoomDescription::add_desc(std::string("Похоже, здесь вам делать нечего."));
		new_room->clear_flags();
		new_room->sector_type = ESector::kSecret;

		new_room->affected_by.clear();
		memset(&new_room->base_property, 0, sizeof(RoomState));
		memset(&new_room->add_property, 0, sizeof(RoomState));
		new_room->func = nullptr;
		new_room->contents = nullptr;
		new_room->track = nullptr;
		new_room->light = 0;
		new_room->fires = 0;
		new_room->gdark = 0;
		new_room->glight = 0;
		new_room->proto_script.reset(new ObjData::triggers_list_t());
	}
}

// resolve all vnums into rnums in the world
void renum_world(void) {
	int room, door;

	for (room = kFirstRoom; room <= top_of_world; room++) {
		for (door = 0; door < EDirection::kMaxDirNum; door++) {
			if (world[room]->dir_option[door]) {
				if (world[room]->dir_option[door]->to_room() != kNowhere) {
					const auto to_room = real_room(world[room]->dir_option[door]->to_room());

					world[room]->dir_option[door]->to_room(to_room);
				}
			}
		}
	}
}

// Установка принадлежности к зоне в прототипах
void renum_obj_zone(void) {
	for (size_t i = 0; i < obj_proto.size(); ++i) {
		obj_proto.zone(i, get_zone_rnum_by_obj_vnum(obj_proto[i]->get_vnum()));
	}
}

// Установкапринадлежности к зоне в индексе
void renum_mob_zone(void) {
	int i;
	for (i = 0; i <= top_of_mobt; ++i) {
		mob_index[i].zone = get_zone_rnum_by_mob_vnum(mob_index[i].vnum);
	}
}

#define ZCMD_CMD(cmd_nom) zone_table[zone].cmd[cmd_nom]
#define ZCMD zone_table[zone].cmd[cmd_no]

void renum_single_table(int zone) {
	int cmd_no, a, b, c, olda, oldb, oldc;
	char buf[128];

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
		a = b = c = 0;
		olda = ZCMD.arg1;
		oldb = ZCMD.arg2;
		oldc = ZCMD.arg3;
		switch (ZCMD.command) {
			case 'M': a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			case 'F': a = ZCMD.arg1 = real_room(ZCMD.arg1);
				b = ZCMD.arg2 = real_mobile(ZCMD.arg2);
				c = ZCMD.arg3 = real_mobile(ZCMD.arg3);
				break;
			case 'O': a = ZCMD.arg1 = real_object(ZCMD.arg1);
				if (ZCMD.arg3 != kNowhere)
					c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			case 'G': a = ZCMD.arg1 = real_object(ZCMD.arg1);
				break;
			case 'E': a = ZCMD.arg1 = real_object(ZCMD.arg1);
				break;
			case 'P': a = ZCMD.arg1 = real_object(ZCMD.arg1);
				c = ZCMD.arg3 = real_object(ZCMD.arg3);
				break;
			case 'D': a = ZCMD.arg1 = real_room(ZCMD.arg1);
				break;
			case 'R':    // rem obj from room
				a = ZCMD.arg1 = real_room(ZCMD.arg1);
				b = ZCMD.arg2 = real_object(ZCMD.arg2);
				break;
			case 'Q': a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
				break;
			case 'T':    // a trigger
				// designer's choice: convert this later
				// b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
				b = real_trigger(ZCMD.arg2);    // leave this in for validation
				if (ZCMD.arg1 == WLD_TRIGGER)
					c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			case 'V':    // trigger variable assignment
				if (ZCMD.arg1 == WLD_TRIGGER)
					b = ZCMD.arg2 = real_room(ZCMD.arg2);
				break;
		}
		if (a < 0 || b < 0 || c < 0) {
			sprintf(buf, "Invalid vnum %d, cmd disabled", (a < 0) ? olda : ((b < 0) ? oldb : oldc));
			log_zone_error(zone, cmd_no, buf);
			ZCMD.command = '*';
		}
	}
}

// resove vnums into rnums in the zone reset tables
void renum_zone_table(void) {
	for (ZoneRnum zone = 0; zone < static_cast<ZoneRnum>(zone_table.size()); zone++) {
		renum_single_table(zone);
	}
}

// Make own name by process aliases
int trans_obj_name(ObjData *obj, CharData *ch) {
	// ищем метку @p , @p1 ... и заменяем на падежи.
	int i, k;
	for (i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
		std::string obj_pad = GET_OBJ_PNAME(obj_proto[GET_OBJ_RNUM(obj)], i);
		size_t j = obj_pad.find("@p");
		if (std::string::npos != j && 0 < j) {
			// Родитель найден прописываем его.
			k = atoi(obj_pad.substr(j + 2, j + 3).c_str());
			obj_pad.replace(j, 3, GET_PAD(ch, k));

			obj->set_PName(i, obj_pad);
			// Если имя в именительном то дублируем запись
			if (i == 0) {
				obj->set_short_description(obj_pad);
				obj->set_aliases(obj_pad); // ставим алиасы
			}
		}
	};
	obj->set_is_rename(true); // присвоим флажок что у шмотки заменены падежи
	return (true);
}

void dl_list_copy(OnDeadLoadList **pdst, OnDeadLoadList *src) {
	OnDeadLoadList::iterator p;
	if (src == nullptr) {
		*pdst = nullptr;
		return;
	} else {
		*pdst = new OnDeadLoadList;
		p = src->begin();
		while (p != src->end()) {
			(*pdst)->push_back(*p);
			p++;
		}
	}
}

// Dead load list object load
int dl_load_obj(ObjData *corpse, CharData *ch, CharData *chr, int DL_LOAD_TYPE) {
	bool last_load = true;
	bool load = false;
	bool miw;
	OnDeadLoadList::iterator p;

	if (mob_proto[GET_MOB_RNUM(ch)].dl_list == nullptr)
		return false;

	p = mob_proto[GET_MOB_RNUM(ch)].dl_list->begin();

	while (p != mob_proto[GET_MOB_RNUM(ch)].dl_list->end()) {
		switch ((*p)->load_type) {
			case DL_LOAD_ANYWAY: last_load = load;
			case DL_LOAD_ANYWAY_NC: break;
			case DL_LOAD_IFLAST: last_load = load && last_load;
				break;
			case DL_LOAD_IFLAST_NC: load = load && last_load;
				break;
		}
		// Блокируем лоад в зависимости от значения смецпараметра
		if ((*p)->spec_param != DL_LOAD_TYPE)
			load = false;
		else
			load = true;
		if (load) {
			const auto tobj = world_objects.create_from_prototype_by_vnum((*p)->obj_vnum);
			if (!tobj) {
				sprintf(buf, "Попытка загрузки в труп (VNUM:%d) несуществующего объекта (VNUM:%d).",
						GET_MOB_VNUM(ch), (*p)->obj_vnum);
				mudlog(buf, NRM, kLvlBuilder, ERRLOG, true);
			} else {
				// Проверяем мах_ин_ворлд и вероятность загрузки, если это необходимо для такого DL_LOAD_TYPE
				if (GET_OBJ_MIW(tobj) >= obj_proto.actual_count(tobj->get_rnum())
					|| GET_OBJ_MIW(tobj) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
					|| check_unlimited_timer(tobj.get())) {
					miw = true;
				} else {
					miw = false;
				}

				switch (DL_LOAD_TYPE) {
					case DL_ORDINARY:    //Обычная загрузка - без выкрутасов
						if (miw && (number(1, 100) <= (*p)->load_prob))
							load = true;
						else
							load = false;
						break;

					case DL_PROGRESSION:    //Загрузка с убывающей до 0.01 вероятностью
						if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
							load = true;
						else
							load = false;
						break;

					case DL_SKIN:    //Загрузка по применению "освежевать"
						if ((miw && (number(1, 100) <= (*p)->load_prob)) || (number(1, 100) <= 1))
							load = true;
						else
							load = false;
						if (chr == nullptr)
							load = false;
						break;
				}
				if (load) {
					tobj->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));
					tobj->set_parent(GET_MOB_VNUM(ch));
					if (DL_LOAD_TYPE == DL_SKIN) {
						trans_obj_name(tobj.get(), ch);
					}
					// Добавлена проверка на отсутствие трупа
					if (MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
						act("На земле остал$U лежать $o.", false, ch, tobj.get(), 0, kToRoom);
						PlaceObjToRoom(tobj.get(), ch->in_room);
					} else {
						if ((DL_LOAD_TYPE == DL_SKIN) && (corpse->get_carried_by() == chr)) {
							can_carry_obj(chr, tobj.get());
						} else {
							PlaceObjIntoObj(tobj.get(), corpse);
						}
					}
				} else {
					ExtractObjFromWorld(tobj.get());
					load = false;
				}

			}
		}
		p++;
	}

	return true;
}

// Dead load list object parse
int dl_parse(OnDeadLoadList **dl_list, char *line) {
	// Формат парсинга D {номер прототипа} {вероятность загрузки} {спец поле - тип загрузки}
	int vnum, prob, type, spec;
	struct LoadingItem *new_item;

	if (sscanf(line, "%d %d %d %d", &vnum, &prob, &type, &spec) != 4) {
		// Ошибка чтения.
		log("SYSERR: Parse death load list (bad param count).");
		return false;
	};
	// проверяем существование прототипа в мире (предметы уже должны быть загружены)
	if (*dl_list == nullptr) {
		// Создаем новый список.
		*dl_list = new OnDeadLoadList;
	}

	CREATE(new_item, 1);
	new_item->obj_vnum = vnum;
	new_item->load_prob = prob;
	new_item->load_type = type;
	new_item->spec_param = spec;

	(*dl_list)->push_back(new_item);
	return true;
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

int calc_boss_value(CharData *mob, int num) {
	if (mob->get_role(MOB_ROLE_BOSS)) {
		num += num * 25 / 100;
	}
	return num;
}

void set_test_data(CharData *mob) {
	if (!mob) {
		log("SYSERROR: null mob (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (GET_EXP(mob) == 0) {
		return;
	}

	if (GetRealLevel(mob) <= 50) {
		if (GET_EXP(mob) > test_levels[49]) {
			// log("test1: %s - %d -> %d", mob->get_name(), mob->get_level(), 50);
			mob->set_level(50);
		} else {
			if (mob->GetLevel() == 0) {
				for (int i = 0; i < 50; ++i) {
					if (test_levels[i] >= GET_EXP(mob)) {
						// log("test2: %s - %d -> %d", mob->get_name(), mob->get_level(), i + 1);
						mob->set_level(i + 1);

						// -10..-86
						int min_save = -(10 + 4 * (mob->GetLevel() - 31));
						min_save = calc_boss_value(mob, min_save);

						for (auto s = ESaving::kFirst; s <= ESaving::kLast; ++s) {
							if (GetSave(mob, s) > min_save) {
								SetSave(mob, s, min_save);
							}
						}
						// 20..77
						int min_cast = 20 + 3 * (mob->GetLevel() - 31);
						min_cast = calc_boss_value(mob, min_cast);

						if (GET_CAST_SUCCESS(mob) < min_cast) {
							//log("test4: %s - %d -> %d", mob->get_name(), GET_CAST_SUCCESS(mob), min_cast);
							GET_CAST_SUCCESS(mob) = min_cast;
						}

						int min_absorbe = calc_boss_value(mob, mob->GetLevel());
						if (GET_ABSORBE(mob) < min_absorbe) {
							GET_ABSORBE(mob) = min_absorbe;
						}

						break;
					}
				}
			}
		}
	}
}

int csort(const void *a, const void *b) {
	const struct SocialKeyword *a1, *b1;

	a1 = (const struct SocialKeyword *) a;
	b1 = (const struct SocialKeyword *) b;

	return (str_cmp(a1->keyword, b1->keyword));
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/



int vnum_mobile(char *searchname, CharData *ch) {
	int nr, found = 0;

	for (nr = 0; nr <= top_of_mobt; nr++) {
		if (isname(searchname, mob_proto[nr].GetCharAliases())) {
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum, mob_proto[nr].get_npc_name().c_str());
			SendMsgToChar(buf, ch);
		}
	}
	return (found);
}

int vnum_object(char *searchname, CharData *ch) {
	int found = 0;

	for (size_t nr = 0; nr < obj_proto.size(); nr++) {
		if (isname(searchname, obj_proto[nr]->get_aliases())) {
			++found;
			sprintf(buf, "%3d. [%5d] %s\r\n",
					found, obj_proto[nr]->get_vnum(),
					obj_proto[nr]->get_short_description().c_str());
			SendMsgToChar(buf, ch);
		}
	}
	return (found);
}

int vnum_flag(char *searchname, CharData *ch) {
	int found = 0, plane = 0, counter = 0, plane_offset = 0;
	bool f = false;
	std::string out;

// ---------------------- extra_bits
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*extra_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		};
		if (utils::IsAbbr(searchname, extra_bits[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->has_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s\r\n",
						++found, i->get_vnum(), 
						utils::RemoveColors(i->get_short_description().c_str()).c_str(), 
						extra_bits[counter]);
				out += buf;
			}
		}
	}
// --------------------- apply_types
	f = false;
	for (counter = 0; *apply_types[counter] != '\n'; counter++) {
		if (utils::IsAbbr(searchname, apply_types[counter])) {
			f = true;
			break;
		}
	}
	if (f) {
		for (const auto &i : obj_proto) {
			for (plane = 0; plane < kMaxObjAffect; plane++) {
				if (i->get_affected(plane).location == static_cast<EApply>(counter)) {
					snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s, значение: %d\r\n",
							 ++found, i->get_vnum(),
							 utils::RemoveColors(i->get_short_description().c_str()).c_str(),
							 apply_types[counter], i->get_affected(plane).modifier);
					out += buf;
					continue;
				}
			}
		}
	}
// --------------------- weapon affects
	f = false;
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*weapon_affects[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		};
		if (utils::IsAbbr(searchname, weapon_affects[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description().c_str()).c_str(),
						 weapon_affects[counter]);
				out += buf;
			}
		}
	}
// --------------------- anti_bits
	f = false;
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*anti_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		};
		if (utils::IsAbbr(searchname, anti_bits[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : запрещен для: %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description().c_str()).c_str(),
						 anti_bits[counter]);
				out += buf;
			}
		}
	}
// --------------------- no_bits
	f = false;
	for (counter = 0, plane = 0, plane_offset = 0; plane < NUM_PLANES; counter++) {
		if (*no_bits[counter] == '\n') {
			plane++;
			plane_offset = 0;
			continue;
		};
		if (utils::IsAbbr(searchname, no_bits[counter])) {
			f = true;
			break;
		}
		plane_offset++;
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->get_affect_flags().get_flag(plane, 1 << plane_offset)) {
				snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : неудобен для: %s\r\n",
						 ++found, i->get_vnum(),
						 utils::RemoveColors(i->get_short_description().c_str()).c_str(),
						 no_bits[counter]);
				out += buf;
			}
		}
	}
//--------------------------------- skills
	f = false;
	ESkill skill_id;
	for (skill_id = ESkill::kFirst; skill_id <= ESkill::kLast; ++skill_id) {
		if (utils::IsAbbr(searchname, MUD::Skill(skill_id).GetName())) {
			f = true;
			break;
		}
	}
	if (f) {
		for (const auto &i : obj_proto) {
			if (i->has_skills()) {
				auto it = i->get_skills().find(skill_id);
				if (it != i->get_skills().end()) {
					snprintf(buf, kMaxStringLength, "%3d. [%7d] %60s : %s, значение: %d\r\n",
							 ++found, i->get_vnum(),
							 utils::RemoveColors(i->get_short_description().c_str()).c_str(),
							 MUD::Skill(skill_id).GetName(), it->second);
					out += buf;
				}
			}
		}
	}
	if (!out.empty()) {
		page_string(ch->desc, out);
	}
	return found;
}

int vnum_room(char *searchname, CharData *ch) {
	int nr, found = 0;

	for (nr = 0; nr <= top_of_world; nr++) {
		if (isname(searchname, world[nr]->name)) {
			sprintf(buf, "%3d. [%7d] %s\r\n", ++found, world[nr]->room_vn, world[nr]->name);
			SendMsgToChar(buf, ch);
		}
	}
	return found;
}

int vnum_obj_trig(char *searchname, CharData *ch) {
	int num;
	if ((num = atoi(searchname)) == 0) {
		return 0;
	}

	const auto trigger = obj2triggers.find(num);
	if (trigger == obj2triggers.end()) {
		return 0;
	}

	int found = 0;
	for (const auto &t : trigger->second) {
		TrgRnum rnum = real_trigger(t);
		sprintf(buf, "%3d. [%5d] %s\r\n", ++found, trig_index[rnum]->vnum, trig_index[rnum]->proto->get_name().c_str());
		SendMsgToChar(buf, ch);
	}

	return found;
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

int get_test_hp(int lvl) {
	if (lvl > 0 && lvl <= 50) {
		return test_hp[lvl - 1];
	}
	return 1;
}

// create a new mobile from a prototype
CharData *read_mobile(MobVnum nr, int type) {                // and MobRnum
	int is_corpse = 0;
	MobRnum i;

	if (nr < 0) {
		is_corpse = 1;
		nr = -nr;
	}

	if (type == VIRTUAL) {
		if ((i = real_mobile(nr)) < 0) {
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (nullptr);
		}
	} else {
		i = nr;
	}

	auto *mob = new CharData(mob_proto[i]); //чет мне кажется что конструкции типа этой не принесут нам щастья...
	mob->set_normal_morph();
	mob->proto_script.reset(new ObjData::triggers_list_t());
	mob->script.reset(new Script());    //fill it in assign_triggers from proto_script
	character_list.push_front(mob);

	if (!mob->points.max_hit) {
		mob->points.max_hit = std::max(1, RollDices(mob->mem_queue.total, mob->mem_queue.stored) + mob->points.hit);
	} else {
		mob->points.max_hit = std::max(1, number(mob->points.hit, mob->mem_queue.total));
	}

	int test_hp = get_test_hp(GetRealLevel(mob));
	if (GET_EXP(mob) > 0 && mob->points.max_hit < test_hp) {
//		log("hp: (%s) %d -> %d", GET_NAME(mob), mob->points.max_hit, test_hp);
		mob->points.max_hit = test_hp;
	}

	mob->points.hit = mob->points.max_hit;
	mob->mem_queue.total = mob->mem_queue.stored = 0;
	GET_HORSESTATE(mob) = 800;
	GET_LASTROOM(mob) = kNowhere;
	if (mob->mob_specials.speed <= -1)
		GET_ACTIVITY(mob) = number(0, kPulseMobile - 1);
	else
		GET_ACTIVITY(mob) = number(0, mob->mob_specials.speed);
	mob->extract_timer = 0;
	mob->points.move = mob->points.max_move;
	mob->add_gold(RollDices(GET_GOLD_NoDs(mob), GET_GOLD_SiDs(mob)));

	mob->player_data.time.birth = time(nullptr);
	mob->player_data.time.played = 0;
	mob->player_data.time.logon = time(nullptr);

	mob->id = max_id.allocate();

	if (!is_corpse) {
		std::vector<long> list_idnum;
		if (mob_id_by_vnum.contains(GET_MOB_VNUM(mob)))
			list_idnum = mob_id_by_vnum[GET_MOB_VNUM(mob)];
		list_idnum.push_back(mob->id);
		mob_id_by_vnum[GET_MOB_VNUM(mob)] = list_idnum;
		mob_index[i].total_online++;
		assign_triggers(mob, MOB_TRIGGER);
	} else {
		MOB_FLAGS(mob).set(EMobFlag::kSummoned);
	}
	mob_by_uid[mob->id] = mob;
	i = mob_index[i].zone;
	if (i != -1 && zone_table[i].under_construction) {
		// mobile принадлежит тестовой зоне
		MOB_FLAGS(mob).set(EMobFlag::kNoSummon);
	}

	auto it = guardian_list.find(GET_MOB_VNUM(mob));
	if (it != guardian_list.end()) {
		MOB_FLAGS(mob).set(EMobFlag::kCityGuardian);
	}

	return (mob);
}

/**
// никакая это не копия, строковые и остальные поля с выделением памяти остаются общими
// мы просто отдаем константный указатель на прототип
 * \param type по дефолту VIRTUAL
 */
CObjectPrototype::shared_ptr get_object_prototype(ObjVnum nr, int type) {
	unsigned i = nr;
	if (type == VIRTUAL) {
		const int rnum = real_object(nr);
		if (rnum < 0) {
			log("Object (V) %d does not exist in database.", nr);
			return nullptr;
		} else {
			i = rnum;
		}
	}

	if (i > obj_proto.size()) {
		return nullptr;
	}
	return obj_proto[i];
}

void after_reset_zone(ZoneRnum nr_zone) {
	for (auto d = descriptor_list; d; d = d->next) {
		// Чар должен быть в игре
		if (STATE(d) == CON_PLAYING) {
			if (world[d->character->in_room]->zone_rn == nr_zone) {
				zone_table[nr_zone].used = true;
				return;
			}
			struct FollowerType *k, *k_next;
			for (k = d->character->followers; k; k = k_next) {
				k_next = k->next;
				if (IS_CHARMICE(k->follower) && world[k->follower->in_room]->zone_rn == nr_zone) {
					zone_table[nr_zone].used = true;
					return;
				}
			}
		}
	}
}

#define ZO_DEAD  9999

// update zone ages, queue for reset if necessary, and dequeue when possible
void zone_update(void) {
	int k = 0;
	struct reset_q_element *update_u, *temp;
	static int timer = 0;
	utils::CExecutionTimer timer_count;
	if (((++timer * kPulseZone) / kPassesPerSec) >= 60)    // one minute has passed
	{
		/*
		 * NOT accurate unless kPulseZone is a multiple of kPassesPerSec or a
		 * factor of 60
		 */
		timer = 0;
		for (std::size_t i = 0; i < zone_table.size(); i++) {
			if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode &&
				(zone_table[i].reset_idle || zone_table[i].used))
				(zone_table[i].age)++;
			if (zone_table[i].age >= zone_table[i].lifespan &&
				zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode &&
				(zone_table[i].reset_idle || zone_table[i].used)) {
				CREATE(update_u, 1);
				update_u->zone_to_reset = static_cast<ZoneRnum>(i);
				update_u->next = nullptr;
				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else {
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}
				zone_table[i].age = ZO_DEAD;
			}
		}
	}
	std::vector<ZoneRnum> zone_repop_list;
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if (zone_table[update_u->zone_to_reset].reset_mode == 2
			|| (zone_table[update_u->zone_to_reset].reset_mode != 3 && is_empty(update_u->zone_to_reset))
			|| can_be_reset(update_u->zone_to_reset)) {
			zone_repop_list.push_back(update_u->zone_to_reset);
			std::stringstream out;
			out << "Auto zone reset: " << zone_table[update_u->zone_to_reset].name << " ("
					<<  zone_table[update_u->zone_to_reset].vnum << ")";
			if (zone_table[update_u->zone_to_reset].reset_mode == 3) {
				for (auto i = 0; i < zone_table[update_u->zone_to_reset].typeA_count; i++) {
					//Ищем ZoneRnum по vnum
					for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
						if (zone_table[j].vnum ==
							zone_table[update_u->zone_to_reset].typeA_list[i]) {
							zone_repop_list.push_back(j);
							out << " ]\r\n[ Also resetting: " << zone_table[j].name << " ("
									<<  zone_table[j].vnum << ")";
							break;
						}
					}
				}
			}
			std::stringstream ss;
			RepopDecay(zone_repop_list);
			ss << "В списке репопа: ";
			for (auto it = zone_repop_list.begin(); it != zone_repop_list.end(); ++it) {
				ss << zone_table[*it].vnum << " ";
				reset_zone(*it);
			}
			mudlog(ss.str(), LGH, kLvlGod, SYSLOG, false);
			out << " ]\r\n[ Time reset: " << timer_count.delta().count();
			mudlog(out.str(), LGH, kLvlGod, SYSLOG, false);
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else {
				for (temp = reset_q.head; temp->next != update_u; temp = temp->next);
				if (!update_u->next)
					reset_q.tail = temp;
				temp->next = update_u->next;
			}
			free(update_u);
			k++;
			if (k >= kZonesReset)
				break;
		}
}

bool can_be_reset(ZoneRnum zone) {
	if (zone_table[zone].reset_mode != 3)
		return false;
// проверяем себя
	if (!is_empty(zone))
		return false;
// проверяем список B
	for (auto i = 0; i < zone_table[zone].typeB_count; i++) {
		//Ищем ZoneRnum по vnum
		for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
			if (zone_table[j].vnum == zone_table[zone].typeB_list[i]) {
				if (!zone_table[zone].typeB_flag[i] || !is_empty(j)) {
					return false;
				}
				break;
			}
		}
	}
// проверяем список A
	for (auto i = 0; i < zone_table[zone].typeA_count; i++) {
		//Ищем ZoneRnum по vnum
		for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
			if (zone_table[j].vnum == zone_table[zone].typeA_list[i]) {
				if (!is_empty(j)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

void paste_mob(CharData *ch, RoomRnum room) {
	if (!ch->IsNpc() || ch->GetEnemy() || GET_POS(ch) < EPosition::kStun)
		return;
	if (IS_CHARMICE(ch)
		|| AFF_FLAGGED(ch, EAffect::kHorse)
		|| AFF_FLAGGED(ch, EAffect::kHold)
		|| (ch->extract_timer > 0)) {
		return;
	}
//	if (MOB_FLAGGED(ch, MOB_CORPSE))
//		return;
	if (room == kNowhere)
		return;

	bool time_ok = false;
	bool month_ok = false;
	bool need_move = false;
	bool no_month = true;
	bool no_time = true;

	if (MOB_FLAGGED(ch, EMobFlag::kAppearsDay)) {
		if (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight)
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (MOB_FLAGGED(ch, EMobFlag::kAppearsNight)) {
		if (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (MOB_FLAGGED(ch, EMobFlag::kAppearsFullmoon)) {
		if ((weather_info.sunlight == kSunSet ||
			weather_info.sunlight == kSunDark) &&
			(weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (MOB_FLAGGED(ch, EMobFlag::kAppearsWinter)) {
		if (weather_info.season == ESeason::kWinter)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (MOB_FLAGGED(ch, EMobFlag::kAppearsSpring)) {
		if (weather_info.season == ESeason::kSpring)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (MOB_FLAGGED(ch, EMobFlag::kAppearsSummer)) {
		if (weather_info.season == ESeason::kSummer)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (MOB_FLAGGED(ch, EMobFlag::kAppearsAutumn)) {
		if (weather_info.season == ESeason::kAutumn)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (need_move) {
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok) {
			if (world[room]->room_vn != zone_table[world[room]->zone_rn].top)
				return;

			if (GET_LASTROOM(ch) == kNowhere) {
				ExtractCharFromWorld(ch, false, true);
				return;
			}

			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, real_room(GET_LASTROOM(ch)));
		} else {
			if (world[room]->room_vn == zone_table[world[room]->zone_rn].top)
				return;

			GET_LASTROOM(ch) = GET_ROOM_VNUM(room);
			RemoveCharFromRoom(ch);
			room = real_room(zone_table[world[room]->zone_rn].top);

			if (room == kNowhere)
				room = real_room(GET_LASTROOM(ch));

			PlaceCharToRoom(ch, room);
		}
	}
}

void paste_obj(ObjData *obj, RoomRnum room) {
	if (obj->get_carried_by()
		|| obj->get_worn_by()
		|| room == kNowhere) {
		return;
	}

	bool time_ok = false;
	bool month_ok = false;
	bool need_move = false;
	bool no_time = true;
	bool no_month = true;

	if (obj->has_flag(EObjFlag::kAppearsDay)) {
		if (weather_info.sunlight == kSunRise
			|| weather_info.sunlight == kSunLight) {
			time_ok = true;
		}
		need_move = true;
		no_time = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsNight)) {
		if (weather_info.sunlight == kSunSet
			|| weather_info.sunlight == kSunDark) {
			time_ok = true;
		}
		need_move = true;
		no_time = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsFullmoon)) {
		if ((weather_info.sunlight == kSunSet
			|| weather_info.sunlight == kSunDark)
			&& weather_info.moon_day >= 12
			&& weather_info.moon_day <= 15) {
			time_ok = true;
		}
		need_move = true;
		no_time = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsWinter)) {
		if (weather_info.season == ESeason::kWinter) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsSpring)) {
		if (weather_info.season == ESeason::kSpring) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsSummer)) {
		if (weather_info.season == ESeason::kSummer) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsAutumn)) {
		if (weather_info.season == ESeason::kAutumn) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (need_move) {
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok) {
			if (world[room]->room_vn != zone_table[world[room]->zone_rn].top) {
				return;
			}
			if (OBJ_GET_LASTROOM(obj) == kNowhere) {
				ExtractObjFromWorld(obj);
				return;
			}
			RemoveObjFromRoom(obj);
			PlaceObjToRoom(obj, real_room(OBJ_GET_LASTROOM(obj)));
		} else {
			if (world[room]->room_vn == zone_table[world[room]->zone_rn].top) {
				return;
			}
			// зачем сезонные переносить в виртуалку? спуржить нафиг
			if (!month_ok) {
				ExtractObjFromWorld(obj);
				return;
			}
			obj->set_room_was_in(GET_ROOM_VNUM(room));
			RemoveObjFromRoom(obj);
			room = real_room(zone_table[world[room]->zone_rn].top);
			if (room == kNowhere) {
				room = real_room(OBJ_GET_LASTROOM(obj));
			}
			PlaceObjToRoom(obj, room);
		}
	}
}

void paste_mobiles() {
	character_list.foreach_on_copy([](const CharData::shared_ptr &character) {
		paste_mob(character.get(), character->in_room);
	});

	world_objects.foreach_on_copy([](const ObjData::shared_ptr &object) {
		paste_obj(object.get(), object->get_in_room());
	});
}

void paste_on_reset(RoomData *to_room) {
	const auto people_copy = to_room->people;
	for (const auto ch : people_copy) {
		paste_mob(ch, ch->in_room);
	}

	ObjData *obj_next;
	for (ObjData *obj = to_room->contents; obj; obj = obj_next) {
		obj_next = obj->get_next_content();
		paste_obj(obj, obj->get_in_room());
	}
}

void log_zone_error(ZoneRnum zone, int cmd_no, const char *message) {
	char buf[256];

	sprintf(buf, "SYSERR: zone file %d.zon: %s", zone_table[zone].vnum, message);
	mudlog(buf, NRM, kLvlGod, SYSLOG, true);

	sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
			ZCMD.command, zone_table[zone].vnum, ZCMD.line);
	mudlog(buf, NRM, kLvlGod, SYSLOG, true);
}

void process_load_celebrate(Celebrates::CelebrateDataPtr celebrate, int vnum) {
	Celebrates::CelebrateRoomsList::iterator room;
	Celebrates::LoadList::iterator load, load_in;

	log("Processing celebrate %s load section for zone %d", celebrate->name.c_str(), vnum);

	if (celebrate->rooms.find(vnum) != celebrate->rooms.end()) {
		for (room = celebrate->rooms[vnum].begin(); room != celebrate->rooms[vnum].end(); ++room) {
			RoomRnum rn = real_room((*room)->vnum);
			if (rn != kNowhere) {
				for (Celebrates::TrigList::iterator it = (*room)->triggers.begin(); it != (*room)->triggers.end();
					 ++it) {
					auto trig = read_trigger(real_trigger(*it));
					if (!add_trigger(world[rn]->script.get(), trig, -1)) {
						extract_trigger(trig);
					}
				}
			}

			for (load = (*room)->mobs.begin(); load != (*room)->mobs.end(); ++load) {
				CharData *mob;
				int i = real_mobile((*load)->vnum);
				if (i > 0
					&& mob_index[i].total_online < (*load)->max) {
					mob = read_mobile(i, REAL);
					if (mob) {
						for (Celebrates::TrigList::iterator it = (*load)->triggers.begin();
							 it != (*load)->triggers.end(); ++it) {
							auto trig = read_trigger(real_trigger(*it));
							if (!add_trigger(SCRIPT(mob).get(), trig, -1)) {
								extract_trigger(trig);
							}
						}
						load_mtrigger(mob);
						PlaceCharToRoom(mob, real_room((*room)->vnum));
						Celebrates::add_mob_to_load_list(mob->id, mob);
						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in) {
							ObjRnum rnum = real_object((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world()) {
								const auto obj = world_objects.create_from_prototype_by_vnum((*load_in)->vnum);
								if (obj) {
									PlaceObjToInventory(obj.get(), mob);
									obj->set_vnum_zone_from(zone_table[world[IN_ROOM(mob)]->zone_rn].vnum);

									for (Celebrates::TrigList::iterator it = (*load_in)->triggers.begin();
										 it != (*load_in)->triggers.end(); ++it) {
										auto trig = read_trigger(real_trigger(*it));
										if (!add_trigger(obj->get_script().get(), trig, -1)) {
											extract_trigger(trig);
										}
									}

									load_otrigger(obj.get());
									Celebrates::add_obj_to_load_list(obj->get_uid(), obj.get());
								} else {
									log("{Error] Processing celebrate %s while loading obj %d",
										celebrate->name.c_str(),
										(*load_in)->vnum);
								}
							}
						}
					} else {
						log("{Error] Processing celebrate %s while loading mob %d",
							celebrate->name.c_str(),
							(*load)->vnum);
					}
				}
			}
			for (load = (*room)->objects.begin(); load != (*room)->objects.end(); ++load) {
				ObjData *obj_room;
				ObjRnum rnum = real_object((*load)->vnum);
				if (rnum == -1) {
					log("{Error] Processing celebrate %s while loading obj %d", celebrate->name.c_str(), (*load)->vnum);
					return;
				}
				int obj_in_room = 0;

				for (obj_room = world[rn]->contents; obj_room; obj_room = obj_room->get_next_content()) {
					if (rnum == GET_OBJ_RNUM(obj_room)) {
						obj_in_room++;
					}
				}

				if ((obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world())
					&& (obj_in_room < (*load)->max)) {
					const auto obj = world_objects.create_from_prototype_by_vnum((*load)->vnum);
					if (obj) {
						for (Celebrates::TrigList::iterator it = (*load)->triggers.begin();
							 it != (*load)->triggers.end(); ++it) {
							auto trig = read_trigger(real_trigger(*it));
							if (!add_trigger(obj->get_script().get(), trig, -1)) {
								extract_trigger(trig);
							}
						}
						load_otrigger(obj.get());
						Celebrates::add_obj_to_load_list(obj->get_uid(), obj.get());

						PlaceObjToRoom(obj.get(), real_room((*room)->vnum));

						for (load_in = (*load)->objects.begin(); load_in != (*load)->objects.end(); ++load_in) {
							ObjRnum rnum = real_object((*load_in)->vnum);

							if (obj_proto.actual_count(rnum) < obj_proto[rnum]->get_max_in_world()) {
								const auto obj_in = world_objects.create_from_prototype_by_vnum((*load_in)->vnum);
								if (obj_in
									&& GET_OBJ_TYPE(obj) == EObjType::kContainer) {
									PlaceObjIntoObj(obj_in.get(), obj.get());
									obj_in->set_vnum_zone_from(GET_OBJ_VNUM_ZONE_FROM(obj));

									for (Celebrates::TrigList::iterator it = (*load_in)->triggers.begin();
										 it != (*load_in)->triggers.end(); ++it) {
										auto trig = read_trigger(real_trigger(*it));
										if (!add_trigger(obj_in->get_script().get(), trig, -1)) {
											extract_trigger(trig);
										}
									}

									load_otrigger(obj_in.get());
									Celebrates::add_obj_to_load_list(obj->get_uid(), obj.get());
								} else {
									log("{Error] Processing celebrate %s while loading obj %d",
										celebrate->name.c_str(),
										(*load_in)->vnum);
								}
							}
						}
					} else {
						log("{Error] Processing celebrate %s while loading mob %d",
							celebrate->name.c_str(),
							(*load)->vnum);
					}
				}
			}
		}
	}
}

void process_attach_celebrate(Celebrates::CelebrateDataPtr celebrate, int zone_vnum) {
	log("Processing celebrate %s attach section for zone %d", celebrate->name.c_str(), zone_vnum);

	if (celebrate->mobsToAttach.find(zone_vnum) != celebrate->mobsToAttach.end()) {
		//поскольку единственным доступным способом получить всех мобов одного внума является
		//обход всего списка мобов в мире, то будем хотя бы 1 раз его обходить
		Celebrates::AttachList list = celebrate->mobsToAttach[zone_vnum];
		for (const auto &ch : character_list) {
			const auto rnum = ch->get_rnum();
			if (rnum > 0
				&& list.find(mob_index[rnum].vnum) != list.end()) {
				for (Celebrates::TrigList::iterator it = list[mob_index[rnum].vnum].begin();
					 it != list[mob_index[rnum].vnum].end();
					 ++it) {
					auto trig = read_trigger(real_trigger(*it));
					if (!add_trigger(SCRIPT(ch).get(), trig, -1)) {
						extract_trigger(trig);
					}
				}

				Celebrates::add_mob_to_attach_list(ch->id, ch.get());
			}
		}
	}

	if (celebrate->objsToAttach.find(zone_vnum) != celebrate->objsToAttach.end()) {
		Celebrates::AttachList list = celebrate->objsToAttach[zone_vnum];

		world_objects.foreach([&](const ObjData::shared_ptr &o) {
			if (o->get_rnum() > 0 && list.find(o->get_rnum()) != list.end()) {
				for (Celebrates::TrigList::iterator it = list[o->get_rnum()].begin(); it != list[o->get_rnum()].end();
					 ++it) {
					auto trig = read_trigger(real_trigger(*it));
					if (!add_trigger(o->get_script().get(), trig, -1)) {
						extract_trigger(trig);
					}
				}

				Celebrates::add_obj_to_attach_list(o->get_uid(), o.get());
			}
		});
	}
}

void process_celebrates(int vnum) {
	Celebrates::CelebrateDataPtr mono = Celebrates::get_mono_celebrate();
	Celebrates::CelebrateDataPtr poly = Celebrates::get_poly_celebrate();
	Celebrates::CelebrateDataPtr real = Celebrates::get_real_celebrate();

	if (mono) {
		process_load_celebrate(mono, vnum);
		process_attach_celebrate(mono, vnum);
	}

	if (poly) {
		process_load_celebrate(poly, vnum);
		process_attach_celebrate(poly, vnum);
	}
	if (real) {
		process_load_celebrate(real, vnum);
		process_attach_celebrate(real, vnum);
	}
}

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); }

// Выполить команду, только если предыдущая успешна
#define        CHECK_SUCCESS        1
// Команда не должна изменить флаг
#define        FLAG_PERSIST        2

class ZoneReset {
 public:
	ZoneReset(const ZoneRnum zone) : m_zone_rnum(zone) {}

	void reset();

 private:
	bool handle_zone_Q_command(const MobRnum rnum);

	// execute the reset command table of a given zone
	void reset_zone_essential();

	ZoneRnum m_zone_rnum;
};

void ZoneReset::reset() {
	if (GlobalObjects::stats_sender().ready()) {
		utils::CExecutionTimer timer;

		reset_zone_essential();

		const auto execution_time = timer.delta();
		influxdb::Record record("zone_reset");
		record.add_tag("pulse", GlobalObjects::heartbeat().pulse_number());
		record.add_tag("zone", zone_table[m_zone_rnum].vnum);
		record.add_field("duration", execution_time.count());
		GlobalObjects::stats_sender().send(record);
	} else {
		reset_zone_essential();
	}
}

bool ZoneReset::handle_zone_Q_command(const MobRnum rnum) {
	utils::CExecutionTimer overall_timer;
	bool extracted = false;

	utils::CExecutionTimer get_mobs_timer;
	Characters::list_t mobs;
	character_list.get_mobs_by_rnum(rnum, mobs);
	const auto get_mobs_time = get_mobs_timer.delta();

	utils::CExecutionTimer extract_timer;
	for (const auto &mob : mobs) {
		if (!MOB_FLAGGED(mob, EMobFlag::kResurrected)) {
			ExtractCharFromWorld(mob.get(), false, true);
			extracted = true;
		}
	}
	const auto extract_time = extract_timer.delta();

	const auto execution_time = overall_timer.delta();

	if (GlobalObjects::stats_sender().ready()) {
		influxdb::Record record("Q_command");

		record.add_tag("pulse", GlobalObjects::heartbeat().pulse_number());
		record.add_tag("zone", zone_table[m_zone_rnum].vnum);
		record.add_tag("rnum", rnum);

		record.add_field("duration", execution_time.count());
		record.add_field("extract", extract_time.count());
		record.add_field("get_mobs", get_mobs_time.count());
		GlobalObjects::stats_sender().send(record);
	}

	return extracted;
}

void ZoneReset::reset_zone_essential() {
	int cmd_no;
	int cmd_tmp, obj_in_room_max, obj_in_room = 0;
	CharData *mob = nullptr, *leader = nullptr;
	ObjData *obj_to, *obj_room;
	CharData *tmob = nullptr;    // for trigger assignment
	ObjData *tobj = nullptr;    // for trigger assignment
	const auto zone = m_zone_rnum;    // for ZCMD macro
	int last_state, curr_state;    // статус завершения последней и текущей команды

	log("[Reset] Start zone %s", zone_table[m_zone_rnum].name);
	//----------------------------------------------------------------------------
	last_state = 1;        // для первой команды считаем, что все ок

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
		if (ZCMD.command == '*') {
			// комментарий - ни на что не влияет
			continue;
		}

		curr_state = 0;    // по умолчанию - неудачно
		if (!(ZCMD.if_flag & CHECK_SUCCESS) || last_state) {
			// Выполняем команду, если предыдущая успешна или не нужно проверять
			switch (ZCMD.command) {
				case 'M':
					// read a mobile
					// 'M' <flag> <MobVnum> <max_in_world> <RoomVnum> <max_in_room|-1>

					if (ZCMD.arg3 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка загрузить моба в 0 комнату. (VNUM = %d, ZONE = %d)",
								mob_index[ZCMD.arg1].vnum, zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					mob = nullptr;    //Добавлено Ладником
					if (mob_index[ZCMD.arg1].total_online < ZCMD.arg2 &&
						(ZCMD.arg4 < 0 || mobs_in_room(ZCMD.arg1, ZCMD.arg3) < ZCMD.arg4)) {
						mob = read_mobile(ZCMD.arg1, REAL);
						if (!mob) {
							sprintf(buf,
									"ZRESET: ошибка! моб %d  в зоне %d не существует",
									ZCMD.arg1,
									zone_table[m_zone_rnum].vnum);
							mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
							return;
						}
						if (!mob_proto[mob->get_rnum()].get_role_bits().any()) {
							int rndlev = mob->GetLevel();
							rndlev += number(-2, +2);
							mob->set_level(std::max(1, rndlev));
						}

						PlaceCharToRoom(mob, ZCMD.arg3);
						load_mtrigger(mob);
						tmob = mob;
						curr_state = 1;
					}
					tobj = nullptr;
					break;

				case 'F':
					// Follow mobiles
					// 'F' <flag> <RoomVnum> <leader_vnum> <MobVnum>
					leader = nullptr;
					if (ZCMD.arg1 >= kFirstRoom && ZCMD.arg1 <= top_of_world) {
						for (const auto ch : world[ZCMD.arg1]->people) {
							if (ch->IsNpc() && GET_MOB_RNUM(ch) == ZCMD.arg2) {
								leader = ch;
							}
						}

						if (leader) {
							for (const auto ch : world[ZCMD.arg1]->people) {
								if (ch->IsNpc()
									&& GET_MOB_RNUM(ch) == ZCMD.arg3
									&& leader != ch
									&& !ch->makes_loop(leader)) {
									if (ch->has_master()) {
										stop_follower(ch, kSfEmpty);
									}

									leader->add_follower(ch);

									curr_state = 1;
								}
							}
						}
					}
					break;

				case 'Q':
					if (handle_zone_Q_command(ZCMD.arg1)) {
						curr_state = 1;
					}

					tobj = nullptr;
					tmob = nullptr;
					break;

				case 'O':
					// read an object
					// 'O' <flag> <ObjVnum> <max_in_world> <RoomVnum|-1> <load%|-1>
					// Проверка  - сколько всего таких же обьектов надо на эту клетку

					if (ZCMD.arg3 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка загрузить объект в 0 комнату. (VNUM = %d, ZONE = %d)",
								obj_proto[ZCMD.arg1]->get_vnum(), zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					for (cmd_tmp = 0, obj_in_room_max = 0; ZCMD_CMD(cmd_tmp).command != 'S'; cmd_tmp++)
						if ((ZCMD_CMD(cmd_tmp).command == 'O')
							&& (ZCMD.arg1 == ZCMD_CMD(cmd_tmp).arg1)
							&& (ZCMD.arg3 == ZCMD_CMD(cmd_tmp).arg3))
							obj_in_room_max++;
					// Теперь считаем склько их на текущей клетке
					for (obj_room = world[ZCMD.arg3]->contents, obj_in_room = 0; obj_room;
						 obj_room = obj_room->get_next_content()) {
						if (ZCMD.arg1 == GET_OBJ_RNUM(obj_room)) {
							obj_in_room++;
						}
					}
					// Теперь грузим обьект если надо
					if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
						&& (ZCMD.arg4 <= 0
							|| number(1, 100) <= ZCMD.arg4)
						&& (obj_in_room < obj_in_room_max)) {
						const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
						obj->set_vnum_zone_from(zone_table[world[ZCMD.arg3]->zone_rn].vnum);

						if (!PlaceObjToRoom(obj.get(), ZCMD.arg3)) {
							ExtractObjFromWorld(obj.get());
							break;
						}
						load_otrigger(obj.get());

						tobj = obj.get();
						curr_state = 1;

						if (!obj->has_flag(EObjFlag::kNodecay)) {
							sprintf(buf, "&YВНИМАНИЕ&G На землю загружен объект без флага NODECAY : %s (VNUM=%d)",
									GET_OBJ_PNAME(obj, 0).c_str(), obj->get_vnum());
							mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
						}
					}
					tmob = nullptr;
					break;

				case 'P':
					// object to object
					// 'P' <flag> <ObjVnum> <room or 0> <target_vnum> <load%|-1>
					if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
						&& (ZCMD.arg4 <= 0
							|| number(1, 100) <= ZCMD.arg4)) {
						if (ZCMD.arg2 > 0) {
							RoomRnum arg2 = real_room(ZCMD.arg2);
							if (arg2 == kNowhere) {
								ZONE_ERROR("room in arg2 not found, command omited");
								break;
							}
							if (!(obj_to = GetObjByRnumInContent(ZCMD.arg3, world[arg2]->contents))) {
								break;
							}
						} else {
							if (!(obj_to = SearchObjByRnum(ZCMD.arg3))) {
								ZONE_ERROR("target obj not found in word, command omited");
								break;
							}
						}
						if (GET_OBJ_TYPE(obj_to) != EObjType::kContainer) {
							ZONE_ERROR("attempt put obj to non container, omited");
							ZCMD.command = '*';
							break;
						}
						const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
						if (obj_to->get_in_room() != kNowhere) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_in_room()]->zone_rn].vnum);
						} else if (obj_to->get_worn_by()) {
							obj->set_vnum_zone_from(zone_table[world[IN_ROOM(obj_to->get_worn_by())]->zone_rn].vnum);
						} else if (obj_to->get_carried_by()) {
							obj->set_vnum_zone_from(zone_table[world[IN_ROOM(obj_to->get_carried_by())]->zone_rn].vnum);
						}
						PlaceObjIntoObj(obj.get(), obj_to);
						load_otrigger(obj.get());
						tobj = obj.get();
						curr_state = 1;
					}
					tmob = nullptr;
					break;

				case 'G':
					// obj_to_char
					// 'G' <flag> <ObjVnum> <max_in_world> <-> <load%|-1>
					if (!mob)
						//Изменено Ладником
					{
						// ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
						// ZCMD.command = '*';
						break;
					}
					if ((obj_proto.actual_count(ZCMD.arg1) < GET_OBJ_MIW(obj_proto[ZCMD.arg1])
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
						&& (ZCMD.arg4 <= 0
							|| number(1, 100) <= ZCMD.arg4)) {
						const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
						PlaceObjToInventory(obj.get(), mob);
						obj->set_vnum_zone_from(zone_table[world[IN_ROOM(mob)]->zone_rn].vnum);
						tobj = obj.get();
						load_otrigger(obj.get());
						curr_state = 1;
					}
					tmob = nullptr;
					break;

				case 'E':
					// object to equipment list
					// 'E' <flag> <ObjVnum> <max_in_world> <wear_pos> <load%|-1>
					//Изменено Ладником
					if (!mob) {
						//ZONE_ERROR("trying to equip non-existant mob, command disabled");
						// ZCMD.command = '*';
						break;
					}
					if ((obj_proto.actual_count(ZCMD.arg1) < obj_proto[ZCMD.arg1]->get_max_in_world()
						|| GET_OBJ_MIW(obj_proto[ZCMD.arg1]) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| check_unlimited_timer(obj_proto[ZCMD.arg1].get()))
						&& (ZCMD.arg4 <= 0
							|| number(1, 100) <= ZCMD.arg4)) {
						if (ZCMD.arg3 < 0
							|| ZCMD.arg3 >= EEquipPos::kNumEquipPos) {
							ZONE_ERROR("invalid equipment pos number");
						} else {
							const auto obj = world_objects.create_from_prototype_by_rnum(ZCMD.arg1);
							obj->set_vnum_zone_from(zone_table[world[IN_ROOM(mob)]->zone_rn].vnum);
							obj->set_in_room(IN_ROOM(mob));
							load_otrigger(obj.get());
							if (wear_otrigger(obj.get(), mob, ZCMD.arg3)) {
								obj->set_in_room(kNowhere);
								EquipObj(mob, obj.get(), ZCMD.arg3, CharEquipFlags());
							} else {
								PlaceObjToInventory(obj.get(), mob);
							}
							if (!(obj->get_carried_by() == mob) && !(obj->get_worn_by() == mob)) {
								ExtractObjFromWorld(obj.get(), false);
								tobj = nullptr;
							} else {
								tobj = obj.get();
								curr_state = 1;
							}
						}
					}
					tmob = nullptr;
					break;

				case 'R':
					// rem obj from room
					// 'R' <flag> <RoomVnum> <ObjVnum>

					if (ZCMD.arg1 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка удалить объект из 0 комнаты. (VNUM = %d, ZONE = %d)",
								obj_proto[ZCMD.arg2]->get_vnum(), zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					if (const auto obj = GetObjByRnumInContent(ZCMD.arg2, world[ZCMD.arg1]->contents)) {
						ExtractObjFromWorld(obj);
						curr_state = 1;
					}
					tmob = nullptr;
					tobj = nullptr;
					break;

				case 'D':
					// set state of door
					// 'D' <flag> <RoomVnum> <door_pos> <door_state>

					if (ZCMD.arg1 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка установить двери в 0 комнате. (ZONE = %d)",
								zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					if (ZCMD.arg2 < 0 || ZCMD.arg2 >= EDirection::kMaxDirNum ||
						(world[ZCMD.arg1]->dir_option[ZCMD.arg2] == nullptr)) {
						ZONE_ERROR("door does not exist, command disabled");
						ZCMD.command = '*';
					} else {
						REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EExitFlag::kBrokenLock);
						switch (ZCMD.arg3) {
							case 0:
								REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
									exit_info, EExitFlag::kLocked);
								REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
									exit_info, EExitFlag::kClosed);
								break;
							case 1: SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EExitFlag::kClosed);
								REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
									exit_info, EExitFlag::kLocked);
								break;
							case 2: SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EExitFlag::kLocked);
								SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EExitFlag::kClosed);
								break;
							case 3: SET_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->exit_info, EExitFlag::kHidden);
								break;
							case 4:
								REMOVE_BIT(world[ZCMD.arg1]->dir_option[ZCMD.arg2]->
									exit_info, EExitFlag::kHidden);
								break;
						}
						curr_state = 1;
					}
					tmob = nullptr;
					tobj = nullptr;
					break;

				case 'T':
					// trigger command; details to be filled in later
					// 'T' <flag> <trigger_type> <trigger_vnum> <RoomVnum, для WLD_TRIGGER>
					if (ZCMD.arg1 == MOB_TRIGGER && tmob) {
						auto trig = read_trigger(real_trigger(ZCMD.arg2));
						if (!add_trigger(SCRIPT(tmob).get(), trig, -1)) {
							extract_trigger(trig);
						}
						curr_state = 1;
					} else if (ZCMD.arg1 == OBJ_TRIGGER && tobj) {
						auto trig = read_trigger(real_trigger(ZCMD.arg2));
						if (!add_trigger(tobj->get_script().get(), trig, -1)) {
							extract_trigger(trig);
						}
						curr_state = 1;
					} else if (ZCMD.arg1 == WLD_TRIGGER) {
						if (ZCMD.arg3 > kNowhere) {
							auto trig = read_trigger(real_trigger(ZCMD.arg2));
							if (!add_trigger(world[ZCMD.arg3]->script.get(), trig, -1)) {
								extract_trigger(trig);
							}
							curr_state = 1;
						}
					}
					break;

				case 'V':
					// 'V' <flag> <trigger_type> <RoomVnum> <context> <var_name> <var_value>
					if (ZCMD.arg1 == MOB_TRIGGER && tmob) {
						if (!SCRIPT(tmob)->has_triggers()) {
							ZONE_ERROR("Attempt to give variable to scriptless mobile");
						} else {
							add_var_cntx(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
							curr_state = 1;
						}
					} else if (ZCMD.arg1 == OBJ_TRIGGER && tobj) {
						if (!tobj->get_script()->has_triggers()) {
							ZONE_ERROR("Attempt to give variable to scriptless object");
						} else {
							add_var_cntx(&(tobj->get_script()->global_vars), ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
							curr_state = 1;
						}
					} else if (ZCMD.arg1 == WLD_TRIGGER) {
						if (ZCMD.arg2 < kFirstRoom || ZCMD.arg2 > top_of_world) {
							ZONE_ERROR("Invalid room number in variable assignment");
						} else {
							if (!SCRIPT(world[ZCMD.arg2])->has_triggers()) {
								ZONE_ERROR("Attempt to give variable to scriptless room");
							} else {
								add_var_cntx(&(world[ZCMD.arg2]->script->global_vars),
											 ZCMD.sarg1,
											 ZCMD.sarg2,
											 ZCMD.arg3);
								curr_state = 1;
							}
						}
					}
					break;

				default: ZONE_ERROR("unknown cmd in reset table; cmd disabled");
					ZCMD.command = '*';
					break;
			}
		}

		if (!(ZCMD.if_flag & FLAG_PERSIST)) {
			// команда изменяет флаг
			last_state = curr_state;
		}
	}
	if (zone_table[m_zone_rnum].used) {
		zone_table[m_zone_rnum].count_reset++;
	}

	zone_table[m_zone_rnum].age = 0;
	zone_table[m_zone_rnum].used = false;
	process_celebrates(zone_table[m_zone_rnum].vnum);
	int rnum_start = 0;
	int rnum_stop = 0;

	if (GetZoneRooms(m_zone_rnum, &rnum_start, &rnum_stop)) {
		// все внутренние резеты комнат зоны теперь идут за один цикл
		// резет порталов теперь тут же и переписан, чтобы не гонять по всем румам, ибо жрал половину времени резета -- Krodo
		for (int rnum = rnum_start; rnum <= rnum_stop; rnum++) {
			RoomData *room = world[rnum];
			reset_wtrigger(room);
			int sect = real_sector(real_room(room->room_vn));
			if (!(sect == ESector::kWaterSwim || sect == ESector::kWaterNoswim || sect == ESector::kOnlyFlying)) {
				im_reset_room(room, zone_table[m_zone_rnum].level, zone_table[m_zone_rnum].type);
			}
			RoomData *gate_room = OneWayPortal::get_from_room(room);
			if (gate_room)   // случай врат
			{
				gate_room->portal_time = 0;
				OneWayPortal::remove(room);
			} else if (room->portal_time > 0)   // случай двусторонней пенты
			{
				world[room->portal_room]->portal_time = 0;
				room->portal_time = 0;
			}
			paste_on_reset(room);
		}
	}
	for (rnum_start = 0; rnum_start < static_cast<int>(zone_table.size()); rnum_start++) {
		// проверяем, не содержится ли текущая зона в чьем-либо typeB_list
		for (curr_state = zone_table[rnum_start].typeB_count; curr_state > 0; curr_state--) {
			if (zone_table[rnum_start].typeB_list[curr_state - 1] == zone_table[m_zone_rnum].vnum) {
				zone_table[rnum_start].typeB_flag[curr_state - 1] = true;

				break;
			}
		}
	}
	//Если это ведущая зона, то при ее сбросе обнуляем typeB_flag
	for (rnum_start = zone_table[m_zone_rnum].typeB_count; rnum_start > 0; rnum_start--)
		zone_table[m_zone_rnum].typeB_flag[rnum_start - 1] = false;
	log("[Reset] Stop zone %s", zone_table[m_zone_rnum].name);
	after_reset_zone(m_zone_rnum);
}

void reset_zone(ZoneRnum zone) {
	ZoneReset zreset(zone);
	zreset.reset();
}


// Ищет RNUM первой и последней комнаты зоны
// Еси возвращает 0 - комнат в зоне нету
int GetZoneRooms(ZoneRnum zone_nr, int *first, int *last) {
//31.27.2022 месяц пройдет комменты можно стереть
//	*first = 0;
//	*last = 0;
//	debug::backtrace(runtime_config.logs(SYSLOG).handle());
//	auto numzone = zone_table[zone_nr].vnum * 100;
//	sprintf(buf, "По новому ! Зона %d: первая клетка зоны %d, последняя %d", zone_table[zone_nr].vnum, zone_table[zone_nr].FirstRoom, zone_table[zone_nr].LastRoom);
//	mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	*first = real_room(zone_table[zone_nr].FirstRoomVnum);
	*last = real_room(zone_table[zone_nr].LastRoomVnum);
	if (*first == kNowhere)
		return 0;
/*
	for (int nr = kFirstRoom; nr <= top_of_world; nr++) {
		if (world[nr]->room_vn >= numzone && *first == 0) {
			*first = world[nr]->room_vn;
		}
		if (world[nr]->room_vn == numzone + 99) {
			// если первая комната виртуальная (99), то последняя тоже будет виртуальной
			if (*first == numzone + 99) {
				*last = *first;
				break;
			}
			// если в зоне есть комнаты, то берем последнюю комнату перед виртуальной
			if (nr > 1 && *first && world[nr - 1]->room_vn >= *first) {
				*last = world[nr - 1]->room_vn;
				break;
			}
		}
	}
	sprintf(buf, "По старому! Зона %d: первая клетка зоны %d, последняя %d", zone_table[zone_nr].vnum, *first, *last);
	mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);

	*first = real_room(*first);
	*last = real_room(*last);
	if (*first == kNowhere || *last == kNowhere)
		return 0;
*/
	
	return 1;
}

// for use in reset_zone; return true if zone 'nr' is free of PC's
bool is_empty(ZoneRnum zone_nr, bool debug) {
	int rnum_start, rnum_stop;
	static ZoneRnum last_zone_nr = 0;
	static bool result;
	if (debug) {
		sprintf(buf, "is_empty чек. Зона %d", zone_table[zone_nr].vnum);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	}
	if (last_zone_nr == zone_nr) {
		if (debug) {
			sprintf(buf, "is_empty повтор. Зона %d, прошлый запрос %d", zone_table[zone_nr].vnum, zone_table[last_zone_nr].vnum);
			mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		}
		return result;
	}
	last_zone_nr = zone_nr;
	for (auto i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == kNowhere)
			continue;
		if (GetRealLevel(i->character) >= kLvlImmortal)
			continue;
		if (world[i->character->in_room]->zone_rn != zone_nr)
			continue;
		result = false;
		return false;
	}

	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!GetZoneRooms(zone_nr, &rnum_start, &rnum_stop)) {
		result = true;
		return true;    // в зоне нет комнат :)
	}

	for (; rnum_start <= rnum_stop; rnum_start++) {
// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		for (const auto c : world[rnum_start]->people) {
			if (!c->IsNpc() && (GetRealLevel(c) < kLvlImmortal)) {
				result = false;
				return false;
			}
		}
	}

// теперь проверю всех товарищей в void комнате STRANGE_ROOM
	for (const auto c : world[kStrangeRoom]->people) {
		const int was = c->get_was_in_room();

		if (was == kNowhere
			|| GetRealLevel(c) >= kLvlImmortal
			|| world[was]->zone_rn != zone_nr) {
			continue;
		}
		result = false;
		return false;
	}

	if (room_spells::IsZoneRoomAffected(zone_nr, ESpell::kRuneLabel)) {
		result = false;
		return false;
	}
	if (debug) {
		sprintf(buf, "is_empty чек по клеткам зоны. Зона %d в зоне НИКОГО!!!", zone_table[zone_nr].vnum);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	}
	result = true;
	return true;
}

int mobs_in_room(int m_num, int r_num) {
	int count = 0;

	for (const auto &ch : world[r_num]->people) {
		if (m_num == GET_MOB_RNUM(ch)
			&& !MOB_FLAGGED(ch, EMobFlag::kResurrected)) {
			count++;
		}
	}
	return count;
}

/*************************************************************************
*  stuff related to the save/load player system                          *
*************************************************************************/

long cmp_ptable_by_name(char *name, int len) {
	len = std::min(len, static_cast<int>(strlen(name)));
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): I am not sure but I guess that linear search is not the best solution here. TODO: make map helper (MAPHELPER). */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		const char *pname = player_table[i].name();
		if (!strn_cmp(pname, arg, std::min(len, static_cast<int>(strlen(pname))))) {
			return static_cast<long>(i);
		}
	}
	return -1;
}

long get_ptable_by_name(const char *name) {
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		const char *pname = player_table[i].name();
		if (!str_cmp(pname, arg)) {
			return static_cast<long>(i);
		}
	}
	std::stringstream buffer;
	buffer << "Char " << name << " (" << arg << ") not found !!! Сброшен стек в сислог";
	debug::backtrace(runtime_config.logs(SYSLOG).handle());
//	sprintf(buf, "Char %s(%s) not found !!!", name, arg);
	mudlog(buffer.str().c_str(), CMP, kLvlImmortal, SYSLOG, false);
	return (-1);
}

long get_ptable_by_unique(long unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].unique == unique) {
			return static_cast<long>(i);
		}
	}
	return 0;
}

long get_id_by_name(char *name) {
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (!str_cmp(player_table[i].name(), arg)) {
			return (player_table[i].id());
		}
	}

	return (-1);
}

long get_id_by_uid(long uid) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].unique == uid) {
			return player_table[i].id();
		}
	}
	return -1;
}

int get_uid_by_id(int id) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].id() == id) {
			return player_table[i].unique;
		}
	}
	return -1;
}

const char *get_name_by_id(long id) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].id() == id) {
			return player_table[i].name();
		}
	}
	return "";
}

const char *get_name_by_unique(int unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].unique == unique) {
			return player_table[i].name();
		}
	}
	return nullptr;
}

int get_level_by_unique(long unique) {
	int level = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); ++i) {
		if (player_table[i].unique == unique) {
			level = player_table[i].level;
		}
	}
	return level;
}

long get_lastlogon_by_unique(long unique) {
	long time = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); ++i) {
		if (player_table[i].unique == unique) {
			time = player_table[i].last_logon;
		}
	}
	return time;
}

int correct_unique(int unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].unique == unique) {
			return true;
		}
	}

	return false;
}

void recreate_saveinfo(const size_t number) {
	delete player_table[number].timer;
	NEWCREATE(player_table[number].timer);
}

/**
* Можно канеш просто в get_skill иммам возвращать 100 или там 200, но тут зато можно
* потестить че-нить с возможностью покачать скилл во время игры иммом.
*/
void set_god_skills(CharData *ch) {
	for (auto i = ESkill::kFirst; i <= ESkill::kLast; ++i) {
		if (MUD::Skills().IsValid(i)) {
			ch->set_skill(i, 200);
		}
	}
}

#define NUM_OF_SAVE_THROWS    5

// по умолчанию reboot = 0 (пользуется только при ребуте)
int load_char(const char *name, CharData *char_element, bool reboot, const bool find_id) {
	const auto player_i = char_element->load_char_ascii(name, reboot, find_id);
	if (player_i > -1) {
		char_element->set_pfilepos(player_i);
	}
	return (player_i);
}

/************************************************************************
*  funcs of a (more or less) general utility nature                     *
************************************************************************/

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
int file_to_string_alloc(const char *name, char **buf) {
	char temp[kMaxExtendLength];
	DescriptorData *in_use;

	for (in_use = descriptor_list; in_use; in_use = in_use->next) {
		if (in_use->showstr_vector && *in_use->showstr_vector == *buf) {
			return (-1);
		}
	}

	// Lets not free() what used to be there unless we succeeded.
	if (file_to_string(name, temp) < 0)
		return (-1);

	if (*buf)
		free(*buf);

	*buf = str_dup(temp);
	return (0);
}

// read contents of a text file, and place in buf
int file_to_string(const char *name, char *buf) {
	FILE *fl;
	char tmp[READ_SIZE + 3];

	*buf = '\0';

	if (!(fl = fopen(name, "r"))) {
		log("SYSERR: reading %s: %s", name, strerror(errno));
		sprintf(buf + strlen(buf), "Error: file '%s' is empty.\r\n", name);
		return (0);
	}
	do {
		const char *dummy = fgets(tmp, READ_SIZE, fl);
		UNUSED_ARG(dummy);

		tmp[strlen(tmp) - 1] = '\0';    // take off the trailing \n
		strcat(tmp, "\r\n");

		if (!feof(fl)) {
			if (strlen(buf) + strlen(tmp) + 1 > kMaxExtendLength) {
				log("SYSERR: %s: string too big (%d max)", name, kMaxStringLength);
				*buf = '\0';
				return (-1);
			}
			strcat(buf, tmp);
		}
	} while (!feof(fl));

	fclose(fl);

	return (0);
}

void ClearCharTalents(CharData *ch) {
	ch->real_abils.Feats.reset();
	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		GET_SPELL_TYPE(ch, spell_id) = 0;
		GET_SPELL_MEM(ch, spell_id) = 0;
	}
	ch->clear_skills();
}

const char *remort_msg =
	"  Если вы так настойчивы в желании начать все заново -\r\n" "наберите <перевоплотиться> полностью.\r\n";

void do_remort(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	int i, place_of_destination, load_room = kNowhere;
	const char *remort_msg2 = "$n вспыхнул$g ослепительным пламенем и пропал$g!\r\n";

	if (ch->IsNpc() || IS_IMMORTAL(ch)) {
		SendMsgToChar("Вам это, похоже, совсем ни к чему.\r\n", ch);
		return;
	}
	if (GET_EXP(ch) < GetExpUntilNextLvl(ch, kLvlImmortal) - 1) {
		SendMsgToChar("ЧАВО???\r\n", ch);
		return;
	}
/*	if (Remort::need_torc(ch) && !PRF_FLAGGED(ch, EPrf::kCanRemort)) {
		SendMsgToChar(ch,
					  "Вы должны подтвердить свои заслуги, пожертвовав Богам достаточное количество гривен.\r\n"
					  "%s\r\n", Remort::WHERE_TO_REMORT_STR.c_str());
		return;
	}
*/
	if (ch->get_remort() > kMaxRemort) {
		SendMsgToChar("Достигнуто максимальное количество перевоплощений.\r\n", ch);
		return;
	}
	if (NORENTABLE(ch)) {
		SendMsgToChar("Вы не можете перевоплотиться в связи с боевыми действиями.\r\n", ch);
		return;
	}
	if (!subcmd) {
		SendMsgToChar(remort_msg, ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		sprintf(buf, "Укажите, где вы хотите заново начать свой путь:\r\n");
		sprintf(buf + strlen(buf),
				"%s",
				string(Birthplaces::ShowMenu(PlayerRace::GetRaceBirthPlaces(GET_KIN(ch), GET_RACE(ch)))).c_str());
		SendMsgToChar(buf, ch);
		return;
	} else {
		// Сначала проверим по словам - может нам текстом сказали?
		place_of_destination = Birthplaces::ParseSelect(arg);
		if (place_of_destination == kBirthplaceUndefined) {
			//Нет, значит или ерунда в аргументе, или цифирь, смотрим
			place_of_destination = PlayerRace::CheckBirthPlace(GET_KIN(ch), GET_RACE(ch), arg);
			if (!Birthplaces::CheckId(place_of_destination)) {
				SendMsgToChar("Багдад далече, выберите себе местечко среди родных осин.\r\n", ch);
				return;
			}
		}
	}

	log("Remort %s", GET_NAME(ch));
	ch->remort();
	act(remort_msg2, false, ch, nullptr, nullptr, kToRoom);

	if (ch->is_morphed()) ch->reset_morph();
	ch->set_remort(ch->get_remort() + 1);
	CLR_GOD_FLAG(ch, EGf::kRemort);
	ch->inc_str(1);
	ch->inc_dex(1);
	ch->inc_con(1);
	ch->inc_wis(1);
	ch->inc_int(1);
	ch->inc_cha(1);

	if (ch->GetEnemy())
		stop_fighting(ch, true);

	die_follower(ch);
	while (!ch->affected.empty()) {
		ch->affect_remove(ch->affected.begin());
	}

// Снимаем весь стафф
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			PlaceObjToInventory(UnequipChar(ch, i, CharEquipFlags()), ch);
		}
	}

	while (ch->timed) {
		ExpireTimedSkill(ch, ch->timed);
	}

	while (ch->timed_feat) {
		ExpireTimedFeat(ch, ch->timed_feat);
	}
	for (const auto &feat : MUD::Feats()) {
		ch->UnsetFeat(feat.GetId());
	}

	if (ch->get_remort() >= 9 && ch->get_remort() % 3 == 0) {
		ch->clear_skills();
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_TYPE(ch, spell_id) = (IS_MANA_CASTER(ch) ? ESpellType::kRunes : 0);
			GET_SPELL_MEM(ch, spell_id) = 0;
		}
	} else {
		ch->SetSkillAfterRemort(ch->get_remort());
		for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
			if (IS_MANA_CASTER(ch)) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kRunes;
			} else if (MUD::Class(ch->GetClass()).spells[spell_id].GetCircle() >= 8) {
				GET_SPELL_TYPE(ch, spell_id) = ESpellType::kUnknowm;
				GET_SPELL_MEM(ch, spell_id) = 0;
			}
		}
	}

	GET_HIT(ch) = GET_MAX_HIT(ch) = 10;
	GET_MOVE(ch) = GET_MAX_MOVE(ch) = 82;
	ch->mem_queue.total = ch->mem_queue.stored = 0;
	ch->set_level(0);
	GET_WIMP_LEV(ch) = 0;
	GET_AC(ch) = 100;
	GET_LOADROOM(ch) = calc_loadroom(ch, place_of_destination);
	PRF_FLAGS(ch).unset(EPrf::KSummonable);
	PRF_FLAGS(ch).unset(EPrf::kAwake);
	PRF_FLAGS(ch).unset(EPrf::kPunctual);
	PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
	PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
	PRF_FLAGS(ch).unset(EPrf::kAwake);
	PRF_FLAGS(ch).unset(EPrf::kIronWind);
	PRF_FLAGS(ch).unset(EPrf::kDoubleThrow);
	PRF_FLAGS(ch).unset(EPrf::kTripleThrow);
	PRF_FLAGS(ch).unset(EPrf::kShadowThrow);
	// Убираем все заученные порталы
	check_portals(ch);
	if (ch->get_protecting()) {
		ch->remove_protecting();
		ch->battle_affects.unset(kEafProtect);
	}

	//Обновляем статистику рипов для текущего перевоплощения
	GET_RIP_DTTHIS(ch) = 0;
	GET_EXP_DTTHIS(ch) = 0;
	GET_RIP_MOBTHIS(ch) = 0;
	GET_EXP_MOBTHIS(ch) = 0;
	GET_RIP_PKTHIS(ch) = 0;
	GET_EXP_PKTHIS(ch) = 0;
	GET_RIP_OTHERTHIS(ch) = 0;
	GET_EXP_OTHERTHIS(ch) = 0;

	// забываем всех чармисов
	std::map<int, MERCDATA> *m;
	m = ch->getMercList();
	if (m->size() > 0) {
		std::map<int, MERCDATA>::iterator it = m->begin();
		for (; it != m->end(); ++it)
			it->second.currRemortAvail = 0;
	}
	do_start(ch, false);
	ch->save_char();
	if (PLR_FLAGGED(ch, EPlrFlag::kHelled))
		load_room = r_helled_start_room;
	else if (PLR_FLAGGED(ch, EPlrFlag::kNameDenied))
		load_room = r_named_start_room;
	else if (PLR_FLAGGED(ch, EPlrFlag::kFrozen))
		load_room = r_frozen_start_room;
	else {
		if ((load_room = GET_LOADROOM(ch)) == kNowhere)
			load_room = calc_loadroom(ch);
		load_room = real_room(load_room);
	}
	if (load_room == kNowhere) {
		if (GetRealLevel(ch) >= kLvlImmortal)
			load_room = r_immort_start_room;
		else
			load_room = r_mortal_start_room;
	}
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, load_room);
	look_at_room(ch, 0);
	PLR_FLAGS(ch).set(EPlrFlag::kNoDelete);
	RemoveRuneLabelFromWorld(ch, ESpell::kRuneLabel);

	// сброс всего, связанного с гривнами (замакс сохраняем)
	PRF_FLAGS(ch).unset(EPrf::kCanRemort);
	ch->set_ext_money(ExtMoney::kTorcGold, 0);
	ch->set_ext_money(ExtMoney::kTorcSilver, 0);
	ch->set_ext_money(ExtMoney::kTorcBronze, 0);

	snprintf(buf, sizeof(buf),
			 "remort from %d to %d", ch->get_remort() - 1, ch->get_remort());
	snprintf(buf2, sizeof(buf2), "dest=%d", place_of_destination);
	add_karma(ch, buf, buf2);

	act("$n вступил$g в игру.", true, ch, 0, 0, kToRoom);
	act("Вы перевоплотились! Желаем удачи!", false, ch, 0, 0, kToChar);
}

// returns the real number of the room with given virtual number
RoomRnum real_room(RoomVnum vnum) {
	RoomRnum bot, top, mid;

	bot = 1;        // 0 - room is kNowhere
	top = top_of_world;
	// perform binary search on world-table
	for (;;) {
		mid = (bot + top) / 2;

		if (world[mid]->room_vn == vnum)
			return (mid);
		if (bot >= top)
			return (kNowhere);
		if (world[mid]->room_vn > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// returns the real number of the monster with given virtual number
MobRnum real_mobile(MobVnum vnum) {
	MobRnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	// perform binary search on mob-table
	for (;;) {
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

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
int must_be_deleted(CharData *short_ch) {
	int ci, timeout;

	if (PLR_FLAGS(short_ch).get(EPlrFlag::kNoDelete)) {
		return 0;
	}

	if (PLR_FLAGS(short_ch).get(EPlrFlag::kDeleted)) {
		return 1;
	}

	if (short_ch->get_remort()) {
		return (0);
	}

	timeout = -1;
	for (ci = 0; ci == 0 || pclean_criteria[ci].level > pclean_criteria[ci - 1].level; ci++) {
		if (short_ch->GetLevel() <= pclean_criteria[ci].level) {
			timeout = pclean_criteria[ci].days;
			break;
		}
	}
	if (timeout >= 0) {
		timeout *= kSecsPerRealDay;
		if ((time(0) - LAST_LOGON(short_ch)) > timeout) {
			return (1);
		}
	}

	return (0);
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void entrycount(char *name, const bool find_id /*= true*/) {
	int deleted;
	char filename[kMaxStringLength];

	if (get_filename(name, filename, kPlayersFile)) {
		Player t_short_ch;
		Player *short_ch = &t_short_ch;
		deleted = 1;
		// персонаж загружается неполностью
		if (load_char(name, short_ch, 1, find_id) > -1) {
			// если чар удален или им долго не входили, то не создаем для него запись
			if (!must_be_deleted(short_ch)) {
				deleted = 0;

				PlayerIndexElement element(GET_IDNUM(short_ch), GET_NAME(short_ch));


				CREATE(element.mail, strlen(GET_EMAIL(short_ch)) + 1);
				for (int i = 0; (element.mail[i] = LOWER(GET_EMAIL(short_ch)[i])); i++);

				CREATE(element.last_ip, strlen(GET_LASTIP(short_ch)) + 1);
				for (int i = 0; (element.last_ip[i] = GET_LASTIP(short_ch)[i]); i++);

				element.unique = GET_UNIQUE(short_ch);
				element.level = GetRealLevel(short_ch);
				element.remorts = short_ch->get_remort();
				element.timer = nullptr;
				element.plr_class = short_ch->GetClass();
				if (PLR_FLAGS(short_ch).get(EPlrFlag::kDeleted)) {
					element.last_logon = -1;
					element.activity = -1;
				} else {
					element.last_logon = LAST_LOGON(short_ch);
					element.activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
				}

#ifdef TEST_BUILD
				log("entry: char:%s level:%d mail:%s ip:%s", element.name(), element.level, element.mail, element.last_ip);
#endif

				top_idnum = std::max(top_idnum, GET_IDNUM(short_ch));
				TopPlayer::Refresh(short_ch, true);

				log("Adding new player %s", element.name());
				player_table.append(element);
			}
			else {
				log("Delete %s from account email: %s", GET_NAME(short_ch), short_ch->get_account()->get_email().c_str());
				short_ch->get_account()->remove_player(short_ch->get_uid());
			}
		} else {
			log("SYSERR: Failed to load player %s.", name);
		}

		// если чар уже удален, то стираем с диска его файл
		if (deleted) {
			log("Player %s already deleted - kill player file", name);
			remove(filename);
			// 2) Remove all other files
			get_filename(name, filename, kAliasFile);
			remove(filename);
			get_filename(name, filename, kScriptVarsFile);
			remove(filename);
			get_filename(name, filename, kPersDepotFile);
			remove(filename);
			get_filename(name, filename, kShareDepotFile);
			remove(filename);
			get_filename(name, filename, kPurgeDepotFile);
			remove(filename);
			get_filename(name, filename, kTextCrashFile);
			remove(filename);
			get_filename(name, filename, kTimeCrashFile);
			remove(filename);
		}
	}
}

void new_build_player_index() {
	FILE *players;
	char name[kMaxInputLength], playername[kMaxInputLength];

	if (!(players = fopen(LIB_PLRS "players.lst", "r"))) {
		log("Players list empty...");
		return;
	}

	while (get_line(players, name)) {
		if (!*name || *name == ';')
			continue;
		if (sscanf(name, "%s ", playername) == 0)
			continue;

		if (!player_table.player_exists(playername)) {
			entrycount(playername, false);
		}
	}

	fclose(players);

	player_table.name_adviser().init();
}

void flush_player_index() {
	FILE *players;
	char name[kMaxStringLength];

	if (!(players = fopen(LIB_PLRS "players.lst", "w+"))) {
		log("Can't save players list...");
		return;
	}

	std::size_t saved = 0;
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (!player_table[i].name()
			|| !*player_table[i].name()) {
			continue;
		}

		++saved;
		sprintf(name, "%s %d %d %d %d\n",
				player_table[i].name(),
				player_table[i].id(), player_table[i].unique, player_table[i].level, player_table[i].last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Сохранено индексов %zd (считано при загрузке %zd)", saved, player_table.size());
}

void rename_char(CharData *ch, char *oname) {
	char filename[kMaxInputLength], ofilename[kMaxInputLength];

	// 1) Rename(if need) char and pkill file - directly
	log("Rename char %s->%s", GET_NAME(ch), oname);
	get_filename(oname, ofilename, kPlayersFile);
	get_filename(GET_NAME(ch), filename, kPlayersFile);
	rename(ofilename, filename);

	ch->save_char();

	// 2) Rename all other files
	get_filename(oname, ofilename, kTextCrashFile);
	get_filename(GET_NAME(ch), filename, kTextCrashFile);
	rename(ofilename, filename);

	get_filename(oname, ofilename, kTimeCrashFile);
	get_filename(GET_NAME(ch), filename, kTimeCrashFile);
	rename(ofilename, filename);

	get_filename(oname, ofilename, kAliasFile);
	get_filename(GET_NAME(ch), filename, kAliasFile);
	rename(ofilename, filename);

	get_filename(oname, ofilename, kScriptVarsFile);
	get_filename(GET_NAME(ch), filename, kScriptVarsFile);
	rename(ofilename, filename);

	// хранилища
	Depot::rename_char(ch);
	get_filename(oname, ofilename, kPersDepotFile);
	get_filename(GET_NAME(ch), filename, kPersDepotFile);
	rename(ofilename, filename);
	get_filename(oname, ofilename, kPurgeDepotFile);
	get_filename(GET_NAME(ch), filename, kPurgeDepotFile);
	rename(ofilename, filename);
}

// * Добровольное удаление персонажа через игровое меню.
void delete_char(const char *name) {
	Player t_st;
	Player *st = &t_st;
	int id = load_char(name, st);

	if (id >= 0) {
		PLR_FLAGS(st).set(EPlrFlag::kDeleted);
		NewNames::remove(st);
		if (NAME_FINE(st)) {
			player_table.name_adviser().add(GET_NAME(st));
		}
		Clan::remove_from_clan(GET_UNIQUE(st));
		st->save_char();

		Crash_clear_objects(id);
		player_table[id].unique = -1;
		player_table[id].level = -1;
		player_table[id].remorts = -1;
		player_table[id].last_logon = -1;
		player_table[id].activity = -1;
		if (player_table[id].mail)
			free(player_table[id].mail);
		player_table[id].mail = nullptr;
		if (player_table[id].last_ip)
			free(player_table[id].last_ip);
		player_table[id].last_ip = nullptr;
	}
}

void room_copy(RoomData *dst, RoomData *src)
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
		struct TrackData *track = dst->track;
		ObjData *contents = dst->contents;
		const auto people_backup = dst->people;
		auto affected = dst->affected;

		// Копирую все поверх
		*dst = *src;

		// Восстанавливаю track, contents, people, аффекты
		dst->track = track;
		dst->contents = contents;
		dst->people = std::move(people_backup);
		dst->affected = affected;
	}

	// Теперь нужно выделить собственные области памяти

	// Название и описание
	dst->name = str_dup(src->name ? src->name : "неопределено");
	dst->temp_description = 0; // так надо

	// Выходы и входы
	for (i = 0; i < EDirection::kMaxDirNum; ++i) {
		const auto &rdd = src->dir_option[i];
		if (rdd) {
			dst->dir_option[i].reset(new ExitData());
			// Копируем числа
			*dst->dir_option[i] = *rdd;
			// Выделяем память
			dst->dir_option[i]->general_description = rdd->general_description;
			dst->dir_option[i]->keyword = (rdd->keyword ? str_dup(rdd->keyword) : nullptr);
			dst->dir_option[i]->vkeyword = (rdd->vkeyword ? str_dup(rdd->vkeyword) : nullptr);
		}
	}

	// Дополнительные описания, если есть
	ExtraDescription::shared_ptr *pddd = &dst->ex_description;
	ExtraDescription::shared_ptr sdd = src->ex_description;
	*pddd = nullptr;

	while (sdd) {
		pddd->reset(new ExtraDescription());
		(*pddd)->keyword = sdd->keyword ? str_dup(sdd->keyword) : nullptr;
		(*pddd)->description = sdd->description ? str_dup(sdd->description) : nullptr;
		pddd = &((*pddd)->next);
		sdd = sdd->next;
	}

	// Копирую скрипт и прототипы
	SCRIPT(dst).reset(new Script());

	dst->proto_script.reset(new ObjData::triggers_list_t());
	*dst->proto_script = *src->proto_script;
}

void room_free(RoomData *room)
/*++
   Функция полностью освобождает память, занимаемую данными комнаты.
   ВНИМАНИЕ. Память самой структуры room_data не освобождается.
             Необходимо дополнительно использовать delete()
--*/
{
	// Название и описание
	if (room->name) {
		free(room->name);
		room->name = nullptr;
	}

	if (room->temp_description) {
		free(room->temp_description);
		room->temp_description = nullptr;
	}

	// Выходы и входы
	for (int i = 0; i < EDirection::kMaxDirNum; i++) {
		if (room->dir_option[i]) {
			room->dir_option[i].reset();
		}
	}

	// Скрипт
	room->cleanup_script();
	room->affected.clear();
}

void LoadGlobalUID() {
	FILE *guid;
	char buffer[256];

	global_uid = 0;

	if (!(guid = fopen(LIB_MISC "globaluid", "r"))) {
		log("Can't open global uid file...");
		return;
	}
	get_line(guid, buffer);
	global_uid = atoi(buffer);
	fclose(guid);
}

void SaveGlobalUID() {
	FILE *guid;

	if (!(guid = fopen(LIB_MISC "globaluid", "w"))) {
		log("Can't write global uid file...");
		return;
	}

	fprintf(guid, "%d\n", global_uid);
	fclose(guid);
}

void LoadGuardians() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_MISC"guards.xml");
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node xMainNode = doc.child("guardians");

	if (!xMainNode) {
		snprintf(buf, kMaxStringLength, "...guards.xml read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	guardian_list.clear();

	int num_wars_global = atoi(xMainNode.child_value("wars"));

	struct mob_guardian tmp_guard;
	for (pugi::xml_node xNodeGuard = xMainNode.child("guard"); xNodeGuard;
		 xNodeGuard = xNodeGuard.next_sibling("guard")) {
		int guard_vnum = xNodeGuard.attribute("vnum").as_int();

		if (guard_vnum <= 0) {
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

		if (!strcmp(xNodeGuard.attribute("killer").value(), "no"))
			tmp_guard.agro_killers = false;

		if (!strcmp(xNodeGuard.attribute("agressor").value(), "yes"))
			tmp_guard.agro_all_agressors = true;

		if (!strcmp(xNodeGuard.attribute("agressor").value(), "list")) {
			for (pugi::xml_node xNodeZone = xNodeGuard.child("zone"); xNodeZone;
				 xNodeZone = xNodeZone.next_sibling("zone")) {
				tmp_guard.agro_argressors_in_zones.push_back(atoi(xNodeZone.child_value()));
			}
		}
		guardian_list[guard_vnum] = tmp_guard;
	}

}

//Polud тестовый класс для хранения параметров различных рас мобов
//Читает данные из файла
const char *MOBRACE_FILE = LIB_CFG "mob_races.xml";

MobRaceListType mobraces_list;

//загрузка рас из файла
void load_mobraces() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MOBRACE_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node node_list = doc.child("mob_races");

	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...mob races read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	for (auto & race : node_list.children("mob_race")) {
		MobRacePtr tmp_mobrace(new MobRace);
		auto race_num = race.attribute("id").as_int();
		tmp_mobrace->race_name = race.attribute("name").value();

		pugi::xml_node imList = race.child("imlist");

		for (pugi::xml_node im = imList.child("im"); im; im = im.next_sibling("im")) {
			struct ingredient tmp_ingr;
			tmp_ingr.imtype = im.attribute("type").as_int();
			tmp_ingr.imname = string(im.attribute("name").value());
			utils::Trim(tmp_ingr.imname);
			int cur_lvl = 1;
			int prob_value = 1;
			for (pugi::xml_node prob = im.child("prob"); prob; prob = prob.next_sibling("prob")) {
				int next_lvl = prob.attribute("lvl").as_int();
				if (next_lvl > 0) {
					for (int lvl = cur_lvl; lvl < next_lvl; lvl++)
						tmp_ingr.prob[lvl - 1] = prob_value;
				} else {
					log("SYSERROR: Неверный уровень lvl=%d для ингредиента %s расы %s",
						next_lvl,
						tmp_ingr.imname.c_str(),
						tmp_mobrace->race_name.c_str());
					return;
				}
				prob_value = atoi(prob.child_value());
				cur_lvl = next_lvl;
			}
			for (int lvl = cur_lvl; lvl <= kMaxMobLevel; lvl++)
				tmp_ingr.prob[lvl - 1] = prob_value;

			tmp_mobrace->ingrlist.push_back(tmp_ingr);
		}
		mobraces_list[race_num] = tmp_mobrace;
	}
}

MobRace::MobRace() {
	ingrlist.clear();
}

MobRace::~MobRace() {
	ingrlist.clear();
}
//-Polud

////////////////////////////////////////////////////////////////////////////////
namespace OfftopSystem {

const char *BLOCK_FILE = LIB_MISC"offtop.lst";
std::vector<std::string> block_list;

/// Проверка на наличие чара в стоп-списке и сет флага
void set_flag(CharData *ch) {
	std::string mail(GET_EMAIL(ch));
	utils::ConvertToLow(mail);
	auto i = std::find(block_list.begin(), block_list.end(), mail);
	if (i != block_list.end()) {
		PRF_FLAGS(ch).set(EPrf::kStopOfftop);
	} else {
		PRF_FLAGS(ch).unset(EPrf::kStopOfftop);
	}
}

/// Лоад/релоад списка нежелательных для оффтопа товарисчей.
void init() {
	block_list.clear();
	std::ifstream file(BLOCK_FILE);
	if (!file.is_open()) {
		log("SYSERROR: не удалось открыть файл на чтение: %s", BLOCK_FILE);
		return;
	}
	std::string buffer;
	while (file >> buffer) {
		utils::ConvertToLow(buffer);
		block_list.push_back(buffer);
	}

	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character) {
			set_flag(d->character.get());
		}
	}
}

} // namespace OfftopSystem
////////////////////////////////////////////////////////////////////////////////

void load_speedwalk() {

	pugi::xml_document doc_sw;
	pugi::xml_node child_, object_, file_sw;
	file_sw = XMLLoad(LIB_MISC SPEEDWALKS_FILE, "speedwalks", "Error loading cases file: speedwalks.xml", doc_sw);

	for (child_ = file_sw.child("speedwalk"); child_; child_ = child_.next_sibling("speedwalk")) {
		SpeedWalk sw;
		sw.wait = 0;
		sw.cur_state = 0;
		sw.default_wait = child_.attribute("default_wait").as_int();
		for (object_ = child_.child("mob"); object_; object_ = object_.next_sibling("mob"))
			sw.vnum_mobs.push_back(object_.attribute("vnum").as_int());
		for (object_ = child_.child("route"); object_; object_ = object_.next_sibling("route")) {
			Route r;
			r.direction = object_.attribute("direction").as_string();
			r.wait = object_.attribute("wait").as_int();
			sw.route.push_back(r);
		}
		speedwalks.push_back(sw);

	}
	for (const auto &ch : character_list) {
		for (auto &sw : speedwalks) {
			for (auto mob : sw.vnum_mobs) {
				if (GET_MOB_VNUM(ch) == mob) {
					sw.mobs.push_back(ch.get());
				}
			}
		}
	}

}

Rooms::~Rooms() {
	log("~Rooms()");
	for (auto i = this->begin(); i != this->end(); ++i)
		delete *i;
}

const std::size_t PlayersIndex::NOT_FOUND = ~static_cast<std::size_t>(0);

PlayersIndex::~PlayersIndex() {
	log("~PlayersIndex()");
}

std::size_t PlayersIndex::append(const PlayerIndexElement &element) {
	const auto index = size();

	push_back(element);
	m_id_to_index.emplace(element.id(), index);
	add_name_to_index(element.name(), index);

	return index;
}

std::size_t PlayersIndex::get_by_name(const char *name) const {
	name_to_index_t::const_iterator i = m_name_to_index.find(name);
	if (i != m_name_to_index.end()) {
		return i->second;
	}

	return NOT_FOUND;
}

void PlayersIndex::set_name(const std::size_t index, const char *name) {
	name_to_index_t::const_iterator i = m_name_to_index.find(operator[](index).name());
	m_name_to_index.erase(i);
	operator[](index).set_name(name);
	add_name_to_index(name, index);
}

void PlayersIndex::add_name_to_index(const char *name, const std::size_t index) {
	if (m_name_to_index.find(name) != m_name_to_index.end()) {
		log("SYSERR: Detected attempt to create player with duplicate name.");
		abort();
	}

	m_name_to_index.emplace(name, index);
}

PlayerIndexElement::PlayerIndexElement(const int id, const char *name) :
	mail(nullptr),
	last_ip(nullptr),
	unique(0),
	level(0),
	remorts(0),
	plr_class(ECharClass::kUndefined),
	last_logon(0),
	activity(0),
	timer(nullptr),
	m_id(id),
	m_name(nullptr) {
	set_name(name);
}

void PlayerIndexElement::set_name(const char *name) {
	delete[] m_name;

	char *new_name = new char[strlen(name) + 1];
	for (int i = 0; (new_name[i] = LOWER(name[i])); i++);

	m_name = new_name;
}

std::size_t PlayersIndex::hasher::operator()(const std::string &value) const {
	// FNV-1a implementation
	static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");

	const std::size_t FNV_offset_basis = 14695981039346656037ULL;
	const std::size_t FNV_prime = 1099511628211ULL;

	const auto count = value.size();
	std::size_t result = FNV_offset_basis;
	for (std::size_t i = 0; i < count; ++i) {
		result ^= (std::size_t) LOWER(value[i]);
		result *= FNV_prime;
	}

	return result;
}

bool PlayersIndex::equal_to::operator()(const std::string &left, const std::string &right) const {
	if (left.size() != right.size()) {
		return false;
	}

	for (std::size_t i = 0; i < left.size(); ++i) {
		if (LOWER(left[i]) != LOWER(right[i])) {
			return false;
		}
	}

	return true;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
