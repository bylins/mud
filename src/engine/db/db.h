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

#include "world_data_source.h"

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
#include <set>
#include <list>
#include <memory>
#include <vector>
#include <deque>

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
int GetZoneRooms(int, int *, int *);
void ZoneTrafficSave();
void ResetZone(ZoneRnum zone);
void LoadSheduledReboot();
void initIngredientsMagic();
void InitZoneTypes();
int AllocateBufferForFile(const char *name, char **destination_buf);
int LoadPlayerCharacter(const char *name, CharData *char_element, int load_flags);
CharData *ReadMobile(MobVnum nr, int type);
void SaveGlobalUID();
void FlushPlayerIndex();
bool IsZoneEmpty(ZoneRnum zone_nr, bool debug = false);
int get_filename(const char *orig_name, char *filename, int mode);
CharData *find_char(long n);
CharData *find_pc(long n);
void CharTimerUpdate();

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

extern RoomRnum top_of_world;
extern std::map<long, CharData *> chardata_by_uid;
extern std::set<CharData *> chardata_wait_list;
extern std::set<CharData *> chardata_cooldown_list;

void AddTrigIndexEntry(int nr, Trigger *trig);
extern IndexData **trig_index;


// external variables

extern const int sunrise[][2];
extern const int Reverse[];

// external vars
//extern std::list<CharData *> combat_list;


template<typename T>
class UniqueList: private std::list<T> {
private:
	using base_t = std::list<T>;
public:
	using iterator = typename base_t::iterator;
	void push_back(const T& value) {
		if (std::find(base_t::begin(), base_t::end(), value) == base_t::end()) {
			base_t::push_back(value);
		}
	}
	iterator begin() {
		return base_t::begin();
	}
	iterator end() {
		return base_t::end();
	}
};
extern void DecayObjectsOnRepop(UniqueList<ZoneRnum> &zone_list);

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

extern long top_idnum;

void SetGodSkills(CharData *ch);
void CheckRoomForIncompatibleFlags(int rnum);
void SetTestData(CharData *mob);
void SetZoneMobLevel();

class GameLoader {
 public:
	GameLoader() = default;

	static void BootWorld(std::unique_ptr<world_loader::IWorldDataSource> data_source = nullptr);
	static void BootIndex(EBootType mode);

 private:
	static void PrepareGlobalStructures(const EBootType mode, const int rec_count);
};

extern GameLoader game_loader;

#endif // DB_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
