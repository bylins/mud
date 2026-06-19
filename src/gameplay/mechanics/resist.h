/**
 \file resist.h - a part of the Bylins engine.
 \brief issue.spell-pipeline-cleaning: damage-resistance system -- resist types + apply logic
        (moved out of entities_constants.h / magic_utils).
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_RESIST_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_RESIST_H_

#include "gameplay/magic/spells_constants.h"   // EElement, ESpell, NAME_BY_ITEM/ITEM_BY_NAME/NAMES_OF

#include <map>
#include <string>

class CharData;

/**
 * Magic damage resistance types.
 */
enum EResist {
	kFire = 0,
	kAir,
	kWater,
	kEarth,
	kVitality,
	kMind,
	kImmunity,
	kDark,
	kFirstResist = kFire,
	kLastResist = kDark
};

EResist &operator++(EResist &r);

template<>
const std::string &NAME_BY_ITEM<EResist>(EResist item);
template<>
EResist ITEM_BY_NAME<EResist>(const std::string &name);
template<>
const std::map<EResist, std::string> &NAMES_OF<EResist>();  // issue.vedun-editor

const int kMaxPcResist = 75;

// Map a damage element to its resistance type.
EResist GetResisTypeWithElement(EElement element);
// The resistance type for a spell's element.
EResist GetResistType(ESpell spell_id);
// Apply `ch`'s resistance of `resist_type` to `value` (reduces it; amplifies if resistance < 0).
int ApplyResist(CharData *ch, EResist resist_type, int value);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_RESIST_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
