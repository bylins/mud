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

#ifndef _HANDLER_H_
#define _HANDLER_H_

#include "entities/char.h"
#include "structs/structs.h"    // there was defined type "byte" if it had been missing
#include "structs/flags.hpp"

struct RoomData;

#define LIGHT_NO    0
#define LIGHT_YES   1
#define LIGHT_UNDEF 2

enum class CharEquipFlag : uint8_t {
	// no spell casting
	no_cast,

	// no total affect update
	skip_total,

	// show wear and activation messages
	show_msg
};
FLAGS_DECLARE_FROM_ENUM(CharEquipFlags, CharEquipFlag);
FLAGS_DECLARE_OPERATORS(CharEquipFlags, CharEquipFlag);

int get_room_sky(int rnum);
int equip_in_metall(CharacterData *ch);
int awake_others(CharacterData *ch);

void check_light(CharacterData *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef);

// Resistance calculate //
int calculate_resistance_coeff(CharacterData *ch, int resist_type, int effect);
int getResisTypeWithSpellClass(int spellClass);
int get_resist_type(int spellnum);

// handling the affected-structures //
void ImposeTimedFeat(CharacterData *ch, TimedFeat *timed);
void ExpireTimedFeat(CharacterData *ch, TimedFeat *timed);
int IsTimed(CharacterData *ch, int feat);
void timed_to_char(CharacterData *ch, struct TimedSkill *timed);
void timed_from_char(CharacterData *ch, struct TimedSkill *timed);
int IsTimedBySkill(CharacterData *ch, ESkill id);
void decreaseFeatTimer(CharacterData *ch, int featureID);

// utility //
char *money_desc(int amount, int padis);
ObjectData::shared_ptr create_money(int amount);
char *fname(const char *namelist);
int get_number(char **name);
int get_number(std::string &name);

RoomVnum get_room_where_obj(ObjectData *obj, bool deep = false);

// ******** objects *********** //
bool equal_obj(ObjectData *obj_one, ObjectData *obj_two);
void obj_to_char(ObjectData *object, CharacterData *ch);
void obj_from_char(ObjectData *object);

void equip_char(CharacterData *ch, ObjectData *obj, int pos, const CharEquipFlags& equip_flags);
ObjectData *unequip_char(CharacterData *ch, int pos, const CharEquipFlags& equip_flags);
int invalid_align(CharacterData *ch, ObjectData *obj);

ObjectData *get_obj_in_list(char *name, ObjectData *list);
ObjectData *get_obj_in_list_num(int num, ObjectData *list);
ObjectData *get_obj_in_list_vnum(int num, ObjectData *list);

ObjectData *get_obj(char *name, int vnum = 0);
ObjectData *get_obj_num(ObjRnum nr);

int obj_decay(ObjectData *object);
bool obj_to_room(ObjectData *object, RoomRnum room);
void obj_from_room(ObjectData *object);
void obj_to_obj(ObjectData *obj, ObjectData *obj_to);
void obj_from_obj(ObjectData *obj);
void object_list_new_owner(ObjectData *list, CharacterData *ch);

void extract_obj(ObjectData *obj);

// ******* characters ********* //

CharacterData *get_char_room(char *name, RoomRnum room);
CharacterData *get_char_num(MobRnum nr);
CharacterData *get_char(char *name, int vnum = 0);

void char_from_room(CharacterData *ch);
inline void char_from_room(const CharacterData::shared_ptr &ch) { char_from_room(ch.get()); }
void char_to_room(CharacterData *ch, RoomRnum room);
void char_flee_to_room(CharacterData *ch, RoomRnum room);
inline void char_to_room(const CharacterData::shared_ptr &ch, RoomRnum room) { char_to_room(ch.get(), room); }
inline void char_flee_to_room(const CharacterData::shared_ptr &ch, RoomRnum room) { char_flee_to_room(ch.get(), room); }
void extract_char(CharacterData *ch, int clear_objs, bool zone_reset = 0);
void room_affect_process_on_entry(CharacterData *ch, RoomRnum room);

// find if character can see //
CharacterData *get_char_room_vis(CharacterData *ch, const char *name);
inline CharacterData *get_char_room_vis(CharacterData *ch, const std::string &name) {
	return get_char_room_vis(ch,
							 name.c_str());
}

CharacterData *get_player_vis(CharacterData *ch, const char *name, int inroom);
inline CharacterData *get_player_vis(CharacterData *ch, const std::string &name, int inroom) {
	return get_player_vis(ch,
						  name.c_str(),
						  inroom);
}

CharacterData *get_player_pun(CharacterData *ch, const char *name, int inroom);
inline CharacterData *get_player_pun(CharacterData *ch, const std::string &name, int inroom) {
	return get_player_pun(ch,
						  name.c_str(),
						  inroom);
}

CharacterData *get_char_vis(CharacterData *ch, const char *name, int where);
inline CharacterData *get_char_vis(CharacterData *ch, const std::string &name, int where) {
	return get_char_vis(ch,
						name.c_str(),
						where);
}

ObjectData *get_obj_in_list_vis(CharacterData *ch, const char *name, ObjectData *list, bool locate_item = false);
inline ObjectData *get_obj_in_list_vis(CharacterData *ch,
									   const std::string &name,
									   ObjectData *list) { return get_obj_in_list_vis(ch, name.c_str(), list); }

ObjectData *get_obj_vis(CharacterData *ch, const char *name);
inline ObjectData *get_obj_vis(CharacterData *ch, const std::string &name) { return get_obj_vis(ch, name.c_str()); }

ObjectData *get_obj_vis_for_locate(CharacterData *ch, const char *name);
inline ObjectData *get_obj_vis_for_locate(CharacterData *ch, const std::string &name) {
	return get_obj_vis_for_locate(ch,
								  name.c_str());
}

bool try_locate_obj(CharacterData *ch, ObjectData *i);

ObjectData *get_object_in_equip_vis(CharacterData *ch, const char *arg, ObjectData *equipment[], int *j);
inline ObjectData *get_object_in_equip_vis(CharacterData *ch,
										   const std::string &arg,
										   ObjectData *equipment[],
										   int *j) { return get_object_in_equip_vis(ch, arg.c_str(), equipment, j); }
// find all dots //

int find_all_dots(char *arg);

#define FIND_INDIV    0
#define FIND_ALL    1
#define FIND_ALLDOT    2


// Generic Find //

int generic_find(char *arg, Bitvector bitvector, CharacterData *ch, CharacterData **tar_ch, ObjectData **tar_obj);

#define FIND_CHAR_ROOM     (1 << 0)
#define FIND_CHAR_WORLD    (1 << 1)
#define FIND_OBJ_INV       (1 << 2)
#define FIND_OBJ_ROOM      (1 << 3)
#define FIND_OBJ_WORLD     (1 << 4)
#define FIND_OBJ_EQUIP     (1 << 5)
#define FIND_CHAR_DISCONNECTED (1 << 6)
#define FIND_OBJ_EXDESC    (1 << 7)

#define CRASH_DELETE_OLD   (1 << 0)
#define CRASH_DELETE_NEW   (1 << 1)

// prototypes from crash save system //
int Crash_delete_crashfile(CharacterData *ch);
void Crash_listrent(CharacterData *ch, char *name);
int Crash_load(CharacterData *ch);
void Crash_crashsave(CharacterData *ch);
void Crash_idlesave(CharacterData *ch);

bool stop_follower(CharacterData *ch, int mode);

// townportal //
char *find_portal_by_vnum(int vnum);
int level_portal_by_vnum(int vnum);
int find_portal_by_word(char *wrd);
void add_portal_to_char(CharacterData *ch, int vnum);
int has_char_portal(CharacterData *ch, int vnum);
void check_portals(CharacterData *ch);
struct Portal *get_portal(int vnum, char *wrd);

// charm //

float get_effective_cha(CharacterData *ch);
float get_effective_wis(CharacterData *ch, int spellnum);
float get_effective_int(CharacterData *ch);
int get_player_charms(CharacterData *ch, int spellnum);

// mem queue //
int mag_manacost(const CharacterData *ch, int spellnum);
void MemQ_init(CharacterData *ch);
void MemQ_flush(CharacterData *ch);
int MemQ_learn(CharacterData *ch);
inline int MemQ_learn(const CharacterData::shared_ptr &ch) { return MemQ_learn(ch.get()); }
void MemQ_remember(CharacterData *ch, int num);
void MemQ_forget(CharacterData *ch, int num);
int *MemQ_slots(CharacterData *ch);

int get_object_low_rent(ObjectData *obj);
void set_uid(ObjectData *object);

void remove_rune_label(CharacterData *ch);
#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
