/**
 \file magic_item.h - a part of the Bylins engine.
 \brief issue.spellhandlers: magic-item recipe / rune-creation mechanic (used by do_create,
        do_mixture, dg scripts and the rune-stats admin/heartbeat reporting).
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_MAGIC_ITEM_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_MAGIC_ITEM_H_

#include "gameplay/magic/spells_constants.h"   // ESpell, ESpellType, ITEM_BY_NAME/NAME_BY_ITEM

#include <string>

class CharData;
class ObjData;

int CheckRecipeValues(CharData *ch, ESpell spell_id, ESpellType spell_type, int showrecipe);
int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, CharData *tch = nullptr);
void print_rune_stats(CharData *ch);
void print_rune_log();

/// Flags for ingredient items (kMagicIngredient)
enum EIngredientFlag {
	kItemRunes = 1 << 0,
	kItemCheckUses = 1 << 1,
	kItemCheckLag = 1 << 2,
	kItemCheckLevel = 1 << 3,
	kItemDecayEmpty = 1 << 4
};

template<>
EIngredientFlag ITEM_BY_NAME<EIngredientFlag>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_MAGIC_ITEM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
