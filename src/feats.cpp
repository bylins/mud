//#include "feats.h"
//#include "feats_constants.h"

#include <third_party_libs/fmt/include/fmt/core.h>

#include "entities/player_races.h"
#include "color.h"
#include "game_magic/magic_utils.h"
#include "structs/global_objects.h"

/* Служебные функции */
bool CheckVacantFeatSlot(CharData *ch, EFeat feat);

/* Функции для работы с переключаемыми способностями */
void ActivateFeat(CharData *ch, EFeat feat_id);
void DeactivateFeature(CharData *ch, EFeat feat_id);

/* Extern */
extern void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);

bool CanUseFeat(const CharData *ch, EFeat feat_id) {
	if (MUD::Feat(feat_id).IsInvalid() || !ch->HaveFeat(feat_id)) {
		return false;
	};
	if (ch->IsNpc()) {
		return true;
	};
	if (CalcMaxFeatSlotPerLvl(ch) < MUD::Class(ch->GetClass()).feats[feat_id].GetSlot()) {
		return false;
	};
	if (GetRealRemort(ch) < MUD::Class(ch->GetClass()).feats[feat_id].GetMinRemort()) {
		return false;
	};

	switch (feat_id) {
		case EFeat::kWeaponFinesse:
		case EFeat::kFInesseShot: return (GetRealDex(ch) > GetRealStr(ch) && GetRealDex(ch) > 17);
		case EFeat::kParryArrow: return (GetRealDex(ch) > 15);
		case EFeat::kPowerAttack: return (GetRealStr(ch) > 19);
		case EFeat::kGreatPowerAttack: return (GetRealStr(ch) > 21);
		case EFeat::kAimingAttack: return (GetRealDex(ch) > 15);
		case EFeat::kGreatAimingAttack: return (GetRealDex(ch) > 17);
		case EFeat::kDoubleShot: return (ch->GetSkill(ESkill::kBows) > 39);
		case EFeat::kJeweller: return (ch->GetSkill(ESkill::kJewelry) > 59);
		case EFeat::kSkilledTrader: return ((GetRealLevel(ch) + GetRealRemort(ch) / 3) > 19);
		case EFeat::kMagicUser: return (GetRealLevel(ch) < 25);
		case EFeat::kLiveShield: return (ch->GetSkill(ESkill::kRescue) > 124);
		case EFeat::kShadowThrower: return (ch->GetSkill(ESkill::kDarkMagic) > 120);
		case EFeat::kAnimalMaster: return (ch->GetSkill(ESkill::kMindMagic) > 79);
		case EFeat::kScirmisher: return !(AFF_FLAGGED(ch, EAffect::kStopFight) ||
			AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
			GET_POS(ch) < EPosition::kFight);
		default: return true;
	}
}

bool CanGetFocusFeat(const CharData *ch, const EFeat feat_id) {
	int count{0};
	for (auto focus_feat_id = EFeat::kPunchFocus; focus_feat_id <= EFeat::kBowsFocus; ++focus_feat_id) {
		if (ch->HaveFeat(focus_feat_id)) {
			count++;
		}
	}

	if (count >= 2 + GetRealRemort(ch) / 6) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchFocus: return ch->GetSkillWithoutEquip(ESkill::kPunch);
		case EFeat::kClubsFocus: return ch->GetSkillWithoutEquip(ESkill::kClubs);
		case EFeat::kAxesFocus: return ch->GetSkillWithoutEquip(ESkill::kAxes);
		case EFeat::kLongsFocus: return ch->GetSkillWithoutEquip(ESkill::kLongBlades);
		case EFeat::kShortsFocus: return ch->GetSkillWithoutEquip(ESkill::kShortBlades);
		case EFeat::kNonstandartsFocus: return ch->GetSkillWithoutEquip(ESkill::kNonstandart);
		case EFeat::kTwohandsFocus: return ch->GetSkillWithoutEquip(ESkill::kTwohands);
		case EFeat::kPicksFocus: return ch->GetSkillWithoutEquip(ESkill::kPicks);
		case EFeat::kSpadesFocus: return ch->GetSkillWithoutEquip(ESkill::kSpades);
		case EFeat::kBowsFocus: return ch->GetSkillWithoutEquip(ESkill::kBows);
		default: return false;
	}
}

bool CanGetMasterFeat(const CharData *ch, const EFeat feat_id) {
	int count{0};
	for (auto master_feat = EFeat::kPunchMaster; master_feat <= EFeat::kBowsMaster; ++master_feat) {
		if (ch->HaveFeat(master_feat)) {
			count++;
		}
	}
	if (count >= 1 + GetRealRemort(ch) / 7) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchMaster:
			return ch->HaveFeat(EFeat::kPunchFocus) && ch->GetSkillWithoutEquip(ESkill::kPunch);
		case EFeat::kClubsMaster:
			return ch->HaveFeat(EFeat::kClubsFocus) && ch->GetSkillWithoutEquip(ESkill::kClubs);
		case EFeat::kAxesMaster:
			return ch->HaveFeat(EFeat::kAxesFocus) && ch->GetSkillWithoutEquip(ESkill::kAxes);
		case EFeat::kLongsMaster:
			return ch->HaveFeat(EFeat::kLongsFocus) && ch->GetSkillWithoutEquip(ESkill::kLongBlades);
		case EFeat::kShortsMaster:
			return ch->HaveFeat(EFeat::kShortsFocus) && ch->GetSkillWithoutEquip(ESkill::kShortBlades);
		case EFeat::kNonstandartsMaster:
			return ch->HaveFeat(EFeat::kNonstandartsFocus) && ch->GetSkillWithoutEquip(ESkill::kNonstandart);
		case EFeat::kTwohandsMaster:
			return ch->HaveFeat(EFeat::kTwohandsFocus) && ch->GetSkillWithoutEquip(ESkill::kTwohands);
		case EFeat::kPicksMaster:
			return ch->HaveFeat(EFeat::kPicksFocus) && ch->GetSkillWithoutEquip(ESkill::kPicks);
		case EFeat::kSpadesMaster:
			return ch->HaveFeat(EFeat::kSpadesFocus) && ch->GetSkillWithoutEquip(ESkill::kSpades);
		case EFeat::kBowsMaster:
			return ch->HaveFeat(EFeat::kBowsFocus) && ch->GetSkillWithoutEquip(ESkill::kBows);
		default: return false;
	}
}

bool CanGetFeat(CharData *ch, EFeat feat) {
	if (MUD::Feat(feat).IsInvalid()) {
		return false;
	}

	if ((MUD::Class(ch->GetClass()).feats.IsUnavailable(feat) &&
		!PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat))) ||
		(GetRealRemort(ch) < MUD::Class(ch->GetClass()).feats[feat].GetMinRemort())) {
		return false;
	}

	if (!CheckVacantFeatSlot(ch, feat)) {
		return false;
	}

	switch (feat) {
		case EFeat::kParryArrow:
			return ch->GetSkillWithoutEquip(ESkill::kMultiparry) ||
				ch->GetSkillWithoutEquip(ESkill::kParry);
		case EFeat::kConnoiseur:
			return ch->GetSkillWithoutEquip(ESkill::kIdentify);
		case EFeat::kExorcist:
			return ch->GetSkillWithoutEquip(ESkill::kTurnUndead);
		case EFeat::kHealer:
			return ch->GetSkillWithoutEquip(ESkill::kFirstAid);
		case EFeat::kStealthy:
			return ch->GetSkillWithoutEquip(ESkill::kHide) ||
				ch->GetSkillWithoutEquip(ESkill::kSneak) ||
				ch->GetSkillWithoutEquip(ESkill::kDisguise);
		case EFeat::kTracker:
			return (ch->GetSkillWithoutEquip(ESkill::kTrack) ||
				ch->GetSkillWithoutEquip(ESkill::kSense));
		case EFeat::kPunchMaster:
		case EFeat::kClubsMaster:
		case EFeat::kAxesMaster:
		case EFeat::kLongsMaster:
		case EFeat::kShortsMaster:
		case EFeat::kNonstandartsMaster:
		case EFeat::kTwohandsMaster:
		case EFeat::kPicksMaster:
		case EFeat::kSpadesMaster: [[fallthrough]];
		case EFeat::kBowsMaster:
			return CanGetMasterFeat(ch, feat);
		case EFeat::kWarriorSpirit:
			return ch->HaveFeat(EFeat::kGreatFortitude);
		case EFeat::kNimbleFingers:
			return ch->GetSkillWithoutEquip(ESkill::kSteal) ||
				ch->GetSkillWithoutEquip(ESkill::kPickLock);
		case EFeat::kGreatPowerAttack:
			return ch->HaveFeat(EFeat::kPowerAttack);
		case EFeat::kPunchFocus:
		case EFeat::kClubsFocus:
		case EFeat::kAxesFocus:
		case EFeat::kLongsFocus:
		case EFeat::kShortsFocus:
		case EFeat::kNonstandartsFocus:
		case EFeat::kTwohandsFocus:
		case EFeat::kPicksFocus:
		case EFeat::kSpadesFocus: [[fallthrough]];
		case EFeat::kBowsFocus:
			return CanGetFocusFeat(ch, feat);
		case EFeat::kGreatAimingAttack:
			return ch->HaveFeat(EFeat::kAimingAttack);
		case EFeat::kDoubleShot:
			return ch->HaveFeat(EFeat::kBowsFocus) && ch->GetSkillWithoutEquip(ESkill::kBows) > 39;
		case EFeat::kJeweller:
			return ch->GetSkillWithoutEquip(ESkill::kJewelry) > 59;
		case EFeat::kCutting:
			return ch->HaveFeat(EFeat::kShortsMaster) ||
				ch->HaveFeat(EFeat::kPicksMaster) ||
				ch->HaveFeat(EFeat::kLongsMaster) ||
				ch->HaveFeat(EFeat::kSpadesMaster);
		case EFeat::kScirmisher:
			return ch->GetSkillWithoutEquip(ESkill::kRescue);
		case EFeat::kTactician:
			return ch->GetSkillWithoutEquip(ESkill::kLeadership) > 99;
		case EFeat::kShadowThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kDarkMagic) > 120;
		case EFeat::kShadowDagger:
		case EFeat::kShadowSpear: [[fallthrough]];
		case EFeat::kShadowClub:
			return ch->HaveFeat(EFeat::kShadowThrower) &&
				ch->GetSkillWithoutEquip(ESkill::kDarkMagic) > 130;
		case EFeat::kDoubleThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 100;
		case EFeat::kTripleThrower:
			return ch->HaveFeat(EFeat::kDeadlyThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 130;
		case EFeat::kPowerThrow:
			return ch->GetSkillWithoutEquip(ESkill::kThrow) > 90;
		case EFeat::kDeadlyThrow:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 110;
		case EFeat::kSerratedBlade:
			return ch->HaveFeat(EFeat::kCutting);
		default: return true;
	}
}

bool CheckVacantFeatSlot(CharData *ch, EFeat feat_id) {
	if (MUD::Class(ch->GetClass()).feats[feat_id].IsInborn()
		|| PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat_id))) {
		return true;
	}

	//сколько у нас вообще способностей, у которых слот меньше требуемого, и сколько - тех, у которых больше или равно?
	int lowfeat = 0;
	int hifeat = 0;

	//Мы не можем просто учесть кол-во способностей меньше требуемого и больше требуемого,
	//т.к. возможны свободные слоты меньше требуемого, и при этом верхние заняты все
	auto slot_list = std::vector<int>();
	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (feat.IsInborn() || PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat.GetId()))) {
			continue;
		}

		if (ch->HaveFeat(feat.GetId())) {
			if (feat.GetSlot() >= MUD::Class(ch->GetClass()).feats[feat_id].GetSlot()) {
				++hifeat;
			} else {
				slot_list.push_back(feat.GetSlot());
			}
		}
	}

	std::sort(slot_list.begin(), slot_list.end());

	//Посчитаем сколько действительно нижние способности занимают слотов (с учетом пропусков)
	for (const auto &slot: slot_list) {
		if (lowfeat < slot) {
			lowfeat = slot + 1;
		} else {
			++lowfeat;
		}
	}

	//из имеющегося количества слотов нужно вычесть:
	//число высоких слотов, занятых низкоуровневыми способностями,
	//с учетом, что низкоуровневые могут и не занимать слотов выше им положенных,
	//а также собственно число слотов, занятых высокоуровневыми способностями
	if (CalcMaxFeatSlotPerLvl(ch) - MUD::Class(ch->GetClass()).feats[feat_id].GetSlot() -
		hifeat - std::max(0, lowfeat - MUD::Class(ch->GetClass()).feats[feat_id].GetSlot()) > 0) {
		return true;
	}

	//oops.. слотов нет
	return false;
}

// \todo Надо как-то переделать загрузку родовых способностей, чтобы там было не int, а сразу EFeat
void SetRaceFeats(CharData *ch) {
	auto race_features = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int i : race_features) {
		auto feat_id = static_cast<EFeat>(i);
		if (CanGetFeat(ch, feat_id)) {
			ch->SetFeat(feat_id);
		}
	}
}

void UnsetRaceFeats(CharData *ch) {
	auto race_features = PlayerRace::GetRaceFeatures((int) GET_KIN(ch), (int) GET_RACE(ch));
	for (int i : race_features) {
		ch->UnsetFeat(static_cast<EFeat>(i));
	}
}

void SetInbornFeats(CharData *ch) {
	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (feat.IsInborn() && !ch->HaveFeat(feat.GetId())) {
			ch->SetFeat(feat.GetId());
		}
	}
}

void SetInbornAndRaceFeats(CharData *ch) {
	SetInbornFeats(ch);
	SetRaceFeats(ch);
}

void UnsetInaccessibleFeats(CharData *ch) {
	for (const auto &feat : MUD::Feats()) {
		if (ch->HaveFeat(feat.GetId())) {
			if (MUD::Class(ch->GetClass()).feats.IsUnavailable(feat.GetId())) {
				ch->UnsetFeat(feat.GetId());
			}
		}
	}
}

bool TryFlipActivatedFeature(CharData *ch, char *argument) {
	auto feat_id = FindFeatId(argument);
	if (MUD::Feat(feat_id).IsInvalid()) {
		return false;
	}
	if (!CanUseFeat(ch, feat_id)) {
		SendMsgToChar("Вы не в состоянии использовать эту способность.\r\n", ch);
		return true;
	}
	if (PRF_FLAGGED(ch, GetPrfWithFeatNumber(feat_id))) {
		DeactivateFeature(ch, feat_id);
	} else {
		ActivateFeat(ch, feat_id);
	}

	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	return true;
}

void ActivateFeat(CharData *ch, EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformPowerAttack);
			break;
		case EFeat::kGreatPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformGreatPowerAttack);
			break;
		case EFeat::kAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformAimingAttack);
			break;
		case EFeat::kGreatAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			PRF_FLAGS(ch).set(EPrf::kPerformGreatAimingAttack);
			break;
		case EFeat::kSerratedBlade: SendMsgToChar("Вы перехватили свои клинки особым хватом.\r\n", ch);
			act("$n0 ловко прокрутил$g между пальцев свои клинки.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			PRF_FLAGS(ch).set(EPrf::kPerformSerratedBlade);
			break;
		case EFeat::kScirmisher:
			if (!AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar(fmt::format("Голос десятника Никифора вдруг рявкнул: \"{}, тюрюхайло! 'В шеренгу по одному' иначе сполняется!\"\r\n",
							ch->get_name()), ch);
				return;
			}
			if (PRF_FLAGGED(ch, EPrf::kSkirmisher)) {
				SendMsgToChar("Вы уже стоите в передовом строю.\r\n", ch);
				return;
			}
			PRF_FLAGS(ch).set(EPrf::kSkirmisher);
			SendMsgToChar("Вы протиснулись вперед и встали в строй.\r\n", ch);
			act("$n0 протиснул$u вперед и встал$g в строй.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			break;
		case EFeat::kDoubleThrower: PRF_FLAGS(ch).unset(EPrf::kTripleThrow);
			PRF_FLAGS(ch).set(EPrf::kDoubleThrow);
			break;
		case EFeat::kTripleThrower: PRF_FLAGS(ch).unset(EPrf::kDoubleThrow);
			PRF_FLAGS(ch).set(EPrf::kTripleThrow);
			break;
		default:
			SendMsgToChar("Эту способность невозможно применить таким образом.\r\n", ch);
			return;
	}
	SendMsgToChar(fmt::format("{}Вы решили использовать способность '{}'.{}\r\n",
							  KIGRN, MUD::Feat(feat_id).GetName(), KNRM), ch);
}

void DeactivateFeature(CharData *ch, EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformPowerAttack);
			break;
		case EFeat::kGreatPowerAttack: PRF_FLAGS(ch).unset(EPrf::kPerformGreatPowerAttack);
			break;
		case EFeat::kAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformAimingAttack);
			break;
		case EFeat::kGreatAimingAttack: PRF_FLAGS(ch).unset(EPrf::kPerformGreatAimingAttack);
			break;
		case EFeat::kSerratedBlade: SendMsgToChar("Вы ловко прокрутили свои клинки в обычный прямой хват.\r\n", ch);
			act("$n0 ловко прокрутил$g между пальцев свои клинки.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			PRF_FLAGS(ch).unset(EPrf::kPerformSerratedBlade);
			break;
		case EFeat::kScirmisher: PRF_FLAGS(ch).unset(EPrf::kSkirmisher);
			if (AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar("Вы решили, что в обозе вам будет спокойней.\r\n", ch);
				act("$n0 тактически отступил$g в тыл отряда.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			}
			break;
		case EFeat::kDoubleThrower: PRF_FLAGS(ch).unset(EPrf::kDoubleThrow);
			break;
		case EFeat::kTripleThrower: PRF_FLAGS(ch).unset(EPrf::kTripleThrow);
			break;
		default:
			SendMsgToChar("Эту способность невозможно применить таким образом.\r\n", ch);
			return;
	}

	SendMsgToChar(fmt::format("{}Вы прекратили использовать способность '{}'.{}\r\n",
							  KIGRN,  MUD::Feat(feat_id).GetName(), KNRM), ch);
}

EFeat FindWeaponMasterFeat(ESkill skill) {
	switch (skill) {
		case ESkill::kPunch: return EFeat::kPunchMaster;
		case ESkill::kClubs: return EFeat::kClubsMaster;
		case ESkill::kAxes: return EFeat::kAxesMaster;
		case ESkill::kLongBlades: return EFeat::kLongsMaster;
		case ESkill::kShortBlades: return EFeat::kShortsMaster;
		case ESkill::kNonstandart: return EFeat::kNonstandartsMaster;
		case ESkill::kTwohands: return EFeat::kTwohandsMaster;
		case ESkill::kPicks: return EFeat::kPicksMaster;
		case ESkill::kSpades: return EFeat::kSpadesMaster;
		case ESkill::kBows: return EFeat::kBowsMaster;
		default: return EFeat::kUndefined;
	}
}

/*
 TODO: при переписывании способностей переделать на композицию или интерфейс
*/
Bitvector GetPrfWithFeatNumber(EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: return EPrf::kPerformPowerAttack;
		case EFeat::kGreatPowerAttack: return EPrf::kPerformGreatPowerAttack;
		case EFeat::kAimingAttack: return EPrf::kPerformAimingAttack;
		case EFeat::kGreatAimingAttack: return EPrf::kPerformGreatAimingAttack;
		case EFeat::kSerratedBlade: return EPrf::kPerformSerratedBlade;
		case EFeat::kScirmisher: return EPrf::kSkirmisher;
		case EFeat::kDoubleThrower: return EPrf::kDoubleThrow;
		case EFeat::kTripleThrower: return EPrf::kTripleThrow;
		default: break;
	}

	return EPrf::kPerformPowerAttack;
}

int CalcMaxFeatSlotPerLvl(const CharData *ch) {
	return (kMinBaseFeatsSlotsAmount + GetRealLevel(ch)*(kMaxBaseFeatsSlotsAmount - 1 +
		GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot())/kLastFeatSlotLvl);
}

int CalcFeatSlotsAmountPerRemort(CharData *ch) {
	auto result = kMinBaseFeatsSlotsAmount + kMaxPlayerLevel*(kMaxBaseFeatsSlotsAmount - 1 +
		ch->get_remort()/ MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot())/kLastFeatSlotLvl;
	return result;
}

// ==============================================================================================================

namespace feats {

using ItemPtr = FeatInfoBuilder::ItemPtr;

void FeatsLoader::Load(DataNode data) {
	MUD::Feats().Init(data.Children());
}

void FeatsLoader::Reload(DataNode data) {
	MUD::Feats().Reload(data.Children());
}

ItemPtr FeatInfoBuilder::Build(DataNode &node) {
	try {
		return ParseFeat(node);
	} catch (std::exception &e) {
		err_log("Feat parsing error (incorrect value '%s')", e.what());
		return nullptr;
	}
}

ItemPtr FeatInfoBuilder::ParseFeat(DataNode &node) {
	auto info = ParseHeader(node);

	ParseEffects(info, node);
	return info;
}

ItemPtr FeatInfoBuilder::ParseHeader(DataNode &node) {
	auto id{EFeat::kUndefined};
	try {
		id = parse::ReadAsConstant<EFeat>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect feat id (%s).", e.what());
		throw;
	}
	auto mode = FeatInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	auto info = std::make_shared<FeatInfo>(id, mode);
	try {
		info->name_ = parse::ReadAsStr(node.GetValue("name"));
	} catch (std::exception &) {
	}

	return info;
}

void FeatInfoBuilder::ParseEffects(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("talent_effects")) {
		info->effects.Build(node);
		node.GoToParent();
	}
}

void FeatInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print feat:" << std::endl
		   << " Id: " << KGRN << NAME_BY_ITEM<EFeat>(GetId()) << KNRM << std::endl
		   << " Name: " << KGRN << GetName() << KNRM << std::endl
		   << " Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(GetMode()) << KNRM << std::endl;

	effects.Print(ch, buffer);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
