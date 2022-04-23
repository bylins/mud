/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы классов персонажей.
*/

#include <map>

#include "classes_constants.h"

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

ECharClass& operator++(ECharClass &c) {
	c =  static_cast<ECharClass>(to_underlying(c) + 1);
	return c;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
