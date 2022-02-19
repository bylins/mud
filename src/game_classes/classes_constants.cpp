/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы классов персонажей.
*/

#include <map>

#include "classes_constants.h"

std::array<const char *, kNumPlayerClasses> pc_class_name =
	{{
		 "лекарь",
		 "колдун",
		 "тать",
		 "богатырь",
		 "наемник",
		 "дружинник",
		 "кудесник",
		 "волшебник",
		 "чернокнижник",
		 "витязь",
		 "охотник",
		 "кузнец",
		 "купец",
		 "волхв"
	 }};

const ClassApplies::ExtraAffectsVector ClericAffects = {};
const ClassApplies::ExtraAffectsVector MageAffects = {{EAffectFlag::AFF_INFRAVISION, true}};
const ClassApplies::ExtraAffectsVector ThiefAffects = {
	{EAffectFlag::AFF_INFRAVISION, true},
	{EAffectFlag::AFF_SENSE_LIFE, true},
	{EAffectFlag::AFF_BLINK, true}};
const ClassApplies::ExtraAffectsVector WarriorAffects = {};
const ClassApplies::ExtraAffectsVector AssasineAffects = {{EAffectFlag::AFF_INFRAVISION, true}};
const ClassApplies::ExtraAffectsVector GuardAffects = {};
const ClassApplies::ExtraAffectsVector DefenderAffects = {};
const ClassApplies::ExtraAffectsVector CharmerAffects = {};
const ClassApplies::ExtraAffectsVector NecromancerAffects = {{EAffectFlag::AFF_INFRAVISION, true}};
const ClassApplies::ExtraAffectsVector PaladineAffects = {};
const ClassApplies::ExtraAffectsVector RangerAffects = {
	{EAffectFlag::AFF_INFRAVISION, true},
	{EAffectFlag::AFF_SENSE_LIFE, true}};
const ClassApplies::ExtraAffectsVector SmithAffects = {};
const ClassApplies::ExtraAffectsVector MerchantAffects = {};
const ClassApplies::ExtraAffectsVector DruidAffects = {};

struct ClassApplies class_app[kNumPlayerClasses] =
	{
// unknown_weapon_fault koef_con base_con min_con max_con extra_affects
		{5, 40, 10, 12, 50, &ClericAffects},
		{3, 35, 10, 10, 50, &MageAffects},
		{3, 55, 10, 14, 50, &ThiefAffects},
		{2, 105, 10, 22, 50, &WarriorAffects},
		{3, 50, 10, 14, 50, &AssasineAffects},
		{2, 105, 10, 17, 50, &GuardAffects},
		{5, 35, 10, 10, 50, &DefenderAffects},
		{5, 35, 10, 10, 50, &CharmerAffects},
		{5, 35, 10, 11, 50, &NecromancerAffects},
		{2, 100, 10, 14, 50, &PaladineAffects},
		{2, 100, 10, 14, 50, &RangerAffects},
		{2, 100, 10, 14, 50, &SmithAffects},
		{3, 50, 10, 14, 50, &MerchantAffects},
		{5, 40, 10, 12, 50, &DruidAffects}
	};

ECharClass& operator++(ECharClass &c) {
	c =  static_cast<ECharClass>(to_underlying(c) + 1);
	return c;
};

typedef std::map<ECharClass, std::string> ECharClass_name_by_value_t;
typedef std::map<const std::string, ECharClass> ECharClass_value_by_name_t;
ECharClass_name_by_value_t ECharClass_name_by_value;
ECharClass_value_by_name_t ECharClass_value_by_name;

void init_ECharClass_ITEM_NAMES() {
	ECharClass_name_by_value.clear();
	ECharClass_value_by_name.clear();

	ECharClass_name_by_value[ECharClass::kUndefined] = "kUndefined";
	ECharClass_name_by_value[ECharClass::kAssasine] = "kAssasine";
	ECharClass_name_by_value[ECharClass::kCharmer] = "kCharmer";
	ECharClass_name_by_value[ECharClass::kConjurer] = "kConjurer";
	ECharClass_name_by_value[ECharClass::kGuard] = "kGuard";
	ECharClass_name_by_value[ECharClass::kMagus] = "kMagus";
	ECharClass_name_by_value[ECharClass::kMerchant] = "kMerchant";
	ECharClass_name_by_value[ECharClass::kNecromancer] = "kNecromancer";
	ECharClass_name_by_value[ECharClass::kPaladine] = "kPaladine";
	ECharClass_name_by_value[ECharClass::kRanger] = "kRanger";
	ECharClass_name_by_value[ECharClass::kSorcerer] = "kSorcerer";
	ECharClass_name_by_value[ECharClass::kThief] = "kThief";
	ECharClass_name_by_value[ECharClass::kVigilant] = "kVigilant";
	ECharClass_name_by_value[ECharClass::kWarrior] = "kWarrior";
	ECharClass_name_by_value[ECharClass::kWizard] = "kWizard";
	ECharClass_name_by_value[ECharClass::kMob] = "kMob";
	ECharClass_name_by_value[ECharClass::kNpcBase] = "kNpcBase";
	ECharClass_name_by_value[ECharClass::kNpcLast] = "kNpcLast";

	for (const auto &i : ECharClass_name_by_value) {
		ECharClass_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ECharClass>(const ECharClass item) {
	if (ECharClass_name_by_value.empty()) {
		init_ECharClass_ITEM_NAMES();
	}
	return ECharClass_name_by_value.at(item);
}

template<>
ECharClass ITEM_BY_NAME<ECharClass>(const std::string &name) {
	if (ECharClass_name_by_value.empty()) {
		init_ECharClass_ITEM_NAMES();
	}
	return ECharClass_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
