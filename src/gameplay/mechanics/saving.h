/**
 \file saving.h - a part of the Bylins engine.
 \brief issue.saving-mechanics: the saving-throw system -- types, data and calculation, gathered
        from entities_constants / core/constants / magic into one place.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_SAVING_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_SAVING_H_

#include "gameplay/affects/affect_contants.h"   // EApply (ApplyNegative)
#include "engine/structs/meta_enum.h"           // NAME_BY_ITEM / ITEM_BY_NAME / NAMES_OF

#include <map>
#include <string>
#include <vector>

class CharData;

// Character saving-throw ids.
enum class ESaving : int {
	kWill = 0,
	kCritical = 1,
	kStability = 2,
	kReflex = 3,
	kNone = 4,
	kFirst = kWill,
	kLast = kReflex,
};

ESaving &operator++(ESaving &s);

template<>
const std::string &NAME_BY_ITEM<ESaving>(ESaving item);
template<>
ESaving ITEM_BY_NAME<ESaving>(const std::string &name);
template<>
const std::map<ESaving, std::string> &NAMES_OF<ESaving>();

// Maps a defensive apply to the saving type it feeds (and a display name); used to recompute the
// saving bonuses an item/affect grants and to render saving info.
struct ApplyNegative {
	std::string name;
	EApply location;
	ESaving savetype;
};
extern const std::vector<ApplyNegative> apply_negative;
extern const std::map<ESaving, std::string> saving_name;

// Saving-throw calculation. GetBasicSave: class/level base + stat/horse adjustments. CalcSaving:
// adds cautious-style and equipment/buff bonuses. CalcGeneralSaving: rolls the save (true = saved).
int GetBasicSave(CharData *ch, ESaving saving, bool log = false);
int CalcSaving(CharData *killer, CharData *victim, ESaving saving, bool need_log = false);
int CalcGeneralSaving(CharData *killer, CharData *victim, ESaving type, int ext_apply);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_SAVING_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
