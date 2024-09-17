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

#include "engine/boot/boot_constants.h"
#include "engine/core/conf.h"    // to get definition of build type: (CIRCLE_AMIGA|CIRCLE_UNIX|CIRCLE_WINDOWS|CIRCLE_ACORN|CIRCLE_VMS)
#include "administration/name_adviser.h"
#include "obj_save.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/player_i.h"
#include "engine/network/descriptor_data.h"
#include "engine/structs/structs.h"
#include "gameplay/mechanics/weather.h"

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
int AllocateBufferForFile(const char *name, char **destination_buf);
int LoadPlayerCharacter(const char *name, CharData *char_element, int load_flags);
CharData *ReadMobile(MobVnum nr, int type);
int IsCorrectUnique(int unique);
void SaveGlobalUID();
void FlushPlayerIndex();
bool IsZoneEmpty(ZoneRnum zone_nr, bool debug = false);
int get_filename(const char *orig_name, char *filename, int mode);
CharData *find_char(long n);
CharData *find_pc(long n);

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

// для экстраффектов в random_obj
struct ExtraAffects {
	int number; // номер экстрааафетка
	int min_val; // минимальное значение
	int max_val; // максимальное значение
	int chance; // вероятность того, что данный экстраффект будет на шмотке
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

// =====================================================================================================================

struct IndexData {
  IndexData() : vnum(0), total_online(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}
  explicit IndexData(int _vnum)
	  : vnum(_vnum), total_online(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}

  Vnum vnum;            // virtual number of this mob/obj       //
  int total_online;        // number of existing units of this mob/obj //
  int stored;        // number of things in rent file            //
  int (*func)(CharData *, void *, int, char *);
  char *farg;        // string argument for special function     //
  Trigger *proto;    // for triggers... the trigger     //
  int zone;            // mob/obj zone rnum //
  size_t set_idx; // индекс сета в obj_sets::set_list, если != -1
};

// =====================================================================================================================

class PlayerIndexElement {
 public:
	explicit PlayerIndexElement(const char *name);

	char *mail;
	char *last_ip;
	int level;
	int remorts;
	ECharClass plr_class;
	int last_logon;
	int activity;        // When player be saved and checked
	SaveInfo *timer;

	[[nodiscard]] const char *name() const { return m_name; }
	[[nodiscard]] long uid() const { return m_uid_; }

	void set_name(const char *name);
	void set_uid(const long _) { m_uid_ = _; }
 private:
	int m_uid_;
	const char *m_name;
};

extern RoomRnum top_of_world;
extern std::unordered_map<long, CharData *> chardata_by_uid;

void AddTrigIndexEntry(int nr, Trigger *trig);
extern IndexData **trig_index;


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
