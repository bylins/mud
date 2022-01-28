/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы классов персонажей.
*/

#include "class_constants.h"

std::array<const char *, NUM_PLAYER_CLASSES> pc_class_name =
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

const class_app_type::extra_affects_list_t ClericAffects = {};
const class_app_type::extra_affects_list_t MageAffects = {{EAffectFlag::AFF_INFRAVISION, 1}};
const class_app_type::extra_affects_list_t ThiefAffects = {
	{EAffectFlag::AFF_INFRAVISION, 1},
	{EAffectFlag::AFF_SENSE_LIFE, 1},
	{EAffectFlag::AFF_BLINK, 1}};
const class_app_type::extra_affects_list_t WarriorAffects = {};
const class_app_type::extra_affects_list_t AssasineAffects = {{EAffectFlag::AFF_INFRAVISION, 1}};
const class_app_type::extra_affects_list_t GuardAffects = {};
const class_app_type::extra_affects_list_t DefenderAffects = {};
const class_app_type::extra_affects_list_t CharmerAffects = {};
const class_app_type::extra_affects_list_t NecromancerAffects = {{EAffectFlag::AFF_INFRAVISION, 1}};
const class_app_type::extra_affects_list_t PaladineAffects = {};
const class_app_type::extra_affects_list_t RangerAffects = {
	{EAffectFlag::AFF_INFRAVISION, 1},
	{EAffectFlag::AFF_SENSE_LIFE, 1}};
const class_app_type::extra_affects_list_t SmithAffects = {};
const class_app_type::extra_affects_list_t MerchantAffects = {};
const class_app_type::extra_affects_list_t DruidAffects = {};

struct class_app_type class_app[NUM_PLAYER_CLASSES] =
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
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
