/**************************************************************************
*  File: constants.h                                     Part of Bylins   *
*  Usage: header file for mud contstants.                                 *
*                                                                         *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include "game_classes/classes_constants.h"
#include "structs/structs.h"
#include "sysdep.h"
#include "conf.h"

#include <vector>
#include <array>

struct IntApplies {
	int spell_aknowlege;    // drop_chance to know spell               //
	int to_skilluse;        // ADD CHANSE FOR USING SKILL         //
	int mana_per_tic;
	int spell_success;        //  max count of spell on 1s level    //
	int improve;        // drop_chance to improve skill           //
	int observation;        // drop_chance to use kAwake/CRITICAL //
};

struct ChaApplies {
	int leadership;
	int charms;
	int morale;
	int illusive;
	int dam_to_hit_rate;
};

struct SizeApplies {
	int ac;
	int interpolate;        // ADD VALUE FOR SOME SKILLS  //
	int initiative;
	int shocking;
};

struct WeaponApplies {
	int shocking;
	int bashing;
	int parrying;
};

struct pray_affect_type {
	int metter;
	EApply location;
	int modifier;
	uint32_t bitvector;
	int battleflag;
};

extern const char *circlemud_version;
extern const char *dirs[];
extern const char *DirsFrom[];
extern const char *DirsTo[];
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *resistance_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *connected_types[];
extern const char *where[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_negative[];
extern const char *weapon_affects[];
extern const char *anti_bits[];
extern const char *no_bits[];
extern const char *material_type[];
extern const char *container_bits[];
extern const char *fullness[];

extern const std::vector<const char *> npc_role_types;
extern const char *npc_race_types[];
extern const char *places_of_birth[];
extern const char *weekdays[];
extern const char *month_name[];
extern const char *weekdays_poly[];
extern const char *month_name_poly[];
extern const char *ingradient_bits[];
extern const char *magic_container_bits[];
extern const char *function_bits[];
extern const char *pray_metter[];
extern const char *pray_whom[];
extern const char *room_aff_visib_bits[];
extern const char *room_aff_invis_bits[];
extern const char *room_self_aff_invis_bits[];
extern const char *equipment_types[];
extern struct IntApplies int_app[];
extern const size_t INT_APP_SIZE;
extern struct ChaApplies cha_app[];
extern struct SizeApplies size_app[];
extern struct WeaponApplies weapon_app[];
extern std::vector<pray_affect_type> pray_affect;
extern int rev_dir[];
extern int movement_loss[];

extern int mana[];
extern int mana_gain_cs[];
extern int mana_cost_cs[][9];
extern const char *material_name[];
extern struct AttackHitType attack_hit_text[];
extern const char *godslike_bits[];
extern const char *weapon_class[];

//The number of changing coefficients (the others are unchanged)
const int MAX_EXP_COEFFICIENTS_USED = 15;
// unless you change this, Puff casts all your dg spells
const int DG_CASTER_PROXY = 113;

const int FIRST_ROOM = 1;
const int STRANGE_ROOM = 3;
const int FIRE_MOVES = 20;
const int LOOKING_MOVES = 5;
const int HEARING_MOVES = 2;
const int LOOKHIDE_MOVES = 3;
const int SNEAK_MOVES = 1;
const int CAMOUFLAGE_MOVES = 1;
const int PICKLOCK_MOVES = 10;
const int TRACK_MOVES = 3;
const int SENSE_MOVES = 4;
const int HIDETRACK_MOVES = 10;
const int MOB_ARMOUR_MULT = 5;
const int MOB_AC_MULT = 5;
const int MOB_DAMAGE_MULT = 3;
const int MAX_GROUPED_FOLLOWERS = 7;

extern int HORSE_VNUM;
extern int HORSE_COST;
extern int START_BREAD;
extern int CREATE_LIGHT;

#endif // CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
