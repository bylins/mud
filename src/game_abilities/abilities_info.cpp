//
// Created by Sventovit on 21.01.2022.
//

#include "abilities_info.h"

#include "entities/char_data.h"
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

	ParseBaseVals(info, node);
	ParseThresholds(info, node);
	ParsePenalties(info, node);
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

void AbilityInfoBuilder::ParseBaseVals(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("base")) {
		info->base_skill_ = parse::ReadAsConstant<ESkill>(node.GetValue("skill"));
		info->base_stat_ = parse::ReadAsConstant<EBaseStat>(node.GetValue("stat"));
		info->saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
		info->roll_bonus_ = parse::ReadAsInt(node.GetValue("roll_bonus"));
		node.GoToParent();
	}
}

void AbilityInfoBuilder::ParseThresholds(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("thresholds")) {
		info->critsuccess_threshold_ = parse::ReadAsInt(node.GetValue("critsuccess"));
		info->critfail_threshold_ = parse::ReadAsInt(node.GetValue("critfail"));
		node.GoToParent();
	}
}

void AbilityInfoBuilder::ParsePenalties(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("penalties")) {
		info->pvp_penalty = parse::ReadAsInt(node.GetValue("pvp"));
		info->pve_penalty = parse::ReadAsInt(node.GetValue("pve"));
		info->evp_penalty = parse::ReadAsInt(node.GetValue("evp"));
		node.GoToParent();
	}
}

void AbilityInfoBuilder::TemporarySetStat(ItemPtr &info) {
	switch (info->GetId()) {
		case EAbility::kScirmisher: [[fallthrough]];
		case EAbility::kTactician: {
			info->CalcSituationalRollBonus = &CalcRollBonusOfGroupFormation;
			break;
		}
		case EAbility::kCutting: {
			info->GetEffectParameter = &GetRealStr;
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
			info->GetEffectParameter = &GetRealStr;
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
			info->GetEffectParameter = &GetRealInt;
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
			info->uses_weapon_skill_ = false;
			break;
		}
		case EAbility::kTurnUndead: {
			info->GetEffectParameter = &GetRealWis;
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
		return (skirmishers * 2 - uncoveredSquadMembers) * abilities::kCircumstanceFactor - 40;
	};
	return (skirmishers * 2 - uncoveredSquadMembers) * abilities::kCircumstanceFactor;
};

void AbilityInfo::Print(CharData * /*ch*/, std::ostringstream &buffer) const {
	buffer << "Print ability:" << "\r\n"
		   << " Id: " << KGRN << NAME_BY_ITEM<EAbility>(GetId()) << KNRM << "\r\n"
		   << " Name: " << KGRN << GetName() << KNRM << "\r\n"
		   << " Abbr: " << KGRN << GetAbbr() << KNRM << "\r\n"
		   << " Base skill: " << KGRN << NAME_BY_ITEM<ESkill>(base_skill_) << KNRM << "\r\n"
		   << " Base stat: " << KGRN << NAME_BY_ITEM<EBaseStat>(base_stat_) << KNRM << "\r\n"
		   << " Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving_) << KNRM << "\r\n"
		   << " Roll bonus: " << KGRN << roll_bonus_ << KNRM << "\r\n"
		   << " Penalties [PvP/PvE/EvP]: "
		   << KGRN << pvp_penalty << "/" << pve_penalty << "/" << evp_penalty << KNRM << "\r\n"
		   << " Critical thresholds [success/fail]: "
		   << KGRN << "critsuccess - " << critsuccess_threshold_ << "/" << critfail_threshold_ << KNRM << "\r\n";

/*		   << "\n"
		   << " Circumstance modificators:\n";*/
//	for (auto &item : circumstance_handlers_) {
//		buffer << "   " << NAME_BY_ITEM<ECirumstance>(item.id_) << ": " << KGRN << item.mod_ << KNRM << "\n";
//	}
//		   << " Messages:" << "\n";
//	for (auto &it : messages_) {
//		buffer << "   " << NAME_BY_ITEM<EAbilityMsg>(it.first) << ": " << KGRN << it.second << KNRM << "\n";
//	}
	buffer << "\r\n";
}

int AbilityInfo::GetBaseParameter(const CharData *ch) const {
	return GetRealBaseStat(ch, base_stat_);
}

}; // abilities

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
