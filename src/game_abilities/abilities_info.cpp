//
// Created by Sventovit on 21.01.2022.
//

#include "abilities_info.h"

//#include "entities/char_data.h"
#include "action_targeting.h"
#include "color.h"
//#include "utils/parser_wrapper.h"
#include "structs/global_objects.h"
//#include "game_mechanics/weather.h"


namespace abilities {

using ItemPtr = AbilityInfoBuilder::ItemPtr;

int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */);

void AbilitiesLoader::Load(DataNode data) {
	MUD::Abilities().Init(data.Children());
}

void AbilitiesLoader::Reload(DataNode data) {
	MUD::Abilities().Reload(data.Children());
}

ItemPtr AbilityInfoBuilder::Build(DataNode &node) {
	try {
		return ParseAbility(node);
	} catch (std::exception &e) {
		err_log("Ability parsing error (incorrect value '%s')", e.what());
		return nullptr;
	}
}

ItemPtr AbilityInfoBuilder::ParseAbility(DataNode &node) {
	auto info = ParseHeader(node);

	//ParseEffects(info, node);
	TemporarySetStat(info);
	return info;
}

ItemPtr AbilityInfoBuilder::ParseHeader(DataNode &node) {
	auto id{EAbility::kUndefined};
	try {
		id = parse::ReadAsConstant<EAbility>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect ability id (%s).", e.what());
		throw;
	}
	auto mode = AbilityInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	auto info = std::make_shared<AbilityInfo>(id, mode);
	try {
		info->name_ = parse::ReadAsStr(node.GetValue("name"));
		info->abbr_ = parse::ReadAsStr(node.GetValue("abbr"));
	} catch (std::exception &) {
	}

	return info;
}

void AbilityInfoBuilder::TemporarySetStat(ItemPtr &info) {
	switch (info->GetId()) {
		case EAbility::kScirmisher: {
			info->diceroll_bonus_ = 90;
			info->base_skill_ = ESkill::kRescue;
			info->saving_ = ESaving::kReflex;
			info->GetBaseParameter = &GET_REAL_DEX;
			info->CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
			break;
		}
		case EAbility::kTactician: {
			info->diceroll_bonus_ = 90;
			info->base_skill_ = ESkill::kLeadership;
			info->saving_ = ESaving::kReflex;
			info->GetBaseParameter = &GET_REAL_CHA;
			info->CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
			break;
		}
		case EAbility::kCutting: {
			info->diceroll_bonus_ = 110;
			info->saving_ = ESaving::kCritical;
			info->GetBaseParameter = &GET_REAL_DEX;
			info->GetEffectParameter = &GET_REAL_STR;
			info->uses_weapon_skill_ = true;
			info->damage_bonus_ = 15;
			info->success_degree_damage_bonus_ = 5;
			info->CalcSituationalRollBonus =
				([](CharData */*ch*/, CharData *enemy) -> int {
					int bonus{0};
					if (AFF_FLAGGED(enemy, EAffect::kBlind)) {
						bonus += 40;
					}
					if (GET_POS(enemy) < EPosition::kFight) {
						bonus += 40;
					}
					return bonus;
				});

			info->item_kits_.reserve(4);

			auto item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(2);
			item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kShortBlades));
			item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kShortBlades));
			info->item_kits_.push_back(std::move(item_kit));

			item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(2);
			item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kLongBlades));
			item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kLongBlades));
			info->item_kits_.push_back(std::move(item_kit));

			item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(2);
			item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kSpades));
			item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kSpades));
			info->item_kits_.push_back(std::move(item_kit));

			item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(2);
			item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon, ESkill::kPicks));
			item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon, ESkill::kPicks));
			info->item_kits_.push_back(std::move(item_kit));
			break;
		}
		case EAbility::kThrowWeapon: {
			info->diceroll_bonus_ = 100;
			info->base_skill_ = ESkill::kThrow;
			info->saving_ = ESaving::kReflex;
			info->GetBaseParameter = &GET_REAL_DEX;
			info->GetEffectParameter = &GET_REAL_STR;
			info->uses_weapon_skill_ = false;
			info->damage_bonus_ = 20;
			info->success_degree_damage_bonus_ = 5;
			info->CalcSituationalDamageFactor =
				([](CharData *ch) -> int {
					return (CanUseFeat(ch, EFeat::kPowerThrow) + CanUseFeat(ch, EFeat::kDeadlyThrow));
				});
			info->CalcSituationalRollBonus =
				([](CharData *ch, CharData * /* enemy */) -> int {
					if (AFF_FLAGGED(ch, EAffect::kBlind)) {
						return -60;
					}
					return 0;
				});

			info->item_kits_.reserve(2);

			auto item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(1);
			item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon,
											  ESkill::kAny, EObjFlag::kThrowing));
			info->item_kits_.push_back(std::move(item_kit));

			item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(1);
			item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon,
											  ESkill::kAny, EObjFlag::kThrowing));
			info->item_kits_.push_back(std::move(item_kit));
			break;
		}
		case EAbility::kShadowThrower: {
			info->diceroll_bonus_ = 100;
			info->base_skill_ = ESkill::kDarkMagic;
			info->saving_ = ESaving::kWill;
			info->GetBaseParameter = &GET_REAL_DEX;
			info->GetEffectParameter = &GET_REAL_INT;
			info->damage_bonus_ = -30;
			info->success_degree_damage_bonus_ = 1;
			info->uses_weapon_skill_ = false;
			info->CalcSituationalRollBonus =
				([](CharData *ch, CharData * /* enemy */) -> int {
					if (AFF_FLAGGED(ch, EAffect::kBlind)) {
						return -60;
					}
					return 0;
				});

			info->item_kits_.reserve(2);
			auto item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(1);
			item_kit->push_back(TechniqueItem(EEquipPos::kWield, EObjType::kWeapon,
											  ESkill::kAny, EObjFlag::kThrowing));
			info->item_kits_.push_back(std::move(item_kit));
			item_kit = std::make_unique<TechniqueItemKit>();
			item_kit->reserve(1);
			item_kit->push_back(TechniqueItem(EEquipPos::kHold, EObjType::kWeapon,
											  ESkill::kAny, EObjFlag::kThrowing));
			info->item_kits_.push_back(std::move(item_kit));
			break;
		}
		case EAbility::kShadowDagger:
		case EAbility::kShadowSpear: [[fallthrough]];
		case EAbility::kShadowClub: {
			info->diceroll_bonus_ = 80;
			info->base_skill_ = ESkill::kDarkMagic;
			info->saving_ = ESaving::kStability;
			info->GetBaseParameter = &GET_REAL_INT;
			info->uses_weapon_skill_ = false;
			break;
		}
		case EAbility::kDoubleThrower: [[fallthrough]];
		case EAbility::kTripleThrower: {
			info->diceroll_bonus_ = 100;
			info->saving_ = ESaving::kReflex;
			info->GetBaseParameter = &GET_REAL_DEX;
			break;
		}
		case EAbility::kPowerThrow: [[fallthrough]];
		case EAbility::kDeadlyThrow: {
			info->diceroll_bonus_ = 100;
			info->saving_ = ESaving::kReflex;
			info->GetBaseParameter = &GET_REAL_STR;
			break;
		}
		case EAbility::kTurnUndead: {
			info->diceroll_bonus_ = 70;
			info->base_skill_ = ESkill::kTurnUndead;
			info->saving_ = ESaving::kStability;
			info->GetBaseParameter = &GET_REAL_INT;
			info->GetEffectParameter = &GET_REAL_WIS;
			info->uses_weapon_skill_ = false;
			info->damage_bonus_ = -30;
			info->success_degree_damage_bonus_ = 2;
			break;
		}
		default: break;
	}
}

/*
* Ситуативный бонус броска для "tactician feat" и "skirmisher feat":
* Каждый персонаж в строю прикрывает двух, третий дает штраф.
* Избыток "строевиков" повышает шанс на удачное срабатывание.
* Svent TODO: Придумать более универсальный механизм бонусов/штрафов в зависимости от данных абилки
*/
int CalcRollBonusOfGroupFormation(CharData *ch, CharData * /* enemy */) {
	ActionTargeting::FriendsRosterType roster{ch};
	int skirmishers = roster.count([](CharData *ch) { return PRF_FLAGGED(ch, EPrf::kSkirmisher); });
	int uncoveredSquadMembers = roster.amount() - skirmishers;
	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		return (skirmishers*2 - uncoveredSquadMembers)*abilities::kCircumstanceFactor - 40;
	};
	return (skirmishers*2 - uncoveredSquadMembers)*abilities::kCircumstanceFactor;
};

void AbilityInfo::Print(CharData * /*ch*/, std::ostringstream &buffer) const {
	buffer << "Print ability:" << "\n"
		   << " Id: " << KGRN << NAME_BY_ITEM<EAbility>(GetId()) << KNRM << "\n"
		   << " Name: " << KGRN << GetName() << KNRM << "\n"
		   << " Abbr: " << KGRN << GetAbbr() << KNRM << "\n";
/*		   << " Base skill: " << KGRN << NAME_BY_ITEM<ESkill>(GetBaseSkillId()) << KNRM << "\n"
		   << " Base characteristic: " << KGRN << NAME_BY_ITEM<EBaseStat>(GetBaseStatId()) << KNRM
		   << "\n"
		   << " Saving type: " << KGRN << NAME_BY_ITEM<ESaving>(GetSavingId()) << KNRM << "\n"
		   << " Difficulty: " << KGRN << GetDifficulty() << KNRM << "\n"
		   << " Critical fail threshold: " << KGRN << GetCritfailThreshold() << KNRM << "\n"
		   << " Critical success threshold: " << KGRN << GetCritsuccessThreshold() << KNRM << "\n"
		   << " Mob vs PC penalty: " << KGRN << GetMVPPenalty() << KNRM << "\n"
		   << " PC vs PC penalty: " << KGRN << GetPVPPenalty() << KNRM << "\n"
		   << " Circumstance modificators:\n";*/
//	for (auto &item : circumstance_handlers_) {
//		buffer << "   " << NAME_BY_ITEM<ECirumstance>(item.id_) << ": " << KGRN << item.mod_ << KNRM << "\n";
//	}
//		   << " Messages:" << "\n";
//	for (auto &it : messages_) {
//		buffer << "   " << NAME_BY_ITEM<EAbilityMsg>(it.first) << ": " << KGRN << it.second << KNRM << "\n";
//	}
	buffer << std::endl;
}

}; // abilities

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
