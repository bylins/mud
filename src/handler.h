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

#include "char.hpp"
#include "structs.h"	// there was defined type "byte" if it had been missing

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

#define LIGHT_NO    0
#define LIGHT_YES   1
#define LIGHT_UNDEF 2

int get_room_sky(int rnum);
int equip_in_metall(CHAR_DATA * ch);
int awake_others(CHAR_DATA * ch);

void check_light(CHAR_DATA * ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef);

// Resistance calculate //
int calculate_resistance_coeff(CHAR_DATA *ch, int resist_type, int effect);

// handling the affected-structures //
void affect_total(CHAR_DATA * ch);
void affect_modify(CHAR_DATA * ch, byte loc, int mod, const EAffectFlag bitv, bool add);
void affect_to_char(CHAR_DATA* ch, const AFFECT_DATA<EApplyLocation>& af);
void affect_from_char(CHAR_DATA * ch, int type);
bool affected_by_spell(CHAR_DATA * ch, int type);
void affect_join_fspell(CHAR_DATA* ch, const AFFECT_DATA<EApplyLocation>& af);
void affect_join(CHAR_DATA * ch, AFFECT_DATA<EApplyLocation>& af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void timed_feat_to_char(CHAR_DATA * ch, struct timed_type *timed);
void timed_feat_from_char(CHAR_DATA * ch, struct timed_type *timed);
int timed_by_feat(CHAR_DATA * ch, int skill);
void timed_to_char(CHAR_DATA * ch, struct timed_type *timed);
void timed_from_char(CHAR_DATA * ch, struct timed_type *timed);
int timed_by_skill(CHAR_DATA * ch, int skill);

// Обработка аффектов комнат//
void affect_room_total(ROOM_DATA * room);
void affect_room_modify(ROOM_DATA * room, byte loc, sbyte mod, bitvector_t bitv, bool add);
void affect_to_room(ROOM_DATA * room, const AFFECT_DATA<ERoomApplyLocation>& af);
void affect_room_remove(ROOM_DATA* room, const ROOM_DATA::room_affects_list_t::iterator& af);
ROOM_DATA::room_affects_list_t::iterator find_room_affect(ROOM_DATA* room, int type);
void affect_room_join_fspell(ROOM_DATA* room, const AFFECT_DATA<ERoomApplyLocation>& af);
void affect_room_join(ROOM_DATA * room, AFFECT_DATA<ERoomApplyLocation>& af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);

// utility //
char *money_desc(int amount, int padis);
OBJ_DATA::shared_ptr create_money(int amount);
char *fname(const char *namelist);
int get_number(char **name);
int get_number(std::string &name);

room_vnum get_room_where_obj(OBJ_DATA *obj, bool deep = false);

// ******** objects *********** //
bool equal_obj(OBJ_DATA *obj_one, OBJ_DATA *obj_two);
void obj_to_char(OBJ_DATA * object, CHAR_DATA * ch);
void obj_from_char(OBJ_DATA * object);

void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int pos);
OBJ_DATA *unequip_char(CHAR_DATA * ch, int pos);
int invalid_align(CHAR_DATA * ch, OBJ_DATA * obj);

OBJ_DATA *get_obj_in_list(char *name, OBJ_DATA * list);
OBJ_DATA *get_obj_in_list_num(int num, OBJ_DATA * list);
OBJ_DATA *get_obj_in_list_vnum(int num, OBJ_DATA * list);

OBJ_DATA *get_obj(char *name, int vnum = 0);
OBJ_DATA *get_obj_num(obj_rnum nr);

int obj_decay(OBJ_DATA * object);
bool obj_to_room(OBJ_DATA * object, room_rnum room);
void obj_from_room(OBJ_DATA * object);
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to);
void obj_from_obj(OBJ_DATA * obj);
void object_list_new_owner(OBJ_DATA * list, CHAR_DATA * ch);

void extract_obj(OBJ_DATA * obj);

// ******* characters ********* //

CHAR_DATA *get_char_room(char *name, room_rnum room);
CHAR_DATA *get_char_num(mob_rnum nr);
CHAR_DATA *get_char(char *name, int vnum = 0);

void char_from_room(CHAR_DATA * ch);
inline void char_from_room(const CHAR_DATA::shared_ptr& ch) { char_from_room(ch.get()); }
void char_to_room(CHAR_DATA * ch, room_rnum room);
inline void char_to_room(const CHAR_DATA::shared_ptr& ch, room_rnum room) { char_to_room(ch.get(), room); }
void extract_char(CHAR_DATA * ch, int clear_objs, bool zone_reset = 0);
void room_affect_process_on_entry(CHAR_DATA * ch, room_rnum room);

// find if character can see //
CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, const char *name);
inline CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, const std::string &name) { return get_char_room_vis(ch, name.c_str()); }

CHAR_DATA *get_player_vis(CHAR_DATA * ch, const char *name, int inroom);
inline CHAR_DATA *get_player_vis(CHAR_DATA * ch, const std::string &name, int inroom) { return get_player_vis(ch, name.c_str(), inroom); }

CHAR_DATA *get_player_pun(CHAR_DATA * ch, const char *name, int inroom);
inline CHAR_DATA *get_player_pun(CHAR_DATA * ch, const std::string &name, int inroom) { return get_player_pun(ch, name.c_str(), inroom); }

CHAR_DATA *get_char_vis(CHAR_DATA * ch, const char *name, int where);
inline CHAR_DATA *get_char_vis(CHAR_DATA * ch, const std::string &name, int where) { return get_char_vis(ch, name.c_str(), where); }

OBJ_DATA* get_obj_in_list_vis(CHAR_DATA * ch, const char *name, OBJ_DATA * list, bool locate_item = false);
inline OBJ_DATA* get_obj_in_list_vis(CHAR_DATA * ch, const std::string &name, OBJ_DATA * list) { return get_obj_in_list_vis(ch, name.c_str(), list); }

OBJ_DATA* get_obj_vis(CHAR_DATA * ch, const char *name);
inline OBJ_DATA *get_obj_vis(CHAR_DATA * ch, const std::string &name) { return get_obj_vis(ch, name.c_str()); }

OBJ_DATA* get_obj_vis_for_locate(CHAR_DATA * ch, const char *name);
inline OBJ_DATA* get_obj_vis_for_locate(CHAR_DATA * ch, const std::string &name) { return get_obj_vis_for_locate(ch, name.c_str()); }

bool try_locate_obj(CHAR_DATA * ch, OBJ_DATA *i);

OBJ_DATA *get_object_in_equip_vis(CHAR_DATA * ch, const char *arg, OBJ_DATA * equipment[], int *j);
inline OBJ_DATA *get_object_in_equip_vis(CHAR_DATA * ch, const std::string &arg, OBJ_DATA * equipment[], int *j) { return get_object_in_equip_vis(ch, arg.c_str(), equipment, j); }

// find all dots //

int find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


// Generic Find //

int generic_find(char *arg, bitvector_t bitvector, CHAR_DATA * ch, CHAR_DATA ** tar_ch, OBJ_DATA ** tar_obj);

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
int Crash_delete_crashfile(CHAR_DATA * ch);
void Crash_listrent(CHAR_DATA * ch, char *name);
int Crash_load(CHAR_DATA * ch);
void Crash_crashsave(CHAR_DATA * ch);
void Crash_idlesave(CHAR_DATA * ch);

bool stop_follower(CHAR_DATA * ch, int mode);
void forget(CHAR_DATA * ch, CHAR_DATA * victim);
void remember(CHAR_DATA * ch, CHAR_DATA * victim);

// townportal //
char *find_portal_by_vnum(int vnum);
int level_portal_by_vnum(int vnum);
int find_portal_by_word(char *wrd);
void add_portal_to_char(CHAR_DATA * ch, int vnum);
int has_char_portal(CHAR_DATA * ch, int vnum);
void check_portals(CHAR_DATA * ch);
struct portals_list_type *get_portal(int vnum, char *wrd);

// charm //

#define MAXPRICE 9999999

float get_damage_per_round(CHAR_DATA * victim);
float get_effective_cha(CHAR_DATA * ch);
float get_effective_wis(CHAR_DATA * ch, int spellnum);
float get_effective_int(CHAR_DATA * ch);
int get_reformed_charmice_hp(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum);
int get_player_charms(CHAR_DATA * ch, int spellnum);
float calc_cha_for_hire(CHAR_DATA * victim);
int calc_hire_price(CHAR_DATA * ch, CHAR_DATA * victim);


// mem queue //
void MemQ_init(CHAR_DATA * ch);
void MemQ_flush(CHAR_DATA * ch);
int MemQ_learn(CHAR_DATA * ch);
inline int MemQ_learn(const CHAR_DATA::shared_ptr& ch) { return MemQ_learn(ch.get()); }
void MemQ_remember(CHAR_DATA * ch, int num);
void MemQ_forget(CHAR_DATA * ch, int num);
int *MemQ_slots(CHAR_DATA * ch);

int get_object_low_rent(OBJ_DATA *obj);
void set_uid(OBJ_DATA *object);

void remove_rune_label(CHAR_DATA *ch);
#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
