//
// Created by Sventovit on 21.01.2022.
//

/*
 *  Todo:
 *  1. Добавить поддержку нескольких сообщений одного типа.
 *  2. Подумать и, вероятно, удалить контейнеры гетеров/сетеров. Этот класс не должен знать архитектуру обработчиков.
 */

#include "abilities/abilities_info.h"

#include "entities/char_data.h"
#include "color.h"
#include "utils/pugixml/pugixml.h"
#include "game_mechanics/weather.h"

#include <functional>
#include <sstream>
#include <iostream>

using pugi::xml_node;

extern xml_node XMLLoad(const char *PathToFile, const char *MainTag, const char *ErrorStr, pugi::xml_document &Doc);

namespace abilities {

const char *kAbilityCfgFile = LIB_CFG "abilities.xml";
const char *kAbilityMainTag = "abilities";
const char *kAbilityLoadFailMsg = "Abilities loading failed.";

/*
 * Имена сущностей и атрибутов файла настроек, чтобы не искать их по всему коду при желании переименовать.
 */
const char *kValAttr = "val";
const char *kIdAttr = "id";
const char *kAbilityEntity = "ability";
const char *kNameEntity = "name";
const char *kIdEntity = "id";
const char *kBaseSkillEntity = "base_skill";
const char *kBaseStatEntity = "base_stat";
const char *kSavingEntity = "saving";
const char *kAbbreviationEntity = "abbreviation";
const char *kDifficultyEntity = "difficulty";
const char *kCritfailEntity = "critfail_threshold";
const char *kCritsuccessEntity = "critsuccess_threshold";
const char *kMvPPenaltyEntity = "mob_vs_pc_penalty";
const char *kPvPPenaltyEntity = "pc_vs_pc_penalty";
const char *kMessagesEntity = "messages";
const char *kMsgEntity = "msg_set";
const char *kCircimstancesEntity = "circumstances";
const char *kCircimstanceEntity = "circumstance";
const char *kInvertedAttr = "inverted";

const std::string AbilityInfo::kMessageNotFound = "Если вы это прочитали - у кодера какие-то проблемы.";
AbilityInfo::MsgRegister AbilityInfo::default_messages_;

bool AbilitiesInfo::AbilitiesInfoBuilder::strict_parsing_ = false;
AbilitiesInfo::AbilitiesInfoBuilder::BaseStatGettersRegister AbilitiesInfo::AbilitiesInfoBuilder::characteristic_getters_register_;
AbilitiesInfo::AbilitiesInfoBuilder::SavingGettersRegister AbilitiesInfo::AbilitiesInfoBuilder::saving_getters_register_;
AbilitiesInfo::AbilitiesInfoBuilder::CircumstanceHandlersRegister AbilitiesInfo::AbilitiesInfoBuilder::circumstance_handlers_register_;

template<typename T>
T FindConstantByName(const char *attribute_name, const xml_node &node) {
	auto constants_name = node.attribute(attribute_name).value();
	try {
		return ITEM_BY_NAME<T>(constants_name);
	} catch (const std::out_of_range &) {
		throw std::runtime_error(constants_name);
	}
}

void AbilitiesInfo::Init() {
	AbilitiesInfoBuilder builder; // Для инициализации static полей.
	abilities_ = std::move(AbilitiesInfoBuilder::Build(false).value());
};

void AbilitiesInfo::Reload() {
	auto new_abilities = AbilitiesInfoBuilder::Build(true);
	if (new_abilities) {
		abilities_ = std::move(new_abilities.value());
	} else {
		err_log("Abilities reloading was canceled: file damaged.");
	}
}

AbilitiesOptional AbilitiesInfo::AbilitiesInfoBuilder::Build(bool strict_parsing) {
	strict_parsing_ = strict_parsing;
	auto abilities = std::make_optional(std::make_unique<AbilitiesRegister>());
	pugi::xml_document doc;
	xml_node node = XMLLoad(kAbilityCfgFile, kAbilityMainTag, kAbilityLoadFailMsg, doc);
	try {
		Parse(abilities, node);
	} catch (std::exception &) {
		return std::nullopt;
	}
	AddDefaultValues(abilities.value());
	AppointHandlers(abilities.value());

	strict_parsing_ = false;
	return abilities;
}

void AbilitiesInfo::AbilitiesInfoBuilder::Parse(AbilitiesOptional &abilities, xml_node &node) {
	for (xml_node cur_node = node.child(kAbilityEntity); cur_node; cur_node = cur_node.next_sibling(kAbilityEntity)) {
		auto ability = std::make_optional(std::make_unique<AbilityInfo>());
		BuildAbilityInfo(ability, cur_node);
		if (ability) {
			EmplaceAbility(abilities.value(), ability.value());
		} else if (strict_parsing_) {
			throw std::exception();
		}
	}
}

void AbilitiesInfo::AbilitiesInfoBuilder::BuildAbilityInfo(AbilityOptional &ability, const xml_node &node) {
	try {
		ParseAbilityData(ability.value(), node);
	} catch (std::exception const &e) {
		ProcessLoadErrors(ability.value(), e);
		ability = std::nullopt;
	}
}

void AbilitiesInfo::AbilitiesInfoBuilder::ParseAbilityData(AbilityPtr &ability, const pugi::xml_node &node) {
	ParseBaseValues(ability, node);
	ParseCircumstances(ability, node);
	ParseMessages(ability, node);
}
void AbilitiesInfo::AbilitiesInfoBuilder::ParseBaseValues(AbilityPtr &ability, const xml_node &node) {
	ability->id_ = FindConstantByName<EAbility>(kValAttr, node.child(kIdEntity));
	ability->name_ = node.child(kNameEntity).attribute(kValAttr).value();
	ability->base_skill_id_ = FindConstantByName<ESkill>(kValAttr, node.child(kBaseSkillEntity));
	ability->base_stat_id_ = FindConstantByName<EBaseStat>(kValAttr, node.child(kBaseStatEntity));
	ability->saving_id_ = FindConstantByName<ESaving>(kValAttr, node.child(kSavingEntity));
	ability->abbreviation_ = node.child(kAbbreviationEntity).attribute(kValAttr).value();
	ability->difficulty_ = node.child(kDifficultyEntity).attribute(kValAttr).as_int();
	ability->critfail_threshold_ = node.child(kCritfailEntity).attribute(kValAttr).as_int();
	ability->critsuccess_threshold_ = node.child(kCritsuccessEntity).attribute(kValAttr).as_int();
	ability->mob_vs_pc_penalty_ = node.child(kMvPPenaltyEntity).attribute(kValAttr).as_int();
	ability->pc_vs_pc_penalty_ = node.child(kPvPPenaltyEntity).attribute(kValAttr).as_int();
}

void AbilitiesInfo::AbilitiesInfoBuilder::ParseCircumstances(AbilityPtr &ability, const xml_node &node) {
	xml_node cur_node = node.child(kCircimstancesEntity).child(kCircimstanceEntity);
	for (; cur_node; cur_node = cur_node.next_sibling(kCircimstanceEntity)) {
		EmplaceCircumstanceInfo(ability, cur_node);
	}
}
void AbilitiesInfo::AbilitiesInfoBuilder::EmplaceCircumstanceInfo(AbilityPtr &ability, const xml_node &node) {
/*	auto mod = node.attribute(kValAttr).as_int();
	auto inverted = node.attribute(kInvertedAttr).as_bool();
	auto handler_id = FindConstantByName<ECirumstance>(kIdAttr, node);
	ability->circumstance_handlers_ids_.push_front(handler_id);
	auto handler = circumstance_handlers_register_[handler_id];
	ability->circumstance_handlers_.emplace_front(AbilityInfo::CircumstanceInfo(handler, mod, inverted));*/
	auto handler_id = FindConstantByName<ECirumstance>(kIdAttr, node);
	auto handler = circumstance_handlers_register_[handler_id];
	ability->circumstance_handlers_.emplace_front(AbilityInfo::CircumstanceInfo(handler, handler_id));
	ability->circumstance_handlers_.front().mod_ = node.attribute(kValAttr).as_int();
	ability->circumstance_handlers_.front().inverted_ = node.attribute(kInvertedAttr).as_bool();
}

void AbilitiesInfo::AbilitiesInfoBuilder::ParseMessages(AbilityPtr &ability, const xml_node &node) {
	xml_node msg_node = node.child(kMessagesEntity).child(kMsgEntity);
	for (; msg_node; msg_node = msg_node.next_sibling(kMsgEntity)) {
		auto msg_id = FindConstantByName<EAbilityMsg>(kIdAttr, msg_node);
		auto it = ability->messages_.try_emplace(msg_id, msg_node.attribute(kValAttr).value());
		if (!it.second) {
			err_log("Ability '%s' has two or more messages of the same type '%s'.\n",
					NAME_BY_ITEM<EAbility>(ability->GetId()).c_str(), NAME_BY_ITEM<EAbilityMsg>(msg_id).c_str());
		}
	}
}

void AbilitiesInfo::AbilitiesInfoBuilder::ProcessLoadErrors(const AbilityPtr &ability, std::exception const &e) {
	err_log("Incorrect value or message type '%s' detected in ability '%s'. Item had been skipped.",
			e.what(), NAME_BY_ITEM<EAbility>(ability->GetId()).c_str());
}

void AbilitiesInfo::AbilitiesInfoBuilder::EmplaceAbility(AbilitiesRegisterPtr &abilities, AbilityPtr &ability) {
	auto it = abilities->try_emplace(ability->GetId(), std::move(ability));
	if (!it.second) {
		err_log("Ability '%s' has already exist. Redundant definition had been ignored.\n",
				NAME_BY_ITEM<EAbility>(it.first->second->GetId()).c_str());
	}
}

void AbilitiesInfo::AbilitiesInfoBuilder::AddDefaultValues(AbilitiesRegisterPtr &abilities) {
	// Всегда должна быть секция "kAbilityUndefined" со значениями по умолчанию, даже если её нет в файле.
	abilities->try_emplace(EAbility::kUndefined, std::make_unique<AbilityInfo>());
	// Сообщения секции "kAbilityUndefined" - это сообщения "по умолчанию", чтобы не писать для них отдельную секцию и парсинг.
	abilities->at(EAbility::kUndefined)->default_messages_.merge(abilities->at(EAbility::kUndefined)->messages_);
}

/// \todo Если убирать "принт", то есть смысл убрать и сохранение идов, добавлять сразу геттер - дурацкая идея
/// Либо наоборот, добавить хранение идов.
void AbilitiesInfo::AbilitiesInfoBuilder::AppointHandlers(AbilitiesRegisterPtr &abilities) {
	for (auto &item : *abilities) {
		item.second->base_stat_getter_ =
			characteristic_getters_register_[item.second->GetBaseStatId()];
		item.second->saving_getter_ = saving_getters_register_[item.second->GetSavingId()];
	}
}

AbilitiesInfo::AbilitiesInfoBuilder::AbilitiesInfoBuilder() {
	if (characteristic_getters_register_.empty()) {
		characteristic_getters_register_[EBaseStat::kDex] = GET_REAL_DEX;
		characteristic_getters_register_[EBaseStat::kStr] = GET_REAL_STR;
		characteristic_getters_register_[EBaseStat::kCon] = GET_REAL_CON;
		characteristic_getters_register_[EBaseStat::kInt] = GET_REAL_INT;
		characteristic_getters_register_[EBaseStat::kWis] = GET_REAL_WIS;
		characteristic_getters_register_[EBaseStat::kCha] = GET_REAL_CHA;
	}

	if (saving_getters_register_.empty()) {
/*		saving_getters_register_[ESaving::kWill] = GET_TOTAL_SAVING_WILL;
		saving_getters_register_[ESaving::kReflex] = GET_TOTAL_SAVING_REFLEX;
		saving_getters_register_[ESaving::kCritical] = GET_TOTAL_SAVING_CRITICAL;
		saving_getters_register_[ESaving::kStability] = GET_TOTAL_SAVING_STABILITY;*/
	}

	if (circumstance_handlers_register_.empty()) {
		circumstance_handlers_register_[ECirumstance::kDarkRoom] =
			[](CharData */*ch*/, CharData * /* victim */) -> bool {
//				return IsDark(ch->in_room);
				return false; //заглушка
			};
		circumstance_handlers_register_[ECirumstance::kMetalEquipment] =
			[](CharData */*ch*/, CharData * /* victim */ ) -> bool {
//				return IsEquipInMetal(ch);
				return false; //заглушка
			};
		circumstance_handlers_register_[ECirumstance::kDrawingAttention] =
			[](CharData *ch, CharData */* victim */) -> bool {
				return (AFF_FLAGGED(ch, EAffectFlag::AFF_STAIRS)
					|| AFF_FLAGGED(ch, EAffectFlag::AFF_SANCTUARY)
					|| AFF_FLAGGED(ch, EAffectFlag::AFF_SINGLELIGHT)
					|| AFF_FLAGGED(ch, EAffectFlag::AFF_HOLYLIGHT));
			};
		circumstance_handlers_register_[ECirumstance::kAmbushAttack] =
			[](CharData *ch, CharData *victim) -> bool {
				return !CAN_SEE(victim, ch);
			};
		circumstance_handlers_register_[ECirumstance::kVictimSits] =
			[](CharData * /* ch */, CharData *victim) -> bool {
				return (victim->char_specials.position ==  EPosition::kSit);
			};
		circumstance_handlers_register_[ECirumstance::kVictimAwareness] =
			[](CharData * /* ch */, CharData *victim) -> bool {
				return AFF_FLAGGED(victim, EAffectFlag::AFF_AWARNESS);
			};
		circumstance_handlers_register_[ECirumstance::kVictimAwake] =
			[](CharData * /* ch */, CharData *victim) -> bool {
				return PRF_FLAGGED(victim, PRF_AWAKE);
			};
		circumstance_handlers_register_[ECirumstance::kVictimHold] =
			[](CharData * /* ch */, CharData *victim) -> bool {
				return AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD);
			};
		circumstance_handlers_register_[ECirumstance::kVictimSleep] =
			[](CharData * /* ch */, CharData *victim) -> bool {
				return (victim->char_specials.position ==  EPosition::kSleep);
			};
		circumstance_handlers_register_[ECirumstance::kRoomInside] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (SECT(ch->in_room) == kSectInside);
			};
		circumstance_handlers_register_[ECirumstance::kRoomCity] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (SECT(ch->in_room) == kSectCity);
			};
		circumstance_handlers_register_[ECirumstance::kRoomForest] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (SECT(ch->in_room) == kSectForest);
			};
		circumstance_handlers_register_[ECirumstance::kRoomHills] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (SECT(ch->in_room) == kSectHills);
			};
		circumstance_handlers_register_[ECirumstance::kRoomMountain] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (SECT(ch->in_room) == kSectMountain);
			};
		circumstance_handlers_register_[ECirumstance::kWeatherRaining] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (GET_ROOM_SKY(ch->in_room) == kSkyRaining);
			};
		circumstance_handlers_register_[ECirumstance::kWeatherLighting] =
			[](CharData *ch, CharData * /* victim */) -> bool {
				return (GET_ROOM_SKY(ch->in_room) == kSkyLightning);
			};
	}
};

const AbilityInfo &AbilitiesInfo::operator[](EAbility ability_id) {
	try {
		return *abilities_->at(ability_id);
	} catch (const std::out_of_range &) {
		err_log("Incorrect ability id (%d) passed into Abilities", to_underlying(ability_id));
		return *abilities_->at(EAbility::kUndefined);
	}
}

std::string AbilityInfo::Print() const {
	std::stringstream buffer;
	buffer << "Print ability:" << "\n"
		   << " Id: " << KGRN << NAME_BY_ITEM<EAbility>(GetId()) << KNRM << "\n"
		   << " Name: " << KGRN << GetName() << KNRM << "\n"
		   << " Abbreviation: " << KGRN << GetAbbreviation() << KNRM << "\n"
		   << " Base skill: " << KGRN << NAME_BY_ITEM<ESkill>(GetBaseSkillId()) << KNRM << "\n"
		   << " Base characteristic: " << KGRN << NAME_BY_ITEM<EBaseStat>(GetBaseStatId()) << KNRM
		   << "\n"
		   << " Saving type: " << KGRN << NAME_BY_ITEM<ESaving>(GetSavingId()) << KNRM << "\n"
		   << " Difficulty: " << KGRN << GetDifficulty() << KNRM << "\n"
		   << " Critical fail threshold: " << KGRN << GetCritfailThreshold() << KNRM << "\n"
		   << " Critical success threshold: " << KGRN << GetCritsuccessThreshold() << KNRM << "\n"
		   << " Mob vs PC penalty: " << KGRN << GetMVPPenalty() << KNRM << "\n"
		   << " PC vs PC penalty: " << KGRN << GetPVPPenalty() << KNRM << "\n"
		   << " Circumstance modificators:\n";
/*	for (auto &item : circumstance_handlers_) {
		buffer << "   " << NAME_BY_ITEM<ECirumstance>(item.id_) << ": " << KGRN << item.mod_ << KNRM << "\n";
	}*/
/*		   << " Messages:" << "\n";
	for (auto &it : messages_) {
		buffer << "   " << NAME_BY_ITEM<EAbilityMsg>(it.first) << ": " << KGRN << it.second << KNRM << "\n";
	}*/
	buffer << std::endl;
	return buffer.str();
}

int AbilityInfo::GetCircumstanceMod(CharData *ch, CharData *victim) const {
	int modificator = 0;
	for (auto &item : circumstance_handlers_) {
		if (item.inverted_ != item.Handle(ch, victim)) {
			std::cout << "Circumstance " << NAME_BY_ITEM<ECirumstance>(item.id_) << " bonus: " << item.mod_ << "\n";
			modificator += item.mod_;
		}
	}
	return modificator;
};

const std::string &AbilityInfo::GetMsg(const EAbilityMsg msg_id) const {
	try {
		return messages_.at(msg_id);
	} catch (const std::out_of_range &) {
		return GetDefaultMsg(msg_id);
	}
}

const std::string &AbilityInfo::GetDefaultMsg(const EAbilityMsg msg_id) const {
	try {
		return default_messages_.at(msg_id);
	} catch (const std::out_of_range &) {
		err_log("Message Id '%s' not found in ability '%s' or in default messages!",
				NAME_BY_ITEM<EAbilityMsg>(msg_id).c_str(), name_.c_str());
		return kMessageNotFound;
	}
}

}; // abilities

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
