/* ************************************************************************
*   File: handler.h                                     Part of Bylins    *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef HANDLER_H_
#define HANDLER_H_

#include "entities/char_data.h"
#include "structs/structs.h"    // there was defined type "byte" if it had been missing
#include "structs/flags.hpp"

struct RoomData;

const int kLightNo = 0;
const int kLightYes = 1;
const int kLightUndef = 2;

enum class CharEquipFlag : uint8_t {
	no_cast,	// no spell casting
	skip_total,	// no total affect update
	show_msg	// show wear and activation messages
};

FLAGS_DECLARE_FROM_ENUM(CharEquipFlags, CharEquipFlag);
FLAGS_DECLARE_OPERATORS(CharEquipFlags, CharEquipFlag);

int get_room_sky(int rnum);
int equip_in_metall(CharData *ch);
bool IsAwakeOthers(CharData *ch);

void check_light(CharData *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef);

// Resistance calculate //
int ApplyResist(CharData *ch, int resist_type, int effect);
int GetResisTypeWithSpellClass(int spell_class);
int GetResistType(int spellnum);

// handling the affected-structures //
void ImposeTimedFeat(CharData *ch, TimedFeat *timed);
void ExpireTimedFeat(CharData *ch, TimedFeat *timed);
int IsTimed(CharData *ch, int feat);
void timed_to_char(CharData *ch, struct TimedSkill *timed);
void timed_from_char(CharData *ch, struct TimedSkill *timed);
int IsTimedBySkill(CharData *ch, ESkill id);
void decreaseFeatTimer(CharData *ch, int featureID);

// utility //
char *money_desc(int amount, int padis);
ObjData::shared_ptr create_money(int amount);
char *fname(const char *namelist);
int get_number(char **name);
int get_number(std::string &name);

RoomVnum get_room_where_obj(ObjData *obj, bool deep = false);

// ******** objects *********** //
bool equal_obj(ObjData *obj_one, ObjData *obj_two);
void obj_to_char(ObjData *object, CharData *ch);
void obj_from_char(ObjData *object);

void equip_char(CharData *ch, ObjData *obj, int pos, const CharEquipFlags& equip_flags);
ObjData *unequip_char(CharData *ch, int pos, const CharEquipFlags& equip_flags);
int invalid_align(CharData *ch, ObjData *obj);

ObjData *get_obj_in_list(char *name, ObjData *list);
ObjData *get_obj_in_list_num(int num, ObjData *list);
ObjData *get_obj_in_list_vnum(int num, ObjData *list);

ObjData *get_obj(char *name, int vnum = 0);
ObjData *get_obj_num(ObjRnum nr);

int obj_decay(ObjData *object);
bool obj_to_room(ObjData *object, RoomRnum room);
void obj_from_room(ObjData *object);
void obj_to_obj(ObjData *obj, ObjData *obj_to);
void obj_from_obj(ObjData *obj);
void object_list_new_owner(ObjData *list, CharData *ch);

void extract_obj(ObjData *obj);

// ******* characters ********* //

CharData *get_char_room(char *name, RoomRnum room);
CharData *get_char_num(MobRnum nr);
CharData *get_char(char *name, int vnum = 0);

void char_from_room(CharData *ch);
inline void char_from_room(const CharData::shared_ptr &ch) { char_from_room(ch.get()); }
void char_to_room(CharData *ch, RoomRnum room);
void char_flee_to_room(CharData *ch, RoomRnum room);
inline void char_to_room(const CharData::shared_ptr &ch, RoomRnum room) { char_to_room(ch.get(), room); }
inline void char_flee_to_room(const CharData::shared_ptr &ch, RoomRnum room) { char_flee_to_room(ch.get(), room); }
void extract_char(CharData *ch, int clear_objs, bool zone_reset = 0);
void room_affect_process_on_entry(CharData *ch, RoomRnum room);

// find if character can see //
CharData *get_char_room_vis(CharData *ch, const char *name);
inline CharData *get_char_room_vis(CharData *ch, const std::string &name) {
	return get_char_room_vis(ch,
							 name.c_str());
}

CharData *get_player_vis(CharData *ch, const char *name, int inroom);
inline CharData *get_player_vis(CharData *ch, const std::string &name, int inroom) {
	return get_player_vis(ch,
						  name.c_str(),
						  inroom);
}

CharData *get_player_pun(CharData *ch, const char *name, int inroom);
inline CharData *get_player_pun(CharData *ch, const std::string &name, int inroom) {
	return get_player_pun(ch,
						  name.c_str(),
						  inroom);
}

CharData *get_char_vis(CharData *ch, const char *name, int where);
inline CharData *get_char_vis(CharData *ch, const std::string &name, int where) {
	return get_char_vis(ch,
						name.c_str(),
						where);
}

ObjData *get_obj_in_list_vis(CharData *ch, const char *name, ObjData *list, bool locate_item = false);
inline ObjData *get_obj_in_list_vis(CharData *ch,
									const std::string &name,
									ObjData *list) { return get_obj_in_list_vis(ch, name.c_str(), list); }

ObjData *get_obj_vis(CharData *ch, const char *name);
inline ObjData *get_obj_vis(CharData *ch, const std::string &name) { return get_obj_vis(ch, name.c_str()); }

ObjData *get_obj_vis_for_locate(CharData *ch, const char *name);
inline ObjData *get_obj_vis_for_locate(CharData *ch, const std::string &name) {
	return get_obj_vis_for_locate(ch,
								  name.c_str());
}

bool try_locate_obj(CharData *ch, ObjData *i);

ObjData *get_object_in_equip_vis(CharData *ch, const char *arg, ObjData *equipment[], int *j);
inline ObjData *get_object_in_equip_vis(CharData *ch,
										const std::string &arg,
										ObjData *equipment[],
										int *j) { return get_object_in_equip_vis(ch, arg.c_str(), equipment, j); }
// find all dots //

int find_all_dots(char *arg);

const int kFindIndiv = 0;
const int kFindAll = 1;
const int kFindAlldot = 2;


// Generic Find //

int generic_find(char *arg, Bitvector bitvector, CharData *ch, CharData **tar_ch, ObjData **tar_obj);
enum EFind : Bitvector {
	kCharInRoom = 1 << 0,
	kCharInWorld = 1 << 1,
	kCharDisconnected = 1 << 6,
	kObjInventory = 1 << 2,
	kObjRoom = 1 << 3,
	kObjWorld = 1 << 4,
	kObjEquip = 1 << 5,
	kObjExtraDesc = 1 << 7
};

// prototypes from crash save system //
int Crash_delete_crashfile(CharData *ch);
void Crash_listrent(CharData *ch, char *name);
int Crash_load(CharData *ch);
void Crash_crashsave(CharData *ch);
void Crash_idlesave(CharData *ch);

bool stop_follower(CharData *ch, int mode);

// townportal //
char *find_portal_by_vnum(int vnum);
int level_portal_by_vnum(int vnum);
int find_portal_by_word(char *wrd);
void add_portal_to_char(CharData *ch, int vnum);
int has_char_portal(CharData *ch, int vnum);
void check_portals(CharData *ch);
struct Portal *get_portal(int vnum, char *wrd);

// charm //

float get_effective_cha(CharData *ch);
float get_effective_wis(CharData *ch, int spellnum);
float get_effective_int(CharData *ch);
int get_player_charms(CharData *ch, int spellnum);

// mem queue //
int mag_manacost(const CharData *ch, int spellnum);
void MemQ_init(CharData *ch);
void MemQ_flush(CharData *ch);
int MemQ_learn(CharData *ch);
inline int MemQ_learn(const CharData::shared_ptr &ch) { return MemQ_learn(ch.get()); }
void MemQ_remember(CharData *ch, int num);
void MemQ_forget(CharData *ch, int num);
int *MemQ_slots(CharData *ch);

int get_object_low_rent(ObjData *obj);
void set_uid(ObjData *object);

void remove_rune_label(CharData *ch);

#endif // HANDLER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
