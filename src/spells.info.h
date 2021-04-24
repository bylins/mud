#ifndef __SPELLS_INFO__
#define __SPELLS_INFO__

#include "classes/constants.hpp"
#include "structs.h"

extern const char *unused_spellname;

struct spellInfo_t {
	byte min_position;	// Position for caster   //
	int mana_min;		// Min amount of mana used by a spell (highest lev) //
	int mana_max;		// Max amount of mana used by a spell (lowest lev) //
	int mana_change;	// Change in mana used by spell from lev to lev //
	int min_remort[NUM_PLAYER_CLASSES][NUM_KIN];
	int min_level[NUM_PLAYER_CLASSES][NUM_KIN];
	int slot_forc[NUM_PLAYER_CLASSES][NUM_KIN];
	int class_change[NUM_PLAYER_CLASSES][NUM_KIN];
	long danger;
	long routines;
	byte violent;
	int targets;		// See below for use with TAR_XXX  //
	byte spell_class;
	const char *name;
	const char *syn;
};

struct spell_create_item
{
	std::array<int, 3> items;
	int rnumber;
	int min_caster_level;
};

struct spell_create_type
{
	struct spell_create_item wand;
	struct spell_create_item scroll;
	struct spell_create_item potion;
	struct spell_create_item items;
	struct spell_create_item runes;
};

extern struct spellInfo_t spell_info[];
extern struct spell_create_type spell_create[];

void initSpells(void);
const char *spell_name(int num);

#endif //__SPELLS_INFO__
