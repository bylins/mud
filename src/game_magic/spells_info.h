#ifndef SPELLS_INFO_H_
#define SPELLS_INFO_H_

#include "game_classes/classes_constants.h"
#include "entities/entities_constants.h"
#include "structs/structs.h"

extern const char *unused_spellname;

struct SpellInfo {
	EPosition min_position;    // Position for caster   //
	int mana_min;        // Min amount of mana used by a spell (highest lev) //
	int mana_max;        // Max amount of mana used by a spell (lowest lev) //
	int mana_change;    // Change in mana used by spell from lev to lev //
	int min_remort[kNumPlayerClasses][kNumKins];
	int min_level[kNumPlayerClasses][kNumKins];
	int slot_forc[kNumPlayerClasses][kNumKins];
	long danger;
	Bitvector routines;
	int violent;
	int targets;
	int spell_class;
	const char *name;
	const char *syn;
};

struct SpellCreateItem {
	std::array<int, 3> items;
	int rnumber;
	int min_caster_level;
};

struct SpellCreate {
	struct SpellCreateItem wand;
	struct SpellCreateItem scroll;
	struct SpellCreateItem potion;
	struct SpellCreateItem items;
	struct SpellCreateItem runes;
};

extern std::unordered_map<ESpell, SpellInfo> spell_info;
extern std::unordered_map<ESpell, SpellCreate> spell_create;

void InitSpells();

#endif //SPELLS_INFO_H_
