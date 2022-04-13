//
// Created by Alexandr on 22.01.2022.
//

#include "abilities_constants.h"

using AbilityNameIdTableType = std::unordered_map<std::string, abilities::EAbility>;
AbilityNameIdTableType ability_name_id_table;
using AbilityIdNameTableType = std::unordered_map<abilities::EAbility, std::string>;
AbilityIdNameTableType ability_id_name_table;
void InitAbilityNameIdTable() {
	ability_name_id_table["kUndefined"] = abilities::EAbility::kUndefined;
	ability_name_id_table["kBackstab"] = abilities::EAbility::kBackstab;
	ability_name_id_table["kKick"] = abilities::EAbility::kKick;
	ability_name_id_table["kThrowWeapon"] = abilities::EAbility::kThrowWeapon;
	ability_name_id_table["kTurnUndead"] = abilities::EAbility::kTurnUndead;

	for (const auto &i: ability_name_id_table) {
		ability_id_name_table[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<abilities::EAbility>(const abilities::EAbility item) {
	if (ability_id_name_table.empty()) {
		InitAbilityNameIdTable();
	}
	return ability_id_name_table.at(item);
}

template<>
abilities::EAbility ITEM_BY_NAME(const std::string &name) {
	if (ability_name_id_table.empty()) {
		InitAbilityNameIdTable();
	}
	return ability_name_id_table.at(name);
}

using AbilityMsgNameKindTableType = std::unordered_map<std::string, abilities::EAbilityMsg>;
AbilityMsgNameKindTableType ability_msg_name_kind_table;
using AbilityMsgKindNameTableType = std::unordered_map<abilities::EAbilityMsg, std::string>;
AbilityMsgKindNameTableType ability_msg_kind_name_table;
void InitAbilityMsgNameKindTable() {
	ability_msg_name_kind_table["kBasicMsgTry"] = abilities::EAbilityMsg::kBasicMsgTry;
	ability_msg_name_kind_table["kKillMsgToDamager"] = abilities::EAbilityMsg::kKillMsgToDamager;
	ability_msg_name_kind_table["kKillMsgToVictim"] = abilities::EAbilityMsg::kKillMsgToVictim;
	ability_msg_name_kind_table["kKillMsgToOnlookers"] = abilities::EAbilityMsg::kKillMsgToOnlookers;
	ability_msg_name_kind_table["kMissMsgToDamager"] = abilities::EAbilityMsg::kMissMsgToDamager;
	ability_msg_name_kind_table["kMissMsgToVictim"] = abilities::EAbilityMsg::kMissMsgToVictim;
	ability_msg_name_kind_table["kMissMsgToOnlookers"] = abilities::EAbilityMsg::kMissMsgToOnlookers;
	ability_msg_name_kind_table["kDmgMsgToDamager"] = abilities::EAbilityMsg::kDmgMsgToDamager;
	ability_msg_name_kind_table["kDmgMsgToVictim"] = abilities::EAbilityMsg::kDmgMsgToVictim;
	ability_msg_name_kind_table["kDmgMsgToOnlookers"] = abilities::EAbilityMsg::kDmgMsgToOnlookers;
	ability_msg_name_kind_table["kDmgGodMsgToDamager"] = abilities::EAbilityMsg::kDmgGodMsgToDamager;
	ability_msg_name_kind_table["kDmgGodMsgToVictim"] = abilities::EAbilityMsg::kDmgGodMsgToVictim;
	ability_msg_name_kind_table["kDmgGodMsgToOnlookers"] = abilities::EAbilityMsg::kDmgGodMsgToOnlookers;
	ability_msg_name_kind_table["kDenyMsgHaveNoAbility"] = abilities::EAbilityMsg::kDenyMsgHaveNoAbility;
	ability_msg_name_kind_table["kDenyMsgNoVictim"] = abilities::EAbilityMsg::kDenyMsgNoVictim;
	ability_msg_name_kind_table["kDenyMsgOnHorse"] = abilities::EAbilityMsg::kDenyMsgOnHorse;
	ability_msg_name_kind_table["kDenyMsgAbilityCooldown"] = abilities::EAbilityMsg::kDenyMsgAbilityCooldown;
	ability_msg_name_kind_table["kDenyMsgActorIsVictim"] = abilities::EAbilityMsg::kDenyMsgActorIsVictim;
	ability_msg_name_kind_table["kDenyMsgHaveNoWeapon"] = abilities::EAbilityMsg::kDenyMsgHaveNoWeapon;
	ability_msg_name_kind_table["kDenyMsgWrongWeapon"] = abilities::EAbilityMsg::kDenyMsgWrongWeapon;
	ability_msg_name_kind_table["kDenyMsgWrongVictim"] = abilities::EAbilityMsg::kDenyMsgWrongVictim;
	ability_msg_name_kind_table["kDenyMsgWrongPosition"] = abilities::EAbilityMsg::kDenyMsgWrongPosition;
	ability_msg_name_kind_table["kDenyMsgIsNotAbleToAct"] = abilities::EAbilityMsg::kDenyMsgIsNotAbleToAct;

	for (const auto &i: ability_msg_name_kind_table) {
		ability_msg_kind_name_table[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<abilities::EAbilityMsg>(const abilities::EAbilityMsg item) {
	if (ability_msg_name_kind_table.empty()) {
		InitAbilityMsgNameKindTable();
	}
	return ability_msg_kind_name_table.at(item);
}

template<>
abilities::EAbilityMsg ITEM_BY_NAME(const std::string &name) {
	if (ability_msg_name_kind_table.empty()) {
		InitAbilityMsgNameKindTable();
	}
	return ability_msg_name_kind_table.at(name);
}

using CircumstanceNameIdTableType = std::unordered_map<std::string, abilities::ECirumstance>;
CircumstanceNameIdTableType circumstance_name_id_table;
using CircumstanceIdNameTableType = std::unordered_map<abilities::ECirumstance, std::string>;
CircumstanceIdNameTableType circumstance_id_name_table;
void InitCircumstanceNameIdTable() {
	circumstance_name_id_table["kDarkRoom"] = abilities::ECirumstance::kDarkRoom;
	circumstance_name_id_table["kMetalEquipment"] = abilities::ECirumstance::kMetalEquipment;
	circumstance_name_id_table["kDrawingAttention"] = abilities::ECirumstance::kDrawingAttention;
	circumstance_name_id_table["kAmbushAttack"] = abilities::ECirumstance::kAmbushAttack;
	circumstance_name_id_table["kVictimSits"] = abilities::ECirumstance::kVictimSits;
	circumstance_name_id_table["kVictimBashed"] = abilities::ECirumstance::kVictimBashed;
	circumstance_name_id_table["kVictimAwareness"] = abilities::ECirumstance::kVictimAwareness;
	circumstance_name_id_table["kVictimAwake"] = abilities::ECirumstance::kVictimAwake;
	circumstance_name_id_table["kVictimHold"] = abilities::ECirumstance::kVictimHold;
	circumstance_name_id_table["kVictimSleep"] = abilities::ECirumstance::kVictimSleep;
	circumstance_name_id_table["kRoomInside"] = abilities::ECirumstance::kRoomInside;
	circumstance_name_id_table["kRoomCity"] = abilities::ECirumstance::kRoomCity;
	circumstance_name_id_table["kRoomForest"] = abilities::ECirumstance::kRoomForest;
	circumstance_name_id_table["kRoomHills"] = abilities::ECirumstance::kRoomHills;
	circumstance_name_id_table["kRoomMountain"] = abilities::ECirumstance::kRoomMountain;
	circumstance_name_id_table["kWeatherRaining"] = abilities::ECirumstance::kWeatherRaining;
	circumstance_name_id_table["kWeatherLighting"] = abilities::ECirumstance::kWeatherLighting;

	for (const auto &i: circumstance_name_id_table) {
		circumstance_id_name_table[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<abilities::ECirumstance>(const abilities::ECirumstance item) {
	if (circumstance_id_name_table.empty()) {
		InitCircumstanceNameIdTable();
	}
	return circumstance_id_name_table.at(item);
}

template<>
abilities::ECirumstance ITEM_BY_NAME(const std::string &name) {
	if (circumstance_name_id_table.empty()) {
		InitCircumstanceNameIdTable();
	}
	return circumstance_name_id_table.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp
