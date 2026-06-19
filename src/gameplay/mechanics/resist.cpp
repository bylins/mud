/**
 \file resist.cpp - a part of the Bylins engine.
 \brief issue.spell-pipeline-cleaning: damage-resistance system (extracted from entities_constants
        + magic_utils).
*/

#include "gameplay/mechanics/resist.h"

#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "utils/utils.h"          // GET_RESIST
#include "utils/random.h"         // number

typedef std::map<EResist, std::string> EResist_name_by_value_t;
typedef std::map<const std::string, EResist> EResist_value_by_name_t;
EResist_name_by_value_t EResist_name_by_value;
EResist_value_by_name_t EResist_value_by_name;

void init_EResist_ITEM_NAMES() {
	EResist_name_by_value.clear();
	EResist_value_by_name.clear();

	EResist_name_by_value[EResist::kFire] = "kFire";
	EResist_name_by_value[EResist::kAir] = "kAir";
	EResist_name_by_value[EResist::kWater] = "kWater";
	EResist_name_by_value[EResist::kEarth] = "kEarth";
	EResist_name_by_value[EResist::kVitality] = "kVitality";
	EResist_name_by_value[EResist::kMind] = "kMind";
	EResist_name_by_value[EResist::kImmunity] = "kImmunity";
	EResist_name_by_value[EResist::kDark] = "kDark";

	for (const auto &i : EResist_name_by_value) {
		EResist_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EResist>(const EResist item) {
	if (EResist_name_by_value.empty()) {
		init_EResist_ITEM_NAMES();
	}
	return EResist_name_by_value.at(item);
}

template<>
const std::map<EResist, std::string> &NAMES_OF<EResist>() {
	if (EResist_name_by_value.empty()) {
		init_EResist_ITEM_NAMES();
	}
	return EResist_name_by_value;
}

template<>
EResist ITEM_BY_NAME<EResist>(const std::string &name) {
	if (EResist_name_by_value.empty()) {
		init_EResist_ITEM_NAMES();
	}
	return EResist_value_by_name.at(name);
}

EResist& operator++(EResist &r) {
	r =  static_cast<EResist>(to_underlying(r) + 1);
	return r;
}

EResist GetResisTypeWithElement(EElement element) {
	switch (element) {
		case EElement::kFire: return EResist::kFire;
		case EElement::kDark: return EResist::kDark;
		case EElement::kAir: return EResist::kAir;
		case EElement::kWater: return EResist::kWater;
		case EElement::kEarth: return EResist::kEarth;
		case EElement::kLight: return EResist::kVitality;
		case EElement::kMind: return EResist::kMind;
		case EElement::kLife: return EResist::kImmunity;
		default: return EResist::kVitality;
	}
}

EResist GetResistType(ESpell spell_id) {
	return GetResisTypeWithElement(MUD::Spell(spell_id).GetElement());
}

int ApplyResist(CharData *ch, EResist resist_type, int value) {
	int resistance = GET_RESIST(ch, resist_type);
	if (resistance <= 0) {
		return value - resistance * value / 100;
	}
	if (!ch->IsNpc()) {
		resistance = std::min(kMaxPcResist, resistance);
	}
	auto result = static_cast<int>(value - (resistance + number(0, resistance)) * value / 200.0);
	return std::max(0, result);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
