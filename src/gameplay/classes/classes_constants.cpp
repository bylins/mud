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

typedef std::map<EMobClass, std::string> EMobClass_name_by_value_t;
typedef std::map<const std::string, EMobClass> EMobClass_value_by_name_t;
EMobClass_name_by_value_t EMobClass_name_by_value;
EMobClass_value_by_name_t EMobClass_value_by_name;

void init_EMobClass_ITEM_NAMES() {
	EMobClass_name_by_value.clear();
	EMobClass_value_by_name.clear();

	EMobClass_name_by_value[EMobClass::kUndefined] = "kUndefined";
	EMobClass_name_by_value[EMobClass::kBoss] = "kBoss";
	EMobClass_name_by_value[EMobClass::kTrash] = "kTrash";
	EMobClass_name_by_value[EMobClass::kTank] = "kTank";
	EMobClass_name_by_value[EMobClass::kMelleeDmg] = "kMelleeDmg";
	EMobClass_name_by_value[EMobClass::kArcher] = "kArcher";
	EMobClass_name_by_value[EMobClass::kRogue] = "kRogue";
	EMobClass_name_by_value[EMobClass::kMageDmg] = "kMageDmg";
	EMobClass_name_by_value[EMobClass::kMageBuff] = "kMageBuff";
	EMobClass_name_by_value[EMobClass::kHealer] = "kHealer";
	EMobClass_name_by_value[EMobClass::kTotal] = "kTotal";

	for (const auto &i : EMobClass_name_by_value) {
		EMobClass_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EMobClass>(const EMobClass item) {
	if (EMobClass_name_by_value.empty()) {
		init_EMobClass_ITEM_NAMES();
	}
	return EMobClass_name_by_value.at(item);
}

template<>
EMobClass ITEM_BY_NAME<EMobClass>(const std::string &name) {
	if (EMobClass_name_by_value.empty()) {
		init_EMobClass_ITEM_NAMES();
	}
	return EMobClass_value_by_name.at(name);
}

EMobClass& operator++(EMobClass &c) {
	c =  static_cast<EMobClass>(to_underlying(c) + 1);
	return c;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
