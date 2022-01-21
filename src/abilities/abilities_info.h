/**
\authors Created by Sventovit
\date 21.01.2022.
\brief Модуль информации о навыках для умений.
\details Содержит структуры, включающие в себя всю информацию, описывающую особенности каждого навыка.
 Для добавления нового навыка нужно вписать новую константу в abilities_constants и добавить ее в vocabulary.cpp.
 Последнее нужно исключительно для парсинга. После этого достаточно создать в конфиге секцию описания нового навыка.
 Если помимо стандартных полей нужны какие-то относительно нестандартные действия - см. abilities_info.cpp.
 Если действия нужны _совсем_ нестандартные - то это уже делается чисто кодом, который связывается с нужной
 командой посредством интерпретатора (и все равно никто не мешает использовать как минимум часть параметров из
 конфига).
*/

#ifndef BYLINS_SRC_ABILITIES_ABILITIES_INFO_H_
#define BYLINS_SRC_ABILITIES_ABILITIES_INFO_H_

#include "abilities_constants.h"
#include "boot/boot_constants.h"
#include "logger.h"
#include "skills.h"

#include <string>
#include <unordered_map>
#include <forward_list>
#include <utility>

namespace pugi {
class xml_node;
}

namespace abilities {

class AbilityInfo;
using AbilityPtr = std::unique_ptr<AbilityInfo>;
using AbilityOptional =  std::optional<AbilityPtr>;
using AbilitiesRegister = std::unordered_map<EAbility, AbilityPtr>;
using AbilitiesRegisterPtr = std::unique_ptr<AbilitiesRegister>;
using AbilitiesOptional = std::optional<AbilitiesRegisterPtr>;

class AbilitiesInfo {
 public:
	void Init();
	void Reload();
	const AbilityInfo &operator[](EAbility ability_id);

 private:
	friend class AbilityInfo;
	class AbilitiesInfoBuilder;
	AbilitiesRegisterPtr abilities_;
};

class AbilityInfo {
 private:
	using MsgRegister = std::unordered_map<EAbilityMsg, std::string>;
	using IntGetter = std::function<int (CharacterData *)>;
	using CircumstanceHandler = std::function<bool (CharacterData *, CharacterData *)>;

	friend class AbilitiesInfo::AbilitiesInfoBuilder;

	struct CircumstanceInfo {
		CircumstanceHandler Handle;
		ECirumstance id_;
		int mod_ = 0;
		bool inverted_ = false;

		CircumstanceInfo() = delete;
/*		explicit CircumstanceInfo(CircumstanceHandler &handler, int mod, bool inverted) :
			Handle(handler),
			mod_(mod),
			inverted_(inverted) {};*/
		explicit CircumstanceInfo(CircumstanceHandler &handler, ECirumstance id) :
			Handle(handler),
			id_(id) {};
	};

	static const std::string kMessageNotFound;
	static MsgRegister default_messages_;

	EAbility id_;
	std::string name_;
	std::string abbreviation_;
	ESkill base_skill_id_;
	EStat base_characteristic_id_;
//	ESaving saving_id_;
	int difficulty_;
	int critical_fail_threshold_;
	int critical_success_threshold_;
	int mob_vs_pc_penalty_;
	int pc_vs_pc_penalty_;

	MsgRegister messages_;
	IntGetter base_characteristic_getter_;
	IntGetter saving_getter_;
	std::forward_list<CircumstanceInfo> circumstance_handlers_;

 public:
	AbilityInfo()
		: id_(EAbility::kUndefined),
		  name_("!undefined"),
		  abbreviation_("!undefined"),
		  base_skill_id_(SKILL_UNDEF),
		  base_characteristic_id_(EStat::kDex),
//		  saving_id_(kSavingReflex),
		  difficulty_(kDefaultDifficulty),
		  critical_fail_threshold_(kDefaultCriticalFailThreshold),
		  critical_success_threshold_(kDefaultCriticalSuccessThreshold),
		  mob_vs_pc_penalty_(kDefaultMobVsPcPenalty),
		  pc_vs_pc_penalty_(kDefaultPcVsPcPenalty) {};

	AbilityInfo(const AbilityInfo&) = delete;
	AbilityInfo(AbilityInfo&&) = delete;

	EAbility GetId() const {
		return id_;
	}
	const std::string &GetName() const {
		return name_;
	}
	const std::string &GetAbbreviation() const {
		return abbreviation_;
	}
	ESkill GetBaseSkillId() const {
		return base_skill_id_;
	}
	EStat GetBaseCharacteristicId() const {
		return base_characteristic_id_;
	};
	int GetBaseCharacteristic(CharacterData *ch) const {
		return base_characteristic_getter_(ch);
	};
/*	ESaving GetSavingId() const {
		return saving_id_;
	}*/
	int GetSaving(CharacterData *ch) const {
		return saving_getter_(ch);
	}
	int GetDifficulty() const {
		return difficulty_;
	}
	int GetCriticalFailThreshold() const {
		return critical_fail_threshold_;
	}
	int GetCriticalSuccessThreshold() const {
		return critical_success_threshold_;
	}
	int GetMobVsPcPenalty() const {
		return mob_vs_pc_penalty_;
	}
	int GetPcVsPcPenalty() const {
		return pc_vs_pc_penalty_;
	}
	int GetCircumstanceMod(CharacterData *ch, CharacterData *victim) const;
	const std::string &GetMessage(EAbilityMsg msg_id) const;
	const std::string &GetDefaultMessage(EAbilityMsg msg_id) const;

	std::string Print() const;
};

class AbilitiesInfo::AbilitiesInfoBuilder {
 public:
	AbilitiesInfoBuilder();

	static AbilitiesOptional Build(bool strict_parsing);

 private:
	using CharacteristicGettersRegister = std::unordered_map<EStat, AbilityInfo::IntGetter>;
//	using SavingGettersRegister = std::unordered_map<ESaving, AbilityInfo::IntGetter>;
	using CircumstanceHandlersRegister = std::unordered_map<ECirumstance, AbilityInfo::CircumstanceHandler>;

	static CharacteristicGettersRegister characteristic_getters_register_;
//	static SavingGettersRegister saving_getters_register_;
	static CircumstanceHandlersRegister circumstance_handlers_register_;
	static bool strict_parsing_;

	static void BuildAbilityInfo(AbilityOptional &ability, const pugi::xml_node &node);
	static void Parse(AbilitiesOptional &abilities, pugi::xml_node &node);
	static void ParseAbilityData(AbilityPtr &ability, const pugi::xml_node &node);
	static void ParseCircumstances(AbilityPtr &ability, const pugi::xml_node &node);
	static void ParseMessages(AbilityPtr &ability, const pugi::xml_node &node);
	static void AddDefaultValues(AbilitiesRegisterPtr &abilities);
	static void EmplaceAbility(AbilitiesRegisterPtr &abilities, AbilityPtr &ability);
	static void ProcessLoadErrors(const AbilityPtr &ability, std::exception const &e);
	static void AppointHandlers(AbilitiesRegisterPtr &abilities);
	static void EmplaceCircumstanceInfo(AbilityPtr &ability, const pugi::xml_node &node);
	static void ParseBase(AbilityPtr &ability, const pugi::xml_node &node);
};

}

#endif //BYLINS_SRC_ABILITIES_ABILITIES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :