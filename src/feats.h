/**
\authors Created by Sventovit
\date 29.05.2022.
\brief Модуль способностей.
\details Структура с мнформацией о способностях и функцими для работы с ними - какого типа, какие эффектиы вызывают,
 при каких условиях могут быть изучены и так далее.
*/
#ifndef FILE_FEATURES_H_INCLUDED
#define FILE_FEATURES_H_INCLUDED

#include "game_abilities/abilities_items_set.h"
#include "game_abilities/abilities_constants.h"
#include "game_skills/skills.h"
#include "structs/effect.h"
#include "structs/structs.h"
#include "conf.h"
#include "game_classes/classes_constants.h"
#include "boot/cfg_manager.h"
#include "structs/info_container.h"

#include <array>
#include <bitset>

struct TimedFeat {
	EFeat feat{EFeat::kUndefined};	// Used feature //
	ubyte time{0};				// Time for next using //
	struct TimedFeat *next{nullptr};
};

int CalcMaxFeatSlotPerLvl(const CharData *ch);
int CalcFeatSlotsAmountPerRemort(CharData *ch);
EFeat FindWeaponMasterFeat(ESkill skill);
void InitFeatures();

void UnsetInaccessibleFeats(CharData *ch);
void SetRaceFeats(CharData *ch);
void UnsetRaceFeats(CharData *ch);
void SetInbornAndRaceFeats(CharData *ch);
bool CanUseFeat(const CharData *ch, EFeat feat);
bool CanGetFeat(CharData *ch, EFeat feat);
bool TryFlipActivatedFeature(CharData *ch, char *argument);
Bitvector GetPrfWithFeatNumber(EFeat feat_id);

struct FeatureInfo {
	EFeat id{EFeat::kUndefined};
	EFeatType type{EFeatType::kUnused};
	bool uses_weapon_skill{false};
	bool always_available{false};

	// Параметры для нового базового броска на способность
	// Пока тут, до переписывания системы способностей
	int damage_bonus{0};
	int success_degree_damage_bonus{0};
	int diceroll_bonus{abilities::kMinRollBonus};
	int critfail_threshold{abilities::kDefaultCritfailThreshold};
	int critsuccess_threshold{abilities::kDefaultCritsuccessThreshold};
	ESaving saving{ESaving::kFirst};
	ESkill base_skill{ESkill::kUndefined};

	TechniqueItemKitsGroup item_kits;

	int (*GetBaseParameter)(const CharData *ch);
	int (*GetEffectParameter)(const CharData *ch);
	int (*CalcSituationalDamageFactor)(CharData * /* ch */){[](CharData *) {return 0;}};
	int (*CalcSituationalRollBonus)(CharData * /*ch*/, CharData * /*enemy*/){[](CharData *, CharData *) {return 0;}};

	FeatureInfo() = default;
	explicit FeatureInfo(EFeat feat_id);
};

extern std::unordered_map<EFeat, FeatureInfo> feat_info;

// =====================================================================================================================
// =====================================================================================================================
// =====================================================================================================================

namespace feats {

using DataNode = parser_wrapper::DataNode;

class FeatsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(DataNode data) final;
	void Reload(DataNode data) final;
};

class FeatInfo : public info_container::BaseItem<EFeat> {
	friend class FeatInfoBuilder;

	std::string name_;


 public:
	FeatInfo() = default;
	FeatInfo(EFeat id, EItemMode mode)
		: BaseItem<EFeat>(id, mode) {};

	effects::PassiveEffects effects;

	[[nodiscard]] const std::string &GetName() const { return name_; };
	[[nodiscard]] const char *GetCName() const { return name_.c_str(); };

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class FeatInfoBuilder : public info_container::IItemBuilder<FeatInfo> {
 public:
	ItemPtr Build(DataNode &node) final;
 private:
	static ItemPtr ParseFeat(DataNode &node);
	static ItemPtr ParseHeader(DataNode &node);
	static void ParseEffects(ItemPtr &info, DataNode &node);
};

using FeatsInfo = info_container::InfoContainer<EFeat, FeatInfo, FeatInfoBuilder>;

}

#endif // __FEATURES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
