/**
\authors Created by Sventovit
\date 21.01.2022.
\brief Модуль информации о навыках для умений.
\details Содержит структуры, включающие в себя всю информацию, описывающую особенности каждого навыка.
 Для добавления нового навыка нужно вписать новую константу в abilities_constants и добавить ее струкутры парсинга.
 Последнее нужно исключительно для парсинга. После этого достаточно создать в конфиге секцию описания нового навыка.
 Если помимо стандартных полей нужны какие-то относительно нестандартные действия - см. Abilities.cpp.
 Если действия нужны _совсем_ нестандартные - то это уже делается чисто кодом, который связывается с нужной
 командой посредством интерпретатора (и все равно никто не мешает использовать как минимум часть параметров из
 конфига).
*/

#ifndef BYLINS_SRC_ABILITIES_ABILITIES_INFO_H_
#define BYLINS_SRC_ABILITIES_ABILITIES_INFO_H_

#include "abilities_constants.h"
#include "abilities_items_set.h"
#include "engine/boot/cfg_manager.h"
#include "engine/structs/info_container.h"
#include "talents_effects.h"

namespace abilities {

using DataNode = parser_wrapper::DataNode;

class AbilitiesLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(DataNode data) final;
	void Reload(DataNode data) final;
};

class AbilityInfo : public info_container::BaseItem<EAbility> {
	friend class AbilityInfoBuilder;

	std::string name_;
	std::string abbr_;

// Костыльные параметры из старой системы способностей:
	bool uses_weapon_skill_{false};
	int damage_bonus_{0};
	int success_degree_damage_bonus_{0};
	int roll_bonus_{abilities::kMinRollBonus};
	int pvp_penalty{0};
	int pve_penalty{0};
	int evp_penalty{0};
	int critfail_threshold_{abilities::kDefaultCritfailThreshold};
	int critsuccess_threshold_{abilities::kDefaultCritsuccessThreshold};
	ESaving saving_{ESaving::kFirst};
	ESkill base_skill_{ESkill::kUndefined};
	EBaseStat base_stat_{EBaseStat::kStr};
	TechniqueItemKitsGroup item_kits_;

 public:
	AbilityInfo() = default;
	AbilityInfo(EAbility id, EItemMode mode)
		: BaseItem<EAbility>(id, mode) {};

	talents_effects::Effects effects;

	[[nodiscard]] const std::string &GetName() const { return name_; };
	[[nodiscard]] const std::string &GetAbbr() const { return abbr_; };
	[[nodiscard]] const char *GetCName() const { return name_.c_str(); };

	[[nodiscard]] ESkill GetBaseSkill() const { return base_skill_; };
	[[nodiscard]] ESaving GetSaving() const { return saving_; };
	[[nodiscard]] int GetBaseParameter(const CharData *ch) const;
	[[nodiscard]] int GetRollBonus() const { return roll_bonus_; };
	[[nodiscard]] int GetPvpPenalty() const { return pvp_penalty; };
	[[nodiscard]] int GetPvePenalty() const { return pve_penalty; };
	[[nodiscard]] int GetEvpPenalty() const { return evp_penalty; };
	[[nodiscard]] int GetCritsuccessThreshold() const { return critsuccess_threshold_; };
	[[nodiscard]] int GetCritfailThreshold() const { return critfail_threshold_; };
	[[nodiscard]] int GetDamageBonus() const { return damage_bonus_; };
	[[nodiscard]] int GetSuccessDegreeDamageBonus() const { return success_degree_damage_bonus_; };
	[[nodiscard]] bool IsWeaponTechnique() const { return uses_weapon_skill_; };
	[[nodiscard]] const TechniqueItemKitsGroup &GetItemKits() const { return item_kits_; };

	int (*GetEffectParameter)(const CharData *ch){[](const CharData *) { return 0; }};
	int (*CalcSituationalDamageFactor)(CharData * /* ch */){[](CharData *) { return 0; }};
	int (*CalcSituationalRollBonus)(CharData * /*ch*/, CharData * /*enemy*/){[](CharData *, CharData *) { return 0; }};

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class AbilityInfoBuilder : public info_container::IItemBuilder<AbilityInfo> {
 public:
	ItemPtr Build(DataNode &node) final;
 private:
	static ItemPtr ParseAbility(DataNode &node);
	static ItemPtr ParseHeader(DataNode &node);
	static void ParseBaseVals(ItemPtr &info, DataNode &node);
	static void ParseThresholds(ItemPtr &info, DataNode &node);
	static void ParsePenalties(ItemPtr &info, DataNode &node);
	//static void ParseEffects(ItemPtr &info, DataNode &node);
	//static void ParseActions(ItemPtr &info, DataNode &node);
	static void TemporarySetStat(ItemPtr &info);
};

using AbilitiesInfo = info_container::InfoContainer<EAbility, AbilityInfo, AbilityInfoBuilder>;


// ==============================================================================================================


/*class AbilityInfo;
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
	using IntGetter = std::function<int (CharData *)>;
	using CircumstanceHandler = std::function<bool (CharData *, CharData *)>;

	friend class AbilitiesInfo::AbilitiesInfoBuilder;

	struct CircumstanceInfo {
		CircumstanceHandler Handle;
		ECirumstance id_;
		int mod_ = 0;
		bool inverted_ = false;

		CircumstanceInfo() = delete;
*//*		explicit CircumstanceInfo(CircumstanceHandler &handler, int mod, bool inverted) :
			Handle(handler),
			mod_(mod),
			inverted_(inverted) {};*//*
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
	EBaseStat base_stat_id_;
	ESaving saving_id_;
	int difficulty_;
	int critfail_threshold_;
	int critsuccess_threshold_;
	int mob_vs_pc_penalty_;
	int pc_vs_pc_penalty_;

	MsgRegister messages_;
	IntGetter base_stat_getter_;
	IntGetter saving_getter_;
	std::forward_list<CircumstanceInfo> circumstance_handlers_;

 public:
	AbilityInfo()
		: id_(EAbility::kUndefined),
		  name_("!undefined"),
		  abbreviation_("!undefined"),
		  base_skill_id_(ESkill::kUndefined),
		  base_stat_id_(EBaseStat::kDex),
		  saving_id_(ESaving::kReflex),
		  difficulty_(kDefaultDifficulty),
		  critfail_threshold_(kDefaultCritfailThreshold),
		  critsuccess_threshold_(kDefaultCritsuccessThreshold),
		  mob_vs_pc_penalty_(kDefaultMvPPenalty),
		  pc_vs_pc_penalty_(kDefaultPvPPenalty) {};

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
	EBaseStat GetBaseStatId() const {
		return base_stat_id_;
	};
	int GetBaseStat(CharData *ch) const {
		return base_stat_getter_(ch);
	};
	ESaving GetSavingId() const {
		return saving_id_;
	}
	int GetSaving(CharData *ch) const {
		return saving_getter_(ch);
	}
	int GetDifficulty() const {
		return difficulty_;
	}
	int GetCritfailThreshold() const {
		return critfail_threshold_;
	}
	int GetCritsuccessThreshold() const {
		return critsuccess_threshold_;
	}
	int GetMVPPenalty() const {
		return mob_vs_pc_penalty_;
	}
	int GetPVPPenalty() const {
		return pc_vs_pc_penalty_;
	}
	int GetCircumstanceMod(CharData *ch, CharData *victim) const;
	const std::string &GetMsg(EAbilityMsg msg_id) const;
	const std::string &GetDefaultMsg(EAbilityMsg msg_id) const;

	std::string Print() const;
};

class AbilitiesInfo::AbilitiesInfoBuilder {
 public:
	AbilitiesInfoBuilder();

	static AbilitiesOptional Build(bool strict_parsing);

 private:
	using BaseStatGettersRegister = std::unordered_map<EBaseStat, AbilityInfo::IntGetter>;
	using SavingGettersRegister = std::unordered_map<ESaving, AbilityInfo::IntGetter>;
	using CircumstanceHandlersRegister = std::unordered_map<ECirumstance, AbilityInfo::CircumstanceHandler>;

	static BaseStatGettersRegister characteristic_getters_register_;
	static SavingGettersRegister saving_getters_register_;
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
	static void ParseBaseValues(AbilityPtr &ability, const pugi::xml_node &node);
};*/

}

#endif //BYLINS_SRC_ABILITIES_ABILITIES_INFO_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
