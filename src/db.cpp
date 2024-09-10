#define DB_CPP_

#include "administration/accounts.h"
#include "cmd_god/ban.h"
#include "boards/boards.h"
#include "boot/boot_data_files.h"
#include "boot/boot_index.h"
#include "communication/social.h"
#include "game_crafts/jewelry.h"
#include "game_crafts/mining.h"
#include "game_mechanics/player_races.h"
#include "corpse.h"
#include "game_mechanics/celebrates.h"
#include "game_mechanics/deathtrap.h"
#include "game_mechanics/dungeons.h"
#include "game_mechanics/city_guards.h"
#include "description.h"
#include "depot.h"
#include "game_economics/ext_money.h"
#include "game_mechanics/bonus.h"
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
#include "communication/offtop.h"
#include "communication/parcel.h"
#include "administration/privilege.h"
#include "game_mechanics/sets_drop.h"
#include "game_mechanics/stable_objs.h"
#include "game_economics/shop_ext.h"
#include "stuff.h"
#include "title.h"
#include "statistics/top.h"
#include "backtrace.h"
#include "dg_script/dg_db_scripts.h"
#include "game_mechanics/mob_races.h"
#include "game_mechanics/treasure_cases.h"
#include "game_mechanics/cities.h"

#include <third_party_libs/fmt/include/fmt/format.h>
#include <sys/stat.h>

#include <memory>

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
const long kBeginningOfTime = -1561789232;
#else
const long kBeginningOfTime = 650336715;
#endif

Rooms &world = GlobalObjects::world();

RoomRnum top_of_world = 0;    // ref to top element of world

void AddTrigIndexEntry(int nr, Trigger *trig) {
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
std::unordered_map<long, CharData *> chardata_by_uid;
int global_uid = 0;

PlayersIndex &player_table = GlobalObjects::player_table();    // index to plr file

bool IsPlayerExists(const long id) { return player_table.IsPlayerExists(id); }

long top_idnum = 0;        // highest idnum in use

int circle_restrict = 0;    // level of game restriction
RoomRnum r_mortal_start_room;    // rnum of mortal start room
RoomRnum r_immort_start_room;    // rnum of immort start room
RoomRnum r_frozen_start_room;    // rnum of frozen start room
RoomRnum r_helled_start_room;
RoomRnum r_named_start_room;
RoomRnum r_unreg_start_room;

char *credits{nullptr};        // game credits
char *motd{nullptr};        // message of the day - mortals
char *rules{nullptr};        // rules for immorts
char *greetings{nullptr};        // opening credits screen
char *help{nullptr};        // help screen
char *info{nullptr};        // info page
char *immlist{nullptr};        // list of peon gods
char *background{nullptr};    // background story
char *handbook{nullptr};        // handbook for new immortals
char *policies{nullptr};        // policies page
char *name_rules{nullptr};        // rules of character's names

TimeInfoData time_info;    // the infomation about the time
struct reset_q_type reset_q;    // queue of zones to be reset

const FlagData clear_flags;

const char *ZONE_TRAFFIC_FILE = LIB_PLRSTUFF"zone_traffic.xml";
time_t zones_stat_date;

GameLoader world_loader;

// local functions
void LoadGlobalUid();
void AssignMobiles();
void AssignObjects();
void AssignRooms();
void InitSpecProcs();
void BuildPlayerIndex();
int ReadFileToBuffer(const char *name, char *destination_buf);
void CheckStartRooms();
void AddVirtualRoomsToAllZones();
void CalculateFirstAndLastRooms();
void CalculateFirstAndLastMobs();
void RosolveWorldDoorToRoomVnumsToRnums();
void PasteDirs();
void ResolveZoneTableCmdVnumArgsToRnums();
void LogZoneError(const ZoneData &zone_data, int cmd_no, const char *message);
void ResetGameWorldTime();
int CountMobsInRoom(int m_num, int r_num);
void BuildPlayerIndexNew();
void SetZoneRnumForObjects();
void SetZoneRnumForMobiles();
void InitBasicValues();
TimeInfoData *CalcMudTimePassed(time_t time_to, time_t time_from);
void LoadMessages();
void SortCommands();
void ReadCharacterInvalidNamesList();
int CompareSocials(const void *a, const void *b);
int ReadCrashTimerFile(std::size_t index, int temp);
int LoadExchange();
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void CalcEaster();

// external
extern int number_of_social_messages;
extern int number_of_social_commands;
extern int no_specials;
extern RoomVnum mortal_start_room;
extern RoomVnum immort_start_room;
extern RoomVnum frozen_start_room;
extern RoomVnum helled_start_room;
extern RoomVnum named_start_room;
extern RoomVnum unreg_start_room;
extern struct MonthTemperature year_temp[];
extern struct PCCleanCriteria pclean_criteria[];

extern void AddKarma(CharData *ch, const char *punish, const char *reason);
extern void ExtractTrigger(Trigger *trig);
extern ESkill FixNameAndFindSkillId(char *name);
extern void CopyMobilePrototypeForMedit(CharData *dst, CharData *src, bool partial_copy);
extern void DecayObjectsOnRepop(std::vector<ZoneRnum> &zone_list);

char *ReadActionMsgFromFile(FILE *fl, int nr) {
	char local_buf[kMaxStringLength];

	const char *result = fgets(local_buf, kMaxStringLength, fl);
	UNUSED_ARG(result);

	if (feof(fl)) {
		log("SYSERR: ReadActionMsgFromFile: unexpected EOF near action #%d", nr);
		exit(1);
	}
	if (*local_buf == '#')
		return (nullptr);

	local_buf[strlen(local_buf) - 1] = '\0';
	return (str_dup(local_buf));
}

// Separate a 4-character id tag from the data it precedes
void ExtractTagFromArgument(char *argument, char *tag) {
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

void LoadSheduledReboot() {
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

		//if we get non-1-full-day offset, but server will reboot too early (36 hour is minimum)
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
pugi::xml_node XmlLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document &Doc) {
	std::ostringstream buffer;
	pugi::xml_parse_result Result;
	pugi::xml_node NodeList;

	Result = Doc.load_file(PathToFile);
	if (!Result) {
		buffer << "..." << Result.description() << "(file: " << PathToFile << ")";
		mudlog(std::string(buffer.str()).c_str(), CMP, kLvlImmortal, SYSLOG, true);
		return NodeList;
	}

	NodeList = Doc.child(MainTag);
	if (!NodeList) {
		mudlog(ErrorStr, CMP, kLvlImmortal, SYSLOG, true);
	}

	return NodeList;
}

/// конверт поля GET_OBJ_SKILL в емкостях TODO: 12.2013
int ConvertDrinkconSkillField(CObjectPrototype *obj, bool proto) {
	if (obj->get_spec_param() > 0
		&& (GET_OBJ_TYPE(obj) == EObjType::kLiquidContainer
			|| GET_OBJ_TYPE(obj) == EObjType::kFountain)) {
		log("obj_skill: %d - %s (%d)", obj->get_spec_param(), GET_OBJ_PNAME(obj, 0).c_str(), GET_OBJ_VNUM(obj));
		// если емскости уже просетили какие-то заклы, то зелье
		// из обж-скилл их не перекрывает, а просто удаляется
		if (obj->GetPotionValueKey(ObjVal::EValueKey::POTION_PROTO_VNUM) < 0) {
			const auto potion = world_objects.create_from_prototype_by_vnum(obj->get_spec_param());
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
		obj->set_spec_param(to_underlying(ESkill::kUndefined));

		return 1;
	}
	return 0;
}

/// конверт параметров прототипов ПОСЛЕ лоада всех файлов с прототипами
void ConvertObjValues() {
	int save = 0;
	for (const auto &i : obj_proto) {
		save = std::max(save, ConvertDrinkconSkillField(i.get(), true));
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

void GameLoader::BootWorld() {
	utils::CSteppedProfiler boot_profiler("World booting", 1.1);

	boot_profiler.next_step("Loading zone table");
	log("Loading zone table.");
	GameLoader::BootIndex(DB_BOOT_ZON);

	boot_profiler.next_step("Create blank zoness for dungeons");
	log("Create zones for dungeons.");
	dungeons::CreateBlankZoneDungeon();

	boot_profiler.next_step("Loading triggers");
	log("Loading triggers and generating index.");
	GameLoader::BootIndex(DB_BOOT_TRG);

	boot_profiler.next_step("Create blank triggers for dungeons");
	log("Create triggers for dungeons.");
	dungeons::CreateBlankTrigsDungeon();

	boot_profiler.next_step("Loading rooms");
	log("Loading rooms.");
	GameLoader::BootIndex(DB_BOOT_WLD);

	boot_profiler.next_step("Create blank rooms for dungeons");
	log("Create blank rooms for dungeons.");
	dungeons::CreateBlankRoomDungeon();

	boot_profiler.next_step("Adding virtual rooms to all zones");
	log("Adding virtual rooms to all zones.");
	AddVirtualRoomsToAllZones();

	boot_profiler.next_step("Adding dirs to  room.");
	log("Adding dirs to room.");
	PasteDirs();

	boot_profiler.next_step("Calculate first end last room into zones");
	log("Calculate first and last room into zones.");
	CalculateFirstAndLastRooms();

	boot_profiler.next_step("Renumbering rooms");
	log("Renumbering rooms.");
	RosolveWorldDoorToRoomVnumsToRnums();

	boot_profiler.next_step("Checking start rooms");
	log("Checking start rooms.");
	CheckStartRooms();

	boot_profiler.next_step("Loading mobs and regerating index");
	log("Loading mobs and generating index.");
	GameLoader::BootIndex(DB_BOOT_MOB);

	boot_profiler.next_step("Counting mob's levels");
	log("Count mob quantity by level");
	MobMax::init();

	boot_profiler.next_step("Create blank mob for dungeons");
	log("Create blank mob for dungeons.");
	dungeons::CreateBlankMobsDungeon();

	boot_profiler.next_step("Calculate first end last mob into zones");
	log("Calculate first and last mob into zones.");
//	CalculateFirstAndLastMobs();

	boot_profiler.next_step("Loading objects");
	log("Loading objs and generating index.");
	GameLoader::BootIndex(DB_BOOT_OBJ);

	boot_profiler.next_step("Create blank obj for dungeons");
	log("Create blank obj for dungeons.");
	dungeons::CreateBlankObjsDungeon();

	boot_profiler.next_step("Calculate first end last mob into zones");
	log("Calculate first and last mob into zones.");
	CalculateFirstAndLastMobs();

	boot_profiler.next_step("Converting deprecated obj values");
	log("Converting deprecated obj values.");
	ConvertObjValues();

	boot_profiler.next_step("enumbering zone table");
	log("Renumbering zone table.");
	ResolveZoneTableCmdVnumArgsToRnums();

	boot_profiler.next_step("Renumbering Obj_zone");
	log("Renumbering Obj_zone.");
	SetZoneRnumForObjects();

	boot_profiler.next_step("Renumbering Mob_zone");
	log("Renumbering Mob_zone.");
	SetZoneRnumForMobiles();

	boot_profiler.next_step("Initialization of object rnums");
	log("Init system_obj rnums.");
	system_obj::init();

	log("Init global_drop_obj.");
}

void InitZoneTypes() {
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

void SetZoneMobLevel() {
	for (auto &i : zone_table) {
		int level = 0;
		int count = 0;

		for (int nr = 0; nr <= top_of_mobt; ++nr) {
			if (mob_index[nr].vnum >= i.vnum * 100
				&& mob_index[nr].vnum <= i.vnum * 100 + 99) {
				level += mob_proto[nr].GetLevel();
				++count;
			}
		}

		i.mob_level = count ? level / count : 0;
	}
}

void SetZonesTownFlags() {
	const auto zones_count = static_cast<ZoneRnum>(zone_table.size());
	for (ZoneRnum i = 0; i < zones_count; ++i) {
		zone_table[i].is_town = false;
		int rnum_start = 0, rnum_end = 0;
		if (!GetZoneRooms(i, &rnum_start, &rnum_end)) {
			continue;
		}

		bool rent_flag = false, bank_flag = false, post_flag = false;
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

		zone_table[i].is_town = (rent_flag && bank_flag && post_flag);
	}
}

void LoadMessages() {
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

		messages->die_msg.attacker_msg = ReadActionMsgFromFile(fl, i);
		messages->die_msg.victim_msg = ReadActionMsgFromFile(fl, i);
		messages->die_msg.room_msg = ReadActionMsgFromFile(fl, i);
		messages->miss_msg.attacker_msg = ReadActionMsgFromFile(fl, i);
		messages->miss_msg.victim_msg = ReadActionMsgFromFile(fl, i);
		messages->miss_msg.room_msg = ReadActionMsgFromFile(fl, i);
		messages->hit_msg.attacker_msg = ReadActionMsgFromFile(fl, i);
		messages->hit_msg.victim_msg = ReadActionMsgFromFile(fl, i);
		messages->hit_msg.room_msg = ReadActionMsgFromFile(fl, i);
		messages->god_msg.attacker_msg = ReadActionMsgFromFile(fl, i);
		messages->god_msg.victim_msg = ReadActionMsgFromFile(fl, i);
		messages->god_msg.room_msg = ReadActionMsgFromFile(fl, i);
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

	for (auto &i : zone_table) {
		pugi::xml_node zone_node = node_list.append_child();
		zone_node.set_name("zone");
		zone_node.append_attribute("vnum") = i.vnum;
		zone_node.append_attribute("traffic") = i.traffic;
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
		zrn = GetZoneRnum(zone_vnum);
		int num = atoi(node.attribute("traffic").value());
		if (zrn == 0 && zone_vnum != 1) {
			snprintf(buf, kMaxStringLength,
					 "zone_traffic: несуществующий номер зоны %d ее траффик %d ",
					 zone_vnum, num);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			continue;
		}
		zone_table[zrn].traffic = atoi(node.attribute("traffic").value());
	}
}

// body of the booting system
void BootMudDataBase() {
	utils::CSteppedProfiler boot_profiler("MUD booting", 1.1);

	log("Boot db -- BEGIN.");

	boot_profiler.next_step("Creating directories");
	struct stat st{};
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
	ResetGameWorldTime();

	boot_profiler.next_step("Reading credits, help, bground, info & motds.");
	log("Reading credits, help, bground, info & motds.");
	AllocateBufferForFile(CREDITS_FILE, &credits);
	AllocateBufferForFile(MOTD_FILE, &motd);
	AllocateBufferForFile(RULES_FILE, &rules);
	AllocateBufferForFile(HELP_PAGE_FILE, &help);
	AllocateBufferForFile(INFO_FILE, &info);
	AllocateBufferForFile(IMMLIST_FILE, &immlist);
	AllocateBufferForFile(POLICIES_FILE, &policies);
	AllocateBufferForFile(HANDBOOK_FILE, &handbook);
	AllocateBufferForFile(BACKGROUND_FILE, &background);
	AllocateBufferForFile(NAME_RULES_FILE, &name_rules);
	if (AllocateBufferForFile(GREETINGS_FILE, &greetings) == 0)
		PruneCrlf(greetings);

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

	boot_profiler.next_step("Loading daily quests");
	log("Loading daily quests.");
	DailyQuest::LoadFromFile();

	pugi::xml_document doc;
	boot_profiler.next_step("Loading undecayable object criterions");
	log("Loading undecayable object criterions.");
	stable_objs::LoadCriterionsCfg();

	boot_profiler.next_step("Loading birthplaces definitions");
	log("Loading birthplaces definitions.");
	Birthplaces::Load(XmlLoad(LIB_MISC BIRTH_PLACES_FILE, BIRTH_PLACE_MAIN_TAG, BIRTH_PLACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading player races definitions");
	log("Loading player races definitions.");
	PlayerRace::Load(XmlLoad(LIB_MISC PLAYER_RACE_FILE, RACE_MAIN_TAG, PLAYER_RACE_ERROR_STR, doc));

	boot_profiler.next_step("Loading ingredients magic");
	log("Booting IM");
	initIngredientsMagic();

	boot_profiler.next_step("Assigning character classs info.");
	log("Assigning character classs info.");
	MUD::CfgManager().LoadCfg("classes");

	InitSpellLevels();

	boot_profiler.next_step("Loading zone types and ingredient for each zone type");
	log("Booting zone types and ingredient types for each zone type.");
	InitZoneTypes();

	boot_profiler.next_step("Loading insert_wanted.lst");
	log("Booting insert_wanted.lst.");
	iwg.init();

	boot_profiler.next_step("Loading gurdians");
	log("Load guardians.");
	city_guards::LoadGuardians();

	boot_profiler.next_step("Loading world");
	GameLoader::BootWorld();

	boot_profiler.next_step("Loading stuff load table");
	log("Booting stuff load table.");
	oload_table.init();

	boot_profiler.next_step("Loading setstuff table");
	log("Booting setstuff table.");
	ObjData::InitSetTable();

	boot_profiler.next_step("Loading item levels");
	log("Init item levels.");
	ObjSystem::init_item_levels();

	boot_profiler.next_step("Loading help entries");
	log("Loading help entries.");
	GameLoader::BootIndex(DB_BOOT_HLP);

	boot_profiler.next_step("Loading social entries");
	log("Loading social entries.");
	GameLoader::BootIndex(DB_BOOT_SOCIAL);

	boot_profiler.next_step("Loading players index");
	log("Generating player index.");
	BuildPlayerIndex();

	// хэши читать после генерации плеер-таблицы
	boot_profiler.next_step("Loading CRC system");
	log("Loading file crc system.");
	FileCRC::load();

	boot_profiler.next_step("Loading fight messages");
	log("Loading fight messages.");
	LoadMessages();

	boot_profiler.next_step("Assigning function pointers");
	log("Assigning function pointers:");
	if (!no_specials) {
		log("   Mobiles.");
		AssignMobiles();
		log("   Objects.");
		AssignObjects();
		log("   Rooms.");
		AssignRooms();
	}

	boot_profiler.next_step("Reading skills variables.");
	log("Reading skills variables.");
	InitMiningVars();
	InitJewelryVars();

	boot_profiler.next_step("Sorting command list");
	log("Sorting command list.");
	SortCommands();

	boot_profiler.next_step("Reading banned site, proxy, privileges and invalid-name list.");
	log("Reading banned site, proxy, privileges and invalid-name list.");
	ban = new BanList();
	ReadCharacterInvalidNamesList();

	boot_profiler.next_step("Loading rented objects info");
	log("Booting rented objects info");
	for (std::size_t i = 0; i < player_table.size(); i++) {
		player_table[i].timer = nullptr;
		ReadCrashTimerFile(i, false);
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
	InitBasicValues();

	boot_profiler.next_step("Loading grouping parameters");
	log("Booting grouping parameters");
	if (grouping.init()) {
		exit(1);
	}

	boot_profiler.next_step("Loading special assignments");
	log("Booting special assignment");
	InitSpecProcs();

	boot_profiler.next_step("Assigning guilds info.");
	log("Assigning guilds info.");
	MUD::CfgManager().LoadCfg("guilds");

	boot_profiler.next_step("Loading portals for 'town portal' spell");
	log("Booting portals for 'town portal' spell");
	MUD::Runestones().LoadRunestones();

	boot_profiler.next_step("Loading made items");
	log("Booting maked items");
	init_make_items();

	boot_profiler.next_step("Loading exchange");
	log("Booting exchange");
	LoadExchange();

	boot_profiler.next_step("Loading scheduled reboot time");
	log("Load schedule reboot time");
	LoadSheduledReboot();

	boot_profiler.next_step("Loading proxies list");
	log("Load proxy list");
	LoadProxyList();

	boot_profiler.next_step("Loading new_name list");
	log("Load new_name list");
	NewNames::load();

	boot_profiler.next_step("Loading global UID timer");
	log("Load global uid counter");
	LoadGlobalUid();

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
	log("Load Celebrates.");
	celebrates::Load();

	// резет должен идти после лоада всех шмоток вне зон (хранилища и т.п.)
	boot_profiler.next_step("Resetting zones");
	for (ZoneRnum i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
		log("Resetting %s (rooms %d-%d).", zone_table[i].name.c_str(),
			(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
		ResetZone(i);
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
	mob_races::LoadMobraces();

	boot_profiler.next_step("Loading morphs");
	log("Load morphs.");
	load_morphs();

	boot_profiler.next_step("Initializing global drop list");
	log("Init global drop list.");
	GlobalDrop::init();

	boot_profiler.next_step("Loading offtop block list");
	log("Init offtop block list.");
	offtop_system::Init();

	boot_profiler.next_step("Loading shop_ext list");
	log("load shop_ext list start.");
	ShopExt::load(false);
	log("load shop_ext list stop.");

	boot_profiler.next_step("Loading zone average mob_level");
	log("Set zone average mob_level.");
	SetZoneMobLevel();

	boot_profiler.next_step("Setting zone town");
	log("Set zone town.");
	SetZonesTownFlags();

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
	boot_profiler.next_step("Loading treasure cases");
	log("Loading treasure cases.");
	treasure_cases::LoadTreasureCases();

	// справка должна иниться после всего того, что может в нее что-то добавить
	boot_profiler.next_step("Reloading help system");
	log("Loading help system.");
	HelpSystem::reload_all();

	boot_profiler.next_step("Loading bonus log");
	log("Loading bonus log.");
	Bonus::bonus_log_load();

	boot_profiler.next_step("Loading cities cfg");
	log("Loading cities cfg.");
	cities::LoadCities();

	shutdown_parameters.mark_boot_time();
	log("Boot db -- DONE.");

}

// reset the time in the game from file
void ResetGameWorldTime() {
	time_info = *CalcMudTimePassed(time(nullptr), kBeginningOfTime);
	// Calculate moon day
	weather_info.moon_day =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kMoonCycle;
	weather_info.week_day_mono =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kWeekCycle;
	weather_info.week_day_poly =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kPolyWeekCycle;
	// Calculate Easter
	CalcEaster();

	if (time_info.hours < sunrise[time_info.month][0]) {
		weather_info.sunlight = kSunDark;
	} else if (time_info.hours == sunrise[time_info.month][0]) {
		weather_info.sunlight = kSunRise;
	} else if (time_info.hours < sunrise[time_info.month][1]) {
		weather_info.sunlight = kSunLight;
	} else if (time_info.hours == sunrise[time_info.month][1]) {
		weather_info.sunlight = kSunSet;
	} else {
		weather_info.sunlight = kSunDark;
	}

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

	if ((time_info.month >= EMonth::kApril && time_info.month <= EMonth::kMay) ||
		(time_info.month == EMonth::kMarch && weather_info.temperature >= 3)) {
		weather_info.season = ESeason::kSpring;
	} else if (time_info.month >= EMonth::kJune && time_info.month <= EMonth::kAugust) {
		weather_info.season = ESeason::kSummer;
	} else if ((time_info.month >= EMonth::kSeptember && time_info.month <= EMonth::kOctober) ||
		(time_info.month == EMonth::kNovember && weather_info.temperature >= 3)) {
		weather_info.season = ESeason::kAutumn;
	} else {
		weather_info.season = ESeason::kWinter;
	}

	if (weather_info.pressure <= 980)
		weather_info.sky = kSkyLightning;
	else if (weather_info.pressure <= 1000) {
		weather_info.sky = kSkyRaining;
		if (time_info.month >= EMonth::kApril && time_info.month <= EMonth::kOctober)
			SetPrecipitations(&weather_info.weather_type, kWeatherLightrain, 40, 40, 20);
		else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary)
			SetPrecipitations(&weather_info.weather_type, kWeatherLightsnow, 50, 40, 10);
		else if (time_info.month == EMonth::kNovember || time_info.month == EMonth::kMarch) {
			if (weather_info.temperature >= 3)
				SetPrecipitations(&weather_info.weather_type, kWeatherLightrain, 70, 30, 0);
			else
				SetPrecipitations(&weather_info.weather_type, kWeatherLightsnow, 80, 20, 0);
		}
	} else if (weather_info.pressure <= 1020)
		weather_info.sky = kSkyCloudy;
	else
		weather_info.sky = kSkyCloudless;
}

// generate index table for the player file
void BuildPlayerIndex() {
	BuildPlayerIndexNew();
}

void GameLoader::BootIndex(const EBootType mode) {
	log("Index booting %d", mode);

	auto index = IndexFileFactory::get_index(mode);
	if (!index->open()) {
		exit(1);
	}
	const int rec_count = index->load();

	PrepareGlobalStructures(mode, rec_count);

	const auto data_file_factory = DataFileFactory::create();
	for (const auto &entry : *index) {
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
		qsort(soc_keys_list, number_of_social_commands, sizeof(struct SocialKeyword), CompareSocials);
	}
}

void GameLoader::PrepareGlobalStructures(const EBootType mode, const int rec_count) {
	// * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	switch (mode) {
		case DB_BOOT_TRG: CREATE(trig_index, rec_count);
			break;

		case DB_BOOT_WLD: {
			// Creating empty world with kNowhere room.
			world.push_back(new RoomData);
			top_of_world = kNowhere;
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
			zone_table.reserve(rec_count + dungeons::kNumberOfZoneDungeons);
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

void CheckRoomForIncompatibleFlags(int rnum) {
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
void CheckStartRooms() {
	if ((r_mortal_start_room = GetRoomRnum(mortal_start_room)) == kNowhere) {
		log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d", mortal_start_room);
		exit(1);
	}

	if ((r_immort_start_room = GetRoomRnum(immort_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}

	if ((r_frozen_start_room = GetRoomRnum(frozen_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}

	if ((r_helled_start_room = GetRoomRnum(helled_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
		r_helled_start_room = r_mortal_start_room;
	}

	if ((r_named_start_room = GetRoomRnum(named_start_room)) == kNowhere) {
		log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
		r_named_start_room = r_mortal_start_room;
	}

	if ((r_unreg_start_room = GetRoomRnum(unreg_start_room)) == kNowhere) {
		log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
		r_unreg_start_room = r_mortal_start_room;
	}
}

void RestoreRoomExitData(RoomRnum rrn) {
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
		const auto &from = world[rrn]->dir_option_proto[dir];

//		if (world[rrn]->dir_option[dir]) {
//			world[rrn]->dir_option[dir]->to_room(kNowhere);
//			world[rrn]->dir_option[dir].reset();
//		}
		if (from) {
			world[rrn]->dir_option[dir] = std::make_shared<ExitData>();
			world[rrn]->dir_option[dir]->to_room(from->to_room());
			if (!from->general_description.empty()) {
				world[rrn]->dir_option[dir]->general_description = from->general_description;
			}
			if (from->keyword) {
				world[rrn]->dir_option[dir]->set_keyword(from->keyword);
			}
			if (from->vkeyword) {
				world[rrn]->dir_option[dir]->set_vkeyword(from->vkeyword);
			}
			world[rrn]->dir_option[dir]->exit_info = from->exit_info;
			world[rrn]->dir_option[dir]->key = from->key;
			world[rrn]->dir_option[dir]->lock_complexity = from->lock_complexity;
		}
	}
}

void PasteDirs() {
	for (int i = kFirstRoom; i <= top_of_world; i++) {
		RestoreRoomExitData(i);
	}
}

void CalculateFirstAndLastMobs() {
}

void CalculateFirstAndLastRooms() {
	int current = world[1]->zone_rn;
	RoomRnum rn;
	ZoneRnum zrn = 0;

	zone_table[0].RnumRoomsLocation.first = 1;
	for (rn = 1; rn <= static_cast<RoomRnum>(world.size() - 1); rn++) {
		zrn = world[rn]->zone_rn;
		if (current != zrn) {
			zone_table[zrn].RnumRoomsLocation.first = rn;
			zone_table[current].RnumRoomsLocation.second = rn - 1;
			current = zrn;
		}
	}
	zone_table[zrn].RnumRoomsLocation.first = zone_table[zrn - 1].RnumRoomsLocation.second + 1;
	zone_table[zrn].RnumRoomsLocation.second = rn - 1;
	for (auto &zone_data : zone_table) {
		zone_data.RnumRoomsLocation.second--; //уберем виртуалки
	}
}

void AddVirtualRoomsToAllZones() {
	for (auto it = zone_table.begin(); it < zone_table.end(); ++it) {
		const auto first_room = it->vnum * 100;
		const auto last_room = first_room + 99;

		const RoomVnum virtual_vnumum = (it->vnum * 100) + 99;
		const auto vroom_it = std::find_if(world.cbegin(), world.cend(), [virtual_vnumum](RoomData *room) {
		  return room->vnum == virtual_vnumum;
		});
		if (vroom_it != world.cend()) {
			log("Zone %d already contains virtual room.", it->vnum);
			continue;
		}
		const auto rnum = static_cast<ZoneRnum>(std::distance(zone_table.begin(), it));

		// ищем место для вставки новой комнаты с конца, чтобы потом не вычитать 1 из результата
		auto insert_reverse_position = std::find_if(world.rbegin(), world.rend(), [rnum](RoomData *room) {
		  return rnum >= room->zone_rn;
		});
		auto insert_position =
			(insert_reverse_position == world.rend()) ? world.begin() : insert_reverse_position.base();

		top_of_world++;
		auto *new_room = new RoomData;
		world.insert(insert_position, new_room);
		new_room->zone_rn = rnum;
		new_room->vnum = last_room;
		new_room->set_name(std::string("Виртуальная комната"));
		new_room->description_num = RoomDescription::add_desc(std::string("Похоже, здесь вам делать нечего."));
		new_room->clear_flags();
		new_room->sector_type = ESector::kSecret;

		new_room->func = nullptr;
		new_room->contents = nullptr;
		new_room->track = nullptr;
		new_room->light = 0;
		new_room->fires = 0;
		new_room->gdark = 0;
		new_room->glight = 0;
		new_room->proto_script = std::make_shared<ObjData::triggers_list_t>();
	}
}

void RosolveWorldDoorToRoomVnumsToRnums() {
	for (auto room = kFirstRoom; room <= top_of_world; room++) {
		for (auto door = 0; door < EDirection::kMaxDirNum; door++) {
			if (world[room]->dir_option_proto[door]) {
				if (world[room]->dir_option_proto[door]->to_room() != kNowhere) {
					const auto to_room = GetRoomRnum(world[room]->dir_option_proto[door]->to_room());
					world[room]->dir_option_proto[door]->to_room(to_room);
					world[room]->dir_option[door]->to_room(to_room);
				}
			}
		}
	}
}

void SetZoneRnumForObjects() {
	for (size_t i = 0; i < obj_proto.size(); ++i) {
		obj_proto.zone(i, get_zone_rnum_by_obj_vnum(obj_proto[i]->get_vnum()));
	}
}

void SetZoneRnumForMobiles() {
	int i;
	for (i = 0; i <= top_of_mobt; ++i) {
		mob_index[i].zone = get_zone_rnum_by_mob_vnum(mob_index[i].vnum);
	}
}

void ResolveZoneCmdVnumArgsToRnums(ZoneData &zone_data) {
	int cmd_no, a, b, c, olda, oldb, oldc;
	char local_buf[128];
	int i;
	for (i = 0; i < zone_data.typeA_count; i++) {
		if (GetZoneRnum(zone_data.typeA_list[i]) == 0) {
			sprintf(local_buf,
					"SYSERROR: некорректное значение в typeA (%d) для зоны: %d",
					zone_data.typeA_list[i],
					zone_data.vnum);
			mudlog(local_buf, CMP, kLvlGreatGod, SYSLOG, true);
		}
	}
	for (i = 0; i < zone_data.typeB_count; i++) {
		if (GetZoneRnum(zone_data.typeB_list[i]) == 0) {
			sprintf(local_buf,
					"SYSERROR: некорректное значение в typeB (%d) для зоны: %d",
					zone_data.typeB_list[i],
					zone_data.vnum);
			mudlog(local_buf, CMP, kLvlGreatGod, SYSLOG, true);
		}
	}

	if (!zone_data.cmd) {
		return;
	}
	for (cmd_no = 0; zone_data.cmd[cmd_no].command != 'S'; cmd_no++) {
		auto &reset_cmd = zone_data.cmd[cmd_no];
		a = b = c = 0;
		olda = reset_cmd.arg1;
		oldb = reset_cmd.arg2;
		oldc = reset_cmd.arg3;
		switch (reset_cmd.command) {
			case 'M': a = reset_cmd.arg1 = GetMobRnum(reset_cmd.arg1);
				if (reset_cmd.arg2 <= 0 || reset_cmd.arg1 <= 0) {
					auto msg = fmt::format("SYSERROR: incorrect M cmd: zone {} rnum {}, stored {} room {}",
										   zone_data.vnum,
										   reset_cmd.arg1,
										   reset_cmd.arg2,
										   reset_cmd.arg3);
					mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
					reset_cmd.command = '*';
				} else {
					if (mob_index[reset_cmd.arg1].stored < reset_cmd.arg2) {
						mob_index[reset_cmd.arg1].stored = reset_cmd.arg2;
					}
					c = reset_cmd.arg3 = GetRoomRnum(reset_cmd.arg3);
				}
				break;
			case 'F': a = reset_cmd.arg1 = GetRoomRnum(reset_cmd.arg1);
				b = reset_cmd.arg2 = GetMobRnum(reset_cmd.arg2);
				c = reset_cmd.arg3 = GetMobRnum(reset_cmd.arg3);
				break;
			case 'O': a = reset_cmd.arg1 = GetObjRnum(reset_cmd.arg1);
				if (reset_cmd.arg3 != kNowhere)
					c = reset_cmd.arg3 = GetRoomRnum(reset_cmd.arg3);
				break;
			case 'G': [[fallthrough]];
			case 'E': a = reset_cmd.arg1 = GetObjRnum(reset_cmd.arg1);
				break;
			case 'P': a = reset_cmd.arg1 = GetObjRnum(reset_cmd.arg1);
				c = reset_cmd.arg3 = GetObjRnum(reset_cmd.arg3);
				break;
			case 'D': a = reset_cmd.arg1 = GetRoomRnum(reset_cmd.arg1);
				break;
			case 'R':    // rem obj from room
				a = reset_cmd.arg1 = GetRoomRnum(reset_cmd.arg1);
				b = reset_cmd.arg2 = GetObjRnum(reset_cmd.arg2);
				break;
			case 'Q': a = reset_cmd.arg1 = GetMobRnum(reset_cmd.arg1);
				break;
			case 'T':    // a trigger
				// designer's choice: convert this later
				// b = reset_cmd.arg2 = GetTriggerRnum(reset_cmd.arg2);
				b = reset_cmd.arg2 = GetTriggerRnum(reset_cmd.arg2);
				if (reset_cmd.arg1 == WLD_TRIGGER)
					c = reset_cmd.arg3 = GetRoomRnum(reset_cmd.arg3);
				break;
			case 'V':    // trigger variable assignment
				if (reset_cmd.arg1 == WLD_TRIGGER)
					b = reset_cmd.arg2 = GetRoomRnum(reset_cmd.arg2);
				break;
		}
		if (a < 0 || b < 0 || c < 0) {
			sprintf(local_buf, "Invalid vnum %d, cmd disabled", (a < 0) ? olda : ((b < 0) ? oldb : oldc));
			LogZoneError(zone_data, cmd_no, local_buf);
			reset_cmd.command = '*';
		}
	}
}

// resolve vnums into rnums in the zone reset tables
void ResolveZoneTableCmdVnumArgsToRnums() {
	for (auto &zone : zone_table) {
		ResolveZoneCmdVnumArgsToRnums(zone);
	}
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

void SetTestData(CharData *mob) {
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

int CompareSocials(const void *a, const void *b) {
	const struct SocialKeyword *a1, *b1;

	a1 = (const struct SocialKeyword *) a;
	b1 = (const struct SocialKeyword *) b;

	return (str_cmp(a1->keyword, b1->keyword));
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/

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
CharData *ReadMobile(MobVnum nr, int type) {                // and MobRnum
	int is_corpse = 0;
	MobRnum i;

	if (nr < 0) {
		is_corpse = 1;
		nr = -nr;
	}

	if (type == kVirtual) {
		if ((i = GetMobRnum(nr)) < 0) {
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (nullptr);
		}
	} else {
		i = nr;
	}
	if (i > top_of_mobt) {
		log("WARNING: Mobile rnum %d is above the list 'top of mobt' (%s %s %d)", i, __FILE__, __func__, __LINE__);
		return (nullptr);
	}
	auto *mob = new CharData(mob_proto[i]); //чет мне кажется что конструкции типа этой не принесут нам щастья...
	mob->set_normal_morph();
	mob->proto_script = std::make_shared<ObjData::triggers_list_t>();
	mob->script = std::make_shared<Script>();    //fill it in assign_triggers from proto_script
	character_list.push_front(mob);

	if (!mob->points.max_hit) {
		mob->points.max_hit = std::max(1, RollDices(mob->mem_queue.total, mob->mem_queue.stored) + mob->points.hit);
	} else {
		mob->points.max_hit = std::max(1, number(mob->points.hit, mob->mem_queue.total));
	}

	int mob_test_hp = get_test_hp(GetRealLevel(mob));
	if (GET_EXP(mob) > 0 && mob->points.max_hit < mob_test_hp) {
		mob->points.max_hit = mob_test_hp;
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
		mob_index[i].total_online++;
		assign_triggers(mob, MOB_TRIGGER);
	} else {
		mob->SetFlag(EMobFlag::kSummoned);
	}
	chardata_by_uid[mob->id] = mob;
	i = mob_index[i].zone;
	if (i != -1 && zone_table[i].under_construction) {
		// mobile принадлежит тестовой зоне
		mob->SetFlag(EMobFlag::kNoSummon);
	}

	if (city_guards::guardian_roster.contains(GET_MOB_VNUM(mob))) {
		mob->SetFlag(EMobFlag::kCityGuardian);
	}

	return (mob);
}

/**
// никакая это не копия, строковые и остальные поля с выделением памяти остаются общими
// мы просто отдаем константный указатель на прототип
 * \param type по дефолту VIRTUAL
 */
CObjectPrototype::shared_ptr GetObjectPrototype(ObjVnum nr, int type) {
	unsigned i = nr;
	if (type == kVirtual) {
		const int rnum = GetObjRnum(nr);
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

const int ZO_DEAD{9999};

// update zone ages, queue for reset if necessary, and dequeue when possible
void ZoneUpdate() {
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
			|| (zone_table[update_u->zone_to_reset].reset_mode != 3 && IsZoneEmpty(update_u->zone_to_reset))
			|| CanBeReset(update_u->zone_to_reset)) {
			zone_repop_list.push_back(update_u->zone_to_reset);
			std::stringstream out;
			out << "Auto zone reset: " << zone_table[update_u->zone_to_reset].name << " ("
				<< zone_table[update_u->zone_to_reset].vnum << ")";
			if (zone_table[update_u->zone_to_reset].reset_mode == 3) {
				for (auto i = 0; i < zone_table[update_u->zone_to_reset].typeA_count; i++) {
					//Ищем ZoneRnum по vnum
					for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
						if (zone_table[j].vnum ==
							zone_table[update_u->zone_to_reset].typeA_list[i]) {
							zone_repop_list.push_back(j);
							out << " ]\r\n[ Also resetting: " << zone_table[j].name << " ("
								<< zone_table[j].vnum << ")";
							break;
						}
					}
				}
			}
			std::stringstream ss;
			DecayObjectsOnRepop(zone_repop_list);
			ss << "В списке репопа: ";
			for (int &it : zone_repop_list) {
				ss << zone_table[it].vnum << " ";
				ResetZone(it);
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

bool CanBeReset(ZoneRnum zone) {
	if (zone_table[zone].reset_mode != 3)
		return false;
// проверяем себя
	if (!IsZoneEmpty(zone))
		return false;
// проверяем список B
	for (auto i = 0; i < zone_table[zone].typeB_count; i++) {
		//Ищем ZoneRnum по vnum
		for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
			if (zone_table[j].vnum == zone_table[zone].typeB_list[i]) {
				if (!zone_table[zone].typeB_flag[i] || !IsZoneEmpty(j)) {
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
				if (!IsZoneEmpty(j)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

void paste_mob(CharData *ch, RoomRnum room) {
	if (!ch->IsNpc() || ch->GetEnemy() || ch->GetPosition() < EPosition::kStun)
		return;
	if (IS_CHARMICE(ch)
		|| AFF_FLAGGED(ch, EAffect::kHorse)
		|| AFF_FLAGGED(ch, EAffect::kHold)
		|| (ch->extract_timer > 0)) {
		return;
	}
//	if (ch->IsFlagged(MOB_CORPSE))
//		return;
	if (room == kNowhere)
		return;

	bool time_ok = false;
	bool month_ok = false;
	bool need_move = false;
	bool no_month = true;
	bool no_time = true;

	if (ch->IsFlagged(EMobFlag::kAppearsDay)) {
		if (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight)
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsNight)) {
		if (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsFullmoon)) {
		if ((weather_info.sunlight == kSunSet ||
			weather_info.sunlight == kSunDark) &&
			(weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsWinter)) {
		if (weather_info.season == ESeason::kWinter)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsSpring)) {
		if (weather_info.season == ESeason::kSpring)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsSummer)) {
		if (weather_info.season == ESeason::kSummer)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsAutumn)) {
		if (weather_info.season == ESeason::kAutumn)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (need_move) {
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok) {
			if (world[room]->vnum != zone_table[world[room]->zone_rn].top)
				return;

			if (GET_LASTROOM(ch) == kNowhere) {
				ExtractCharFromWorld(ch, false, true);
				return;
			}

			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, GetRoomRnum(GET_LASTROOM(ch)));
		} else {
			if (world[room]->vnum == zone_table[world[room]->zone_rn].top)
				return;

			GET_LASTROOM(ch) = GET_ROOM_VNUM(room);
			RemoveCharFromRoom(ch);
			room = GetRoomRnum(zone_table[world[room]->zone_rn].top);

			if (room == kNowhere)
				room = GetRoomRnum(GET_LASTROOM(ch));

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
			if (world[room]->vnum != zone_table[world[room]->zone_rn].top) {
				return;
			}
			if (OBJ_GET_LASTROOM(obj) == kNowhere) {
				ExtractObjFromWorld(obj);
				return;
			}
			RemoveObjFromRoom(obj);
			PlaceObjToRoom(obj, GetRoomRnum(OBJ_GET_LASTROOM(obj)));
		} else {
			if (world[room]->vnum == zone_table[world[room]->zone_rn].top) {
				return;
			}
			// зачем сезонные переносить в виртуалку? спуржить нафиг
			if (!month_ok) {
				ExtractObjFromWorld(obj);
				return;
			}
			obj->set_room_was_in(GET_ROOM_VNUM(room));
			RemoveObjFromRoom(obj);
			room = GetRoomRnum(zone_table[world[room]->zone_rn].top);
			if (room == kNowhere) {
				room = GetRoomRnum(OBJ_GET_LASTROOM(obj));
			}
			PlaceObjToRoom(obj, room);
		}
	}
}

void PasteMobiles() {
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

void LogZoneError(const ZoneData &zone_data, int cmd_no, const char *message) {
	char local_buf[256];

	sprintf(buf, "SYSERR: zone file %d.zon: %s", zone_data.vnum, message);
	mudlog(local_buf, NRM, kLvlGod, SYSLOG, true);

	sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
			zone_data.cmd[cmd_no].command, zone_data.vnum, zone_data.cmd[cmd_no].line);
	mudlog(local_buf, NRM, kLvlGod, SYSLOG, true);
}

const int CHECK_SUCCESS{1};	// Выполить команду, только если предыдущая успешна
const int FLAG_PERSIST{2};	// Команда не должна изменить флаг

class ZoneReset {
 public:
  explicit ZoneReset(const ZoneRnum zone) : m_zone_rnum(zone) {}

  void Reset();

 private:
  [[nodiscard]] bool HandleZoneCmdQ(MobRnum rnum) const;

  // execute the reset command table of a given zone
  void ResetZoneEssential();

  ZoneRnum m_zone_rnum;
};

void ZoneReset::Reset() {
	utils::CExecutionTimer timer;

	if (GlobalObjects::stats_sender().ready()) {
		ResetZoneEssential();
		const auto execution_time = timer.delta();

		influxdb::Record record("zone_reset");
		record.add_tag("pulse", GlobalObjects::heartbeat().pulse_number());
		record.add_tag("zone", zone_table[m_zone_rnum].vnum);
		record.add_field("duration", execution_time.count());
		GlobalObjects::stats_sender().send(record);
	} else {
		ResetZoneEssential();
	}
}

bool ZoneReset::HandleZoneCmdQ(const MobRnum rnum) const {
	utils::CExecutionTimer overall_timer;
	bool extracted = false;

	utils::CExecutionTimer get_mobs_timer;
	Characters::list_t mobs;
	if (rnum > top_of_mobt) {
		log("WARNING: Mobile rnum %d is above the list 'top of mobt' (%s %s %d)", rnum, __FILE__, __func__, __LINE__);
		return extracted;
	}
	character_list.get_mobs_by_vnum(mob_index[rnum].vnum, mobs);
	const auto get_mobs_time = get_mobs_timer.delta();

	utils::CExecutionTimer extract_timer;
	for (const auto &mob : mobs) {
		if (!mob->IsFlagged(EMobFlag::kResurrected)) {
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

void ZoneReset::ResetZoneEssential() {
	int cmd_no;
	int cmd_tmp, obj_in_room_max, obj_in_room = 0;
	CharData *mob = nullptr, *leader = nullptr;
	ObjData *obj_to, *obj_room;
	CharData *tmob = nullptr;    // for trigger assignment
	ObjData *tobj = nullptr;    // for trigger assignment
	int last_state, curr_state;    // статус завершения последней и текущей команды

	log("[Reset] Start zone %s", zone_table[m_zone_rnum].name.c_str());
	if (!zone_table[m_zone_rnum].cmd) {
		log("No cmd, skiped");
		return;
	}
	for (auto rrn = zone_table[m_zone_rnum].RnumRoomsLocation.first;
		 rrn <= zone_table[m_zone_rnum].RnumRoomsLocation.second; rrn++) {
		RestoreRoomExitData(rrn);
	}
	//----------------------------------------------------------------------------
	last_state = 1;        // для первой команды считаем, что все ок

	for (cmd_no = 0; zone_table[m_zone_rnum].cmd != nullptr; cmd_no++) {
		const auto &zone_data = zone_table[m_zone_rnum];
		auto &reset_cmd = zone_table[m_zone_rnum].cmd[cmd_no];
		if (reset_cmd.command == '*') {
			continue;
		}
		if (reset_cmd.command == 'S') {
			break;
		}

		curr_state = 0;    // по умолчанию - неудачно
		if (!(reset_cmd.if_flag & CHECK_SUCCESS) || last_state) {
			// Выполняем команду, если предыдущая успешна или не нужно проверять
			switch (reset_cmd.command) {
				case 'M':
					// read a mobile
					// 'M' <flag> <MobVnum> <max_in_world> <RoomVnum> <max_in_room|-1>

					if (reset_cmd.arg3 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка загрузить моба в 0 комнату. (VNUM = %d, ZONE = %d)",
								mob_index[reset_cmd.arg1].vnum, zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					mob = nullptr;    //Добавлено Ладником
					if (mob_index[reset_cmd.arg1].total_online < reset_cmd.arg2 &&
						(reset_cmd.arg4 < 0 || CountMobsInRoom(reset_cmd.arg1, reset_cmd.arg3) < reset_cmd.arg4)) {
						mob = ReadMobile(reset_cmd.arg1, kReal);
						if (!mob) {
							sprintf(buf,
									"ZRESET: ошибка! моб %d  в зоне %d не существует",
									reset_cmd.arg1,
									zone_table[m_zone_rnum].vnum);
							mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
							return;
						}
						if (!mob_proto[mob->get_rnum()].get_role_bits().any()) {
							int rndlev = mob->GetLevel();
							rndlev += number(-2, +2);
							mob->set_level(std::max(1, rndlev));
						}
						PlaceCharToRoom(mob, reset_cmd.arg3);
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
					if (reset_cmd.arg1 >= kFirstRoom && reset_cmd.arg1 <= top_of_world) {
						for (const auto ch : world[reset_cmd.arg1]->people) {
							if (ch->IsNpc() && GET_MOB_RNUM(ch) == reset_cmd.arg2) {
								leader = ch;
							}
						}

						if (leader) {
							for (const auto ch : world[reset_cmd.arg1]->people) {
								if (ch->IsNpc()
									&& GET_MOB_RNUM(ch) == reset_cmd.arg3
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
					if (HandleZoneCmdQ(reset_cmd.arg1)) {
						curr_state = 1;
					}

					tobj = nullptr;
					tmob = nullptr;
					break;

				case 'O':
					// read an object
					// 'O' <flag> <ObjVnum> <max_in_world> <RoomVnum|-1> <load%|-1>
					// Проверка  - сколько всего таких же обьектов надо на эту клетку
					if (reset_cmd.arg3 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка загрузить объект в 0 комнату. (VNUM = %d, ZONE = %d)",
								obj_proto[reset_cmd.arg1]->get_vnum(), zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}
					for (cmd_tmp = 0, obj_in_room_max = 0; zone_data.cmd[cmd_tmp].command != 'S'; cmd_tmp++)
						if ((zone_data.cmd[cmd_tmp].command == 'O')
							&& (reset_cmd.arg1 == zone_data.cmd[cmd_tmp].arg1)
							&& (reset_cmd.arg3 == zone_data.cmd[cmd_tmp].arg3))
							obj_in_room_max++;
					// Теперь считаем склько их на текущей клетке
					for (obj_room = world[reset_cmd.arg3]->contents, obj_in_room = 0; obj_room;
						 obj_room = obj_room->get_next_content()) {
						if (reset_cmd.arg1 == GET_OBJ_RNUM(obj_room)) {
							obj_in_room++;
						}
					}
					// Теперь грузим обьект если надо
					if ((obj_proto.actual_count(reset_cmd.arg1) < GetObjMIW(reset_cmd.arg1)
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)
						&& (obj_in_room < obj_in_room_max)) {
						const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
						obj->set_vnum_zone_from(zone_table[world[reset_cmd.arg3]->zone_rn].vnum);

						if (!PlaceObjToRoom(obj.get(), reset_cmd.arg3)) {
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
					if ((obj_proto.actual_count(reset_cmd.arg1) < GetObjMIW(reset_cmd.arg1)
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)) {
						if (reset_cmd.arg2 > 0) {
							RoomRnum arg2 = GetRoomRnum(reset_cmd.arg2);
							if (arg2 == kNowhere) {
								LogZoneError(zone_data, cmd_no, "room in arg2 not found, command omited");
								break;
							}
							if (!(obj_to = GetObjByRnumInContent(reset_cmd.arg3, world[arg2]->contents))) {
								break;
							}
						} else {
							if (!(obj_to = SearchObjByRnum(reset_cmd.arg3))) {
								LogZoneError(zone_data, cmd_no, "target obj not found in word, command omited");
								break;
							}
						}
						if (GET_OBJ_TYPE(obj_to) != EObjType::kContainer) {
							LogZoneError(zone_data, cmd_no, "attempt put obj to non container, omited");
							reset_cmd.command = '*';
							break;
						}
						const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
						if (obj_to->get_in_room() != kNowhere) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_in_room()]->zone_rn].vnum);
						} else if (obj_to->get_worn_by()) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_worn_by()->in_room]->zone_rn].vnum);
						} else if (obj_to->get_carried_by()) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_carried_by()->in_room]->zone_rn].vnum);
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
						// reset_cmd.command = '*';
						break;
					}
					if ((obj_proto.actual_count(reset_cmd.arg1) < GetObjMIW(reset_cmd.arg1)
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)) {
						const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
						PlaceObjToInventory(obj.get(), mob);
						obj->set_vnum_zone_from(zone_table[world[mob->in_room]->zone_rn].vnum);
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
						// reset_cmd.command = '*';
						break;
					}
					if ((obj_proto.actual_count(reset_cmd.arg1) < obj_proto[reset_cmd.arg1]->get_max_in_world()
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)) {
						if (reset_cmd.arg3 < 0
							|| reset_cmd.arg3 >= EEquipPos::kNumEquipPos) {
							LogZoneError(zone_data, cmd_no, "invalid equipment pos number");
						} else {
							const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
							obj->set_vnum_zone_from(zone_table[world[mob->in_room]->zone_rn].vnum);
							obj->set_in_room(mob->in_room);
							load_otrigger(obj.get());
							if (wear_otrigger(obj.get(), mob, reset_cmd.arg3)) {
								obj->set_in_room(kNowhere);
								EquipObj(mob, obj.get(), reset_cmd.arg3, CharEquipFlags());
							} else {
								PlaceObjToInventory(obj.get(), mob);
							}
							if (obj->get_carried_by() != mob && obj->get_worn_by() != mob) {
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

					if (reset_cmd.arg1 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка удалить объект из 0 комнаты. (VNUM = %d, ZONE = %d)",
								obj_proto[reset_cmd.arg2]->get_vnum(), zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					if (const auto obj = GetObjByRnumInContent(reset_cmd.arg2, world[reset_cmd.arg1]->contents)) {
						ExtractObjFromWorld(obj);
						curr_state = 1;
					}
					tmob = nullptr;
					tobj = nullptr;
					break;

				case 'D':
					// set state of door
					// 'D' <flag> <RoomVnum> <door_pos> <door_state>

					if (reset_cmd.arg1 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка установить двери в 0 комнате. (ZONE = %d)",
								zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}
					if (reset_cmd.arg2 < 0 || reset_cmd.arg2 >= EDirection::kMaxDirNum ||
						(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2] == nullptr)) {
						LogZoneError(zone_data, cmd_no, "door does not exist, command disabled");
						reset_cmd.command = '*';
					} else {
						REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
								   EExitFlag::kBrokenLock);
						switch (reset_cmd.arg3) {
							case 0:
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kLocked);
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kClosed);
								break;
							case 1:
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kClosed);
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kLocked);
								break;
							case 2:
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kLocked);
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kClosed);
								break;
							case 3:
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kHidden);
								break;
							case 4:
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
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
					if (reset_cmd.arg1 == MOB_TRIGGER && tmob) {
						auto trig = read_trigger(reset_cmd.arg2);
						if (!add_trigger(SCRIPT(tmob).get(), trig, -1)) {
							ExtractTrigger(trig);
						}
						curr_state = 1;
					} else if (reset_cmd.arg1 == OBJ_TRIGGER && tobj) {
						auto trig = read_trigger(reset_cmd.arg2);
						if (!add_trigger(tobj->get_script().get(), trig, -1)) {
							ExtractTrigger(trig);
						}
						curr_state = 1;
					} else if (reset_cmd.arg1 == WLD_TRIGGER) {
						if (reset_cmd.arg3 > kNowhere) {
							auto trig = read_trigger(reset_cmd.arg2);
							if (!add_trigger(world[reset_cmd.arg3]->script.get(), trig, -1)) {
								ExtractTrigger(trig);
							}
							curr_state = 1;
						}
					}
					break;

				case 'V':
					// 'V' <flag> <trigger_type> <RoomVnum> <context> <var_name> <var_value>
					if (reset_cmd.arg1 == MOB_TRIGGER && tmob) {
						if (!SCRIPT(tmob)->has_triggers()) {
							LogZoneError(zone_data, cmd_no, "Attempt to give variable to scriptless mobile");
						} else {
							add_var_cntx(SCRIPT(tmob)->global_vars, reset_cmd.sarg1, reset_cmd.sarg2, reset_cmd.arg3);
							curr_state = 1;
						}
					} else if (reset_cmd.arg1 == OBJ_TRIGGER && tobj) {
						if (!tobj->get_script()->has_triggers()) {
							LogZoneError(zone_data, cmd_no, "Attempt to give variable to scriptless object");
						} else {
							add_var_cntx(tobj->get_script()->global_vars,
										 reset_cmd.sarg1,
										 reset_cmd.sarg2,
										 reset_cmd.arg3);
							curr_state = 1;
						}
					} else if (reset_cmd.arg1 == WLD_TRIGGER) {
						if (reset_cmd.arg2 < kFirstRoom || reset_cmd.arg2 > top_of_world) {
							LogZoneError(zone_data, cmd_no, "Invalid room number in variable assignment");
						} else {
							if (!SCRIPT(world[reset_cmd.arg2])->has_triggers()) {
								LogZoneError(zone_data, cmd_no, "Attempt to give variable to scriptless room");
							} else {
								add_var_cntx(world[reset_cmd.arg2]->script->global_vars,
											 reset_cmd.sarg1,
											 reset_cmd.sarg2,
											 reset_cmd.arg3);
								curr_state = 1;
							}
						}
					}
					break;

				default: LogZoneError(zone_data, cmd_no, "unknown cmd in reset table; cmd disabled");
					reset_cmd.command = '*';
					break;
			}
		}

		if (!(reset_cmd.if_flag & FLAG_PERSIST)) {
			// команда изменяет флаг
			last_state = curr_state;
		}
	}
	if (zone_table[m_zone_rnum].used) {
		zone_table[m_zone_rnum].count_reset++;
	}

	zone_table[m_zone_rnum].age = 0;
	zone_table[m_zone_rnum].used = false;
	celebrates::ProcessCelebrates(zone_table[m_zone_rnum].vnum);
	int rnum_start = 0;
	int rnum_stop = 0;

	if (GetZoneRooms(m_zone_rnum, &rnum_start, &rnum_stop)) {
		// все внутренние резеты комнат зоны теперь идут за один цикл
		// резет порталов теперь тут же и переписан, чтобы не гонять по всем румам, ибо жрал половину времени резета -- Krodo
		for (int rnum = rnum_start; rnum <= rnum_stop; rnum++) {
			RoomData *room = world[rnum];
			reset_wtrigger(room);
			int sect = real_sector(GetRoomRnum(room->vnum));
			if (!(sect == ESector::kWaterSwim || sect == ESector::kWaterNoswim || sect == ESector::kOnlyFlying)) {
				im_reset_room(room, zone_table[m_zone_rnum].level, zone_table[m_zone_rnum].type);
			}
			while (room_spells::IsRoomAffected(world[rnum], ESpell::kPortalTimer)) {
				RemovePortalGate(rnum);
			}
/*
			while (room_spells::IsRoomAffected(world[rnum], ESpell::kPortalTimer)) {
				auto aff = room_spells::FindAffect(world[rnum], ESpell::kPortalTimer);
				const RoomRnum to_room = (*aff)->modifier;

				if (aff != world[rnum]->affected.end()) {
					room_spells::RoomRemoveAffect(world[rnum], aff);
					act("Пентаграмма была разрушена.", false, world[rnum]->first_character(), 0, 0, kToRoom);
					act("Пентаграмма была разрушена.", false, world[rnum]->first_character(), 0, 0, kToChar);
				}
				aff = room_spells::FindAffect(world[to_room], ESpell::kPortalTimer);
				if (aff != world[to_room]->affected.end()) {
					room_spells::RoomRemoveAffect(world[to_room], aff);
					act("Пентаграмма была разрушена.", false, world[to_room]->first_character(), 0, 0, kToRoom);
					act("Пентаграмма была разрушена.", false, world[to_room]->first_character(), 0, 0, kToChar);
				}
			}
*/
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
	log("[Reset] Stop zone %s", zone_table[m_zone_rnum].name.c_str());
	after_reset_zone(m_zone_rnum);
}

void ResetZone(ZoneRnum zone) {
	ZoneReset zreset(zone);
	zreset.Reset();
}

// Ищет RNUM первой и последней комнаты зоны
// Еси возвращает 0 - комнат в зоне нету
int GetZoneRooms(ZoneRnum zrn, int *first, int *last) {
	*first = zone_table[zrn].RnumRoomsLocation.first;
	*last = zone_table[zrn].RnumRoomsLocation.second;

	if (*first <= 0)
		return 0;
	return 1;
}

// for use in ResetZone; return true if zone 'nr' is free of PC's
bool IsZoneEmpty(ZoneRnum zone_nr, bool debug) {
	int rnum_start, rnum_stop;
	static ZoneRnum last_zone_nr = 0;
	static bool result;
	if (debug) {
		sprintf(buf, "Is empty check. Зона %d", zone_table[zone_nr].vnum);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	}
	if (last_zone_nr == zone_nr) {
		if (debug) {
			sprintf(buf,
					"Is empty repeat. Зона %d, прошлый запрос %d",
					zone_table[zone_nr].vnum,
					zone_table[last_zone_nr].vnum);
			mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
		}
		return result;
	}
	last_zone_nr = zone_nr;
	for (auto i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING)
			continue;
		if (i->character->in_room == kNowhere)
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

int CountMobsInRoom(int m_num, int r_num) {
	int count = 0;

	for (const auto &ch : world[r_num]->people) {
		if (m_num == GET_MOB_RNUM(ch)
			&& !ch->IsFlagged(EMobFlag::kResurrected)) {
			count++;
		}
	}
	return count;
}

/*************************************************************************
*  stuff related to the save/load player system                          *
*************************************************************************/

long CmpPtableByName(char *name, int len) {
	len = std::min(len, static_cast<int>(strlen(name)));
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): I am not sure but I guess that linear search is not the best solution here.
	 * TODO: make map helper (MAPHELPER). */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		const char *pname = player_table[i].name();
		if (!strn_cmp(pname, arg, std::min(len, static_cast<int>(strlen(pname))))) {
			return static_cast<long>(i);
		}
	}
	return -1;
}

long GetPlayerTablePosByName(const char *name) {
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

long GetPtableByUnique(long unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (std::size_t i = 0; i < player_table.size(); i++) {
		if (player_table[i].unique == unique) {
			return static_cast<long>(i);
		}
	}
	return 0;
}

long GetPlayerIdByName(char *name) {
	one_argument(name, arg);
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (const auto &i : player_table) {
		if (!str_cmp(i.name(), arg)) {
			return (i.id());
		}
	}

	return (-1);
}

int GetPlayerUidByName(int id) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.id() == id) {
			return i.unique;
		}
	}
	return -1;
}

const char *GetNameById(long id) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (const auto &i : player_table) {
		if (i.id() == id) {
			return i.name();
		}
	}
	return "";
}

const char *GetPlayerNameByUnique(int unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.unique == unique) {
			return i.name();
		}
	}
	return nullptr;
}

int GetLevelByUnique(long unique) {
	int level = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.unique == unique) {
			level = i.level;
		}
	}
	return level;
}

long GetLastlogonByUnique(long unique) {
	long time = 0;

	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.unique == unique) {
			time = i.last_logon;
		}
	}
	return time;
}

int IsCorrectUnique(int unique) {
	/* Anton Gorev (2015/12/29): see (MAPHELPER) comment. */
	for (auto &i : player_table) {
		if (i.unique == unique) {
			return true;
		}
	}

	return false;
}

void RecreateSaveinfo(size_t number) {
	delete player_table[number].timer;
	NEWCREATE(player_table[number].timer);
}

/**
* Можно канеш просто в get_skill иммам возвращать 100 или там 200, но тут зато можно
* потестить че-нить с возможностью покачать скилл во время игры иммом.
*/
void SetGodSkills(CharData *ch) {
	for (auto i = ESkill::kFirst; i <= ESkill::kLast; ++i) {
		if (MUD::Skills().IsValid(i)) {
			ch->set_skill(i, 200);
		}
	}
}

// по умолчанию reboot = 0 (пользуется только при ребуте)
int LoadPlayerCharacter(const char *name, CharData *char_element, int load_flags) {
	const auto player_i = char_element->load_char_ascii(name, load_flags);
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
 * str_dup()'d copy on ->showstr_head, and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector, but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int AllocateBufferForFile(const char *name, char **destination_buf) {
	char temp[kMaxExtendLength];
	DescriptorData *in_use;

	for (in_use = descriptor_list; in_use; in_use = in_use->next) {
		if (in_use->showstr_vector && *in_use->showstr_vector == *destination_buf) {
			return (-1);
		}
	}

	// Lets not free() what used to be there unless we succeeded.
	if (ReadFileToBuffer(name, temp) < 0) {
		return (-1);
	}

	if (*destination_buf) {
		free(*destination_buf);
	}

	*destination_buf = str_dup(temp);
	return (0);
}

// read contents of a text file, and place in destination_buf
int ReadFileToBuffer(const char *name, char *destination_buf) {
	FILE *fl;
	char tmp[READ_SIZE + 3];

	*destination_buf = '\0';

	if (!(fl = fopen(name, "r"))) {
		log("SYSERR: reading %s: %s", name, strerror(errno));
		sprintf(destination_buf + strlen(destination_buf), "Error: file '%s' is empty.\r\n", name);
		return (0);
	}
	do {
		const char *dummy = fgets(tmp, READ_SIZE, fl);
		UNUSED_ARG(dummy);

		tmp[strlen(tmp) - 1] = '\0';    // take off the trailing \n
		strcat(tmp, "\r\n");

		if (!feof(fl)) {
			if (strlen(destination_buf) + strlen(tmp) + 1 > kMaxExtendLength) {
				log("SYSERR: %s: string too big (%d max)", name, kMaxStringLength);
				*destination_buf = '\0';
				return (-1);
			}
			strcat(destination_buf, tmp);
		}
	} while (!feof(fl));

	fclose(fl);

	return (0);
}

ObjRnum GetObjRnum(ObjVnum vnum) {
	return obj_proto.get_rnum(vnum);
}

ZoneRnum GetZoneRnum(ZoneVnum vnum) {
	ZoneRnum bot, top, mid;

	bot = 0;
	top = static_cast<ZoneRnum>(zone_table.size() - 1);
	for (;;) {
		mid = (bot + top) / 2;
		if (zone_table[mid].vnum == vnum)
			return (mid);
		if (bot >= top)
			return (kNowhere);
		if (zone_table[mid].vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// returns the real number of the room with given virtual number
RoomRnum GetRoomRnum(RoomVnum vnum) {
	RoomRnum bot, top, mid;
/*	for (int i = 0; i <= top_of_world; i++) {
		if (world[i]->vnum == vnum)
			return (i);
	}
	return 0;
*/
	bot = 1;        // 0 - room is kNowhere
	top = top_of_world;
	// perform binary search on world-table
	for (;;) {
		mid = (bot + top) / 2;
		if (world[mid]->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (kNowhere);
		if (world[mid]->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

TrgRnum GetTriggerRnum(TrgVnum vnum) {
/*	int rnum;

	for (rnum = 0; rnum <= top_of_trigt; rnum++) {
		if (trig_index[rnum]->vnum == vnum)
			break;
	}

	if (rnum > top_of_trigt)
		rnum = -1;
	return (rnum);
*/
	TrgRnum bot, top, mid;
	bot = 0;
	top = top_of_trigt - 1;
	// perform binary search on world-table
	for (;;) {
		mid = (bot + top) / 2;
		if (trig_index[mid]->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if (trig_index[mid]->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// returns the real number of the monster with given virtual number
MobRnum GetMobRnum(MobVnum vnum) {
//	log("real mobile vnum %d top %d", vnum, top_of_mobt);
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
bool MustBeDeleted(CharData *short_ch) {
	int ci, timeout;

	if (short_ch->IsFlagged(EPlrFlag::kNoDelete)) {
		return false;
	}

	if (short_ch->IsFlagged(EPlrFlag::kDeleted)) {
		return true;
	}

	if (short_ch->get_remort()) {
		return false;
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
		if ((time(nullptr) - LAST_LOGON(short_ch)) > timeout) {
			return true;
		}
	}

	return false;
}

// данная функция работает с неполностью загруженным персонажем
// подробности в комментарии к load_char_ascii
void ActualizePlayersIndex(char *name) {
	int deleted;
	char filename[kMaxStringLength];

	if (get_filename(name, filename, kPlayersFile)) {
		Player t_short_ch;
		Player *short_ch = &t_short_ch;
		deleted = 1;
		// персонаж загружается неполностью
		if (LoadPlayerCharacter(name, short_ch, ELoadCharFlags::kReboot) > -1) {
			// если чар удален или им долго не входили, то не создаем для него запись
			if (!MustBeDeleted(short_ch)) {
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
				if (short_ch->IsFlagged(EPlrFlag::kDeleted)) {
					element.last_logon = -1;
					element.activity = -1;
				} else {
					element.last_logon = LAST_LOGON(short_ch);
					element.activity = number(0, kObjectSaveActivity - 1);
				}

#ifdef TEST_BUILD
				log("entry: char:%s level:%d mail:%s ip:%s", element.name(), element.level, element.mail, element.last_ip);
#endif

				top_idnum = std::max(top_idnum, GET_IDNUM(short_ch));
				TopPlayer::Refresh(short_ch, true);

				log("Adding new player %s", element.name());
				player_table.Append(element);
			} else {
				log("Delete %s from account email: %s",
					GET_NAME(short_ch),
					short_ch->get_account()->get_email().c_str());
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

void BuildPlayerIndexNew() {
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

		if (!player_table.IsPlayerExists(playername)) {
			ActualizePlayersIndex(playername);
		}
	}

	fclose(players);

	player_table.GetNameAdviser().init();
}

void FlushPlayerIndex() {
	FILE *players;
	char name[kMaxStringLength];

	if (!(players = fopen(LIB_PLRS "players.lst", "w+"))) {
		log("Can't save players list...");
		return;
	}

	std::size_t saved = 0;
	for (auto &i : player_table) {
		if (!i.name() || !*i.name()) {
			continue;
		}

		++saved;
		sprintf(name, "%s %d %d %d %d\n",
				i.name(),
				i.id(), i.unique, i.level, i.last_logon);
		fputs(name, players);
	}
	fclose(players);
	log("Сохранено индексов %zd (считано при загрузке %zd)", saved, player_table.size());
}

void LoadGlobalUid() {
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

Rooms::~Rooms() {
	log("~Rooms()");
	for (auto &i : *this) {
		delete i;
	}
}

const std::size_t PlayersIndex::NOT_FOUND = ~static_cast<std::size_t>(0);

PlayersIndex::~PlayersIndex() {
	log("~PlayersIndex()");
}

std::size_t PlayersIndex::Append(const PlayerIndexElement &element) {
	const auto index = size();

	push_back(element);
	m_id_to_index.emplace(element.id(), index);
	AddNameToIndex(element.name(), index);

	return index;
}

std::size_t PlayersIndex::GetRnumByName(const char *name) const {
	const auto i = m_name_to_index.find(name);
	if (i != m_name_to_index.end()) {
		return i->second;
	}

	return NOT_FOUND;
}

void PlayersIndex::SetName(const std::size_t index, const char *name) {
	const auto i = m_name_to_index.find(operator[](index).name());
	m_name_to_index.erase(i);
	operator[](index).set_name(name);
	AddNameToIndex(name, index);
}

void PlayersIndex::AddNameToIndex(const char *name, const std::size_t index) {
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
