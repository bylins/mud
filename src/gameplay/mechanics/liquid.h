/*************************************************************************
*   File: liquid.hpp                                   Part of Bylins    *
*   Все по жидкостям                                                     *
*                                                                        *
*  $Author$                                                      *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#pragma once

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"
#include "gameplay/magic/spells_constants.h"


// виды жидскостей, наливаемых в контейнеры
enum {
	LIQ_WATER = 0,
	LIQ_BEER = 1,
	LIQ_WINE = 2,
	LIQ_ALE = 3,
	LIQ_QUAS = 4,
	LIQ_BRANDY = 5,
	LIQ_MORSE = 6,
	LIQ_VODKA = 7,
	LIQ_BRAGA = 8,
	LIQ_MED = 9,
	LIQ_MILK = 10,
	LIQ_TEA = 11,
	LIQ_COFFE = 12,
	LIQ_BLOOD = 13,
	LIQ_SALTWATER = 14,
	LIQ_CLEARWATER = 15,
	LIQ_POTION = 16,
	LIQ_POTION_RED = 17,
	LIQ_POTION_BLUE = 18,
	LIQ_POTION_WHITE = 19,
	LIQ_POTION_GOLD = 20,
	LIQ_POTION_BLACK = 21,
	LIQ_POTION_GREY = 22,
	LIQ_POTION_FUCHSIA = 23,
	LIQ_POTION_PINK = 24,
	LIQ_POISON_ACONITUM = 25,
	LIQ_POISON_SCOPOLIA = 26,
	LIQ_POISON_BELENA = 27,
	LIQ_POISON_DATURA = 28,
	NUM_LIQ_TYPES = 29
};

extern const char *drinks[];
extern const char *drinknames[];
extern const int drink_aff[][3];
extern const char *color_liquid[];

void name_from_drinkcon(ObjData *obj);
void name_to_drinkcon(ObjData *obj, int type);
bool is_potion(const ObjData *obj);
int TryCastSpellsFromLiquid(CharData *ch, ObjData *jar);

class CObjectPrototype;    // to avoit inclusion of "obj.hpp"

namespace drinkcon {

void identify(CharData *ch, const ObjData *obj);
std::string print_spells(const ObjData *obj);
void copy_potion_values(const CObjectPrototype *from_obj, CObjectPrototype *to_obj);
char *daig_filling_drink(const ObjData *obj, const CharData *ch);
const char *diag_liquid_timer(const ObjData *obj);
void reset_potion_values(CObjectPrototype *obj);
int check_equal_potions(ObjData *from_obj, ObjData *to_obj);
void generate_drinkcon_name(ObjData *to_obj, ESpell spell_id);
int check_equal_drinkcon(ObjData *from_obj, ObjData *to_obj);
void spells_to_drinkcon(ObjData *from_obj, ObjData *to_obj);
// issue.potion-hotfix: quantity-weighted blend of two potions' maker skill/stat when they are mixed.
void mix_potion_values(ObjData *to_obj, const ObjData *from_obj, int n_to, int n_from);

// issue.potion-hotfix: contents-spoilage tuning. A potion in a container carries a freshness
// countdown (kLiquidTimer, see obj_data.h). When it reaches zero the potion SPOILS: its power inputs
// (kPotionSkill and kPotionStat) are halved and it becomes poisonous -- the poison level a fixed
// percentage of the power it had at that moment. Casting "remove poison" on the container clears the
// poison and restarts the timer, so a spoiled potion can be salvaged; but each spoilage cycle halves
// its power again, so after enough cycles it becomes inert.
constexpr int kSpoiledPoisonPercent = 50;      // poison level = this %% of the potion's pre-spoil power
constexpr int kSpoiledResetTimerMinutes = 10;  // remove-poison restarts the freshness timer at this

// Age a food/liquid's CONTENTS by one freshness tick: decrements kLiquidTimer (floored at 0); when it
// crosses to 0 the potion spoils (spoil_potion). No-op for a non-perishable or unset timer. Returns
// the freshness left after the tick (0 means it has just spoiled / is spoiled).
int age_contents(ObjData *obj);
// Spoil a liquid/food's contents now: set the poison level from the current power, then halve the
// power inputs (kPotionSkill/kPotionStat).
void spoil_potion(ObjData *obj);

} // namespace drinkcon

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
