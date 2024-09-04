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

#ifndef DB_H_
#define DB_H_

#include "boot/boot_constants.h"
#include "conf.h"    // to get definition of build type: (CIRCLE_AMIGA|CIRCLE_UNIX|CIRCLE_WINDOWS|CIRCLE_ACORN|CIRCLE_VMS)
#include "administration/name_adviser.h"
#include "obj_save.h"
#include "entities/obj_data.h"
#include "entities/player_i.h"
#include "structs/descriptor_data.h"
#include "structs/structs.h"

#include <map>
#include <list>
#include <memory>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.
class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

// public procedures in db.cpp
RoomRnum GetRoomRnum(RoomVnum vnum);
ZoneRnum GetZoneRnum(ZoneVnum zvn);
MobRnum GetMobRnum(MobVnum vnum);
ObjRnum GetObjRnum(ObjVnum vnum);
void ExtractTagFromArgument(char *argument, char *tag);
void BootMudDataBase();
void ZoneUpdate();
bool CanBeReset(ZoneRnum zone);
long GetPlayerIdByName(char *name);
int GetPlayerUidByName(int id);
long CmpPtableByName(char *name, int len);
const char *GetNameById(long id);
const char *GetPlayerNameByUnique(int unique);
int GetLevelByUnique(long unique);
long GetLastlogonByUnique(long unique);
long GetPtableByUnique(long unique);
int GetZoneRooms(int, int *, int *);
void ZoneTrafficSave();
void ResetZone(ZoneRnum zone);
void LoadSheduledReboot();
void initIngredientsMagic();
void InitZoneTypes();
void InitPortals();
int AllocateBufferForFile(const char *name, char **destination_buf);
int LoadPlayerCharacter(const char *name, CharData *char_element, int load_flags);
CharData *ReadMobile(MobVnum nr, int type);
int IsCorrectUnique(int unique);
bool IsTimerUnlimited(const CObjectPrototype *obj);
void SaveGlobalUID();
void FlushPlayerIndex();
bool IsZoneEmpty(ZoneRnum zone_nr, bool debug = false);
void TrigCommandsConvert(ZoneRnum zrn_from, ZoneRnum zrn_to, ZoneRnum replacer_zrn);

const int kReal		= 0;
const int kVirtual	= 1 << 0;

CObjectPrototype::shared_ptr GetObjectPrototype(ObjVnum nr, int type = kVirtual);

// structure for the reset commands
struct combat_list_element {
	CharData *ch;
	bool deleted;
};

struct reset_com {
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
	char command;        // current command

	int if_flag;        // 4 modes of command execution
	int arg1;        //
	int arg2;        // Arguments to the command
	int arg3;        //
	int arg4;
	int line;        // line number this command appears on
	char *sarg1;        // string argument
	char *sarg2;        // string argument
};

struct TreasureCase {
	ObjVnum vnum;
	int drop_chance;
	std::vector<ObjVnum> vnum_objs; // внумы шмоток, которые выпадают из кейса
};

// для экстраффектов в random_obj
struct ExtraAffects {
	int number; // номер экстрааафетка
	int min_val; // минимальное значение
	int max_val; // максимальное значение
	int chance; // вероятность того, что данный экстраффект будет на шмотке
};

struct QuestBodrichRewards {
	int level;
	int vnum;
	int money;
	int exp;
};

class QuestBodrich {
 public:
	QuestBodrich();

 private:
	void LoadMobs();
	void LoadObjs();
	void LoadRewards();

	// здесь храним предметы для каждого класса
	std::map<int, std::vector<int>> objs;
	// здесь храним мобов
	std::map<int, std::vector<int>> mobs;
	// а здесь награды
	std::map<int, std::vector<QuestBodrichRewards>> rewards;
};

struct City {
	std::string name; // имя города
	std::vector<int> vnums; // номера зон, которые принадлежат городу
	int rent_vnum; // внум ренты города
};

class RandomObj {
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
struct reset_q_element {
	ZoneRnum zone_to_reset;    // ref to zone_data
	struct reset_q_element *next;
};

// structure for the update queue
struct reset_q_type {
	struct reset_q_element *head;
	struct reset_q_element *tail;
};

const int kObjectSaveActivity = 300;
const int kPlayerSaveActivity = 305;
const int kMaxSavedItems = 1000;

class PlayerIndexElement {
 public:
	PlayerIndexElement(int id, const char *name);

	char *mail;
	char *last_ip;
	int unique;
	int level;
	int remorts;
	ECharClass plr_class;
	int last_logon;
	int activity;        // When player be saved and checked
	SaveInfo *timer;

	[[nodiscard]] const char *name() const { return m_name; }
	[[nodiscard]] int id() const { return m_id; }

	void set_name(const char *name);
	void set_id(const int id) { m_id = id; }

 private:
	int m_id;
	const char *m_name;
};

extern RoomRnum top_of_world;
extern std::unordered_map<long, CharData *> chardata_by_uid;

void AddTrigIndexEntry(int nr, Trigger *trig);
extern IndexData **trig_index;

#ifndef __CONFIG_C__
extern char const *OK;
extern char const *NOPERSON;
extern char const *NOEFFECT;
#endif

// external variables

extern const int sunrise[][2];
extern const int Reverse[];

// external vars
//extern std::list<CharData *> combat_list;

#include <vector>
#include <deque>

class Rooms : public std::vector<RoomData *> {
 public:
	static constexpr int UNDEFINED_ROOM_VNUM = -1;
	~Rooms();
};

extern Rooms &world;

extern IndexData *mob_index;
extern MobRnum top_of_mobt;

inline ObjVnum GET_OBJ_VNUM(const CObjectPrototype *obj) { return obj->get_vnum(); }

extern CharData *mob_proto;
extern const char *MENU;

extern struct Portal *portals_list;
extern TimeInfoData time_info;

extern int ConvertDrinkconSkillField(CObjectPrototype *obj, bool proto);

void PasteMobiles();

extern RoomRnum r_helled_start_room;
extern RoomRnum r_mortal_start_room;
extern RoomRnum r_immort_start_room;
extern RoomRnum r_named_start_room;
extern RoomRnum r_unreg_start_room;

long GetPlayerTablePosByName(const char *name);
void FreeAlias(struct alias_data *a);

class PlayersIndex : public std::vector<PlayerIndexElement> {
 public:
	using parent_t = std::vector<PlayerIndexElement>;
	using parent_t::operator[];
	using parent_t::size;

	static const std::size_t NOT_FOUND;

	~PlayersIndex();

	std::size_t Append(const PlayerIndexElement &element);
	bool IsPlayerExists(const int id) const { return m_id_to_index.find(id) != m_id_to_index.end(); }
	bool IsPlayerExists(const char *name) const { return NOT_FOUND != GetRnumByName(name); }
	std::size_t GetRnumByName(const char *name) const;
	void SetName(std::size_t index, const char *name);

	NameAdviser &GetNameAdviser() { return m_name_adviser; }

 private:
	class hasher {
	 public:
		std::size_t operator()(const std::string &value) const;
	};

	class equal_to {
	 public:
		bool operator()(const std::string &left, const std::string &right) const;
	};

	using id_to_index_t = std::unordered_map<int, std::size_t>;
	using name_to_index_t = std::unordered_map<std::string, std::size_t, hasher, equal_to>;

	void AddNameToIndex(const char *name, std::size_t index);

	id_to_index_t m_id_to_index;
	name_to_index_t m_name_to_index;
	// contains free names which are available for new players
	NameAdviser m_name_adviser;
};

extern PlayersIndex &player_table;

extern long top_idnum;

bool IsPlayerExists(long id);

inline SaveInfo *SAVEINFO(const size_t number) {
	return player_table[number].timer;
}

inline void ClearSaveinfo(const size_t number) {
	delete player_table[number].timer;
	player_table[number].timer = nullptr;
}

void RecreateSaveinfo(size_t number);
void SetGodSkills(CharData *ch);
void CheckRoomForIncompatibleFlags(int rnum);
void SetTestData(CharData *mob);
void SetZoneMobLevel();

class GameLoader {
 public:
	GameLoader() = default;

	static void BootWorld();
	static void BootIndex(EBootType mode);

 private:
	static void PrepareGlobalStructures(const EBootType mode, const int rec_count);
};

extern GameLoader world_loader;

#endif // DB_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
