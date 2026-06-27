//#include "feats.h"
//#include "feats_constants.h"

#include <fmt/core.h>

#include "gameplay/mechanics/player_races.h"
#include "engine/ui/color.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/remort.h"

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
	if (remort::GetRealRemort(ch) < MUD::Class(ch->GetClass()).feats[feat_id].GetMinRemort()) {
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
		case EFeat::kDoubleShot: return (GetSkill(ch, ESkill::kBows) > 39);
		case EFeat::kJeweller: return (GetSkill(ch, ESkill::kJewelry) > 59);
		case EFeat::kSkilledTrader: return ((GetRealLevel(ch) + remort::GetRealRemort(ch) / 3) > 19);
		case EFeat::kMagicUser: return (GetRealLevel(ch) < 25);
		case EFeat::kLiveShield: return (GetSkill(ch, ESkill::kRescue) > 124);
		case EFeat::kShadowThrower: return (GetSkill(ch, ESkill::kDarkMagic) > 120);
		case EFeat::kAnimalMaster: return (GetSkill(ch, ESkill::kMindMagic) > 79);
		case EFeat::kScirmisher: return !(AFF_FLAGGED(ch, EAffect::kStopFight) ||
			AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
			ch->GetPosition() < EPosition::kFight);
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

	if (count >= 2 + remort::GetRealRemort(ch) / 6) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchFocus: return GetSkillWithoutEquip(ch, ESkill::kPunch);
		case EFeat::kClubsFocus: return GetSkillWithoutEquip(ch, ESkill::kClubs);
		case EFeat::kAxesFocus: return GetSkillWithoutEquip(ch, ESkill::kAxes);
		case EFeat::kLongsFocus: return GetSkillWithoutEquip(ch, ESkill::kLongBlades);
		case EFeat::kShortsFocus: return GetSkillWithoutEquip(ch, ESkill::kShortBlades);
		case EFeat::kNonstandartsFocus: return GetSkillWithoutEquip(ch, ESkill::kNonstandart);
		case EFeat::kTwohandsFocus: return GetSkillWithoutEquip(ch, ESkill::kTwohands);
		case EFeat::kPicksFocus: return GetSkillWithoutEquip(ch, ESkill::kPicks);
		case EFeat::kSpadesFocus: return GetSkillWithoutEquip(ch, ESkill::kSpades);
		case EFeat::kBowsFocus: return GetSkillWithoutEquip(ch, ESkill::kBows);
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
	if (count >= 1 + remort::GetRealRemort(ch) / 7) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchMaster:
			return ch->HaveFeat(EFeat::kPunchFocus) && GetSkillWithoutEquip(ch, ESkill::kPunch);
		case EFeat::kClubsMaster:
			return ch->HaveFeat(EFeat::kClubsFocus) && GetSkillWithoutEquip(ch, ESkill::kClubs);
		case EFeat::kAxesMaster:
			return ch->HaveFeat(EFeat::kAxesFocus) && GetSkillWithoutEquip(ch, ESkill::kAxes);
		case EFeat::kLongsMaster:
			return ch->HaveFeat(EFeat::kLongsFocus) && GetSkillWithoutEquip(ch, ESkill::kLongBlades);
		case EFeat::kShortsMaster:
			return ch->HaveFeat(EFeat::kShortsFocus) && GetSkillWithoutEquip(ch, ESkill::kShortBlades);
		case EFeat::kNonstandartsMaster:
			return ch->HaveFeat(EFeat::kNonstandartsFocus) && GetSkillWithoutEquip(ch, ESkill::kNonstandart);
		case EFeat::kTwohandsMaster:
			return ch->HaveFeat(EFeat::kTwohandsFocus) && GetSkillWithoutEquip(ch, ESkill::kTwohands);
		case EFeat::kPicksMaster:
			return ch->HaveFeat(EFeat::kPicksFocus) && GetSkillWithoutEquip(ch, ESkill::kPicks);
		case EFeat::kSpadesMaster:
			return ch->HaveFeat(EFeat::kSpadesFocus) && GetSkillWithoutEquip(ch, ESkill::kSpades);
		case EFeat::kBowsMaster:
			return ch->HaveFeat(EFeat::kBowsFocus) && GetSkillWithoutEquip(ch, ESkill::kBows);
		default: return false;
	}
}

bool CanGetFeat(CharData *ch, EFeat feat) {
	if (MUD::Feat(feat).IsInvalid()) {
		return false;
	}

	if ((MUD::Class(ch->GetClass()).feats.IsUnavailable(feat) &&
		!MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat)) ||
		(remort::GetRealRemort(ch) < MUD::Class(ch->GetClass()).feats[feat].GetMinRemort())) {
		return false;
	}

	if (!CheckVacantFeatSlot(ch, feat)) {
		return false;
	}

	switch (feat) {
		case EFeat::kParryArrow:
			return GetSkillWithoutEquip(ch, ESkill::kMultiparry) ||
				GetSkillWithoutEquip(ch, ESkill::kParry);
		case EFeat::kConnoiseur:
			return GetSkillWithoutEquip(ch, ESkill::kIdentify);
		case EFeat::kExorcist:
			return GetSkillWithoutEquip(ch, ESkill::kTurnUndead);
		case EFeat::kHealer:
			return GetSkillWithoutEquip(ch, ESkill::kFirstAid);
		case EFeat::kStealthy:
			return GetSkillWithoutEquip(ch, ESkill::kHide) ||
				GetSkillWithoutEquip(ch, ESkill::kSneak) ||
				GetSkillWithoutEquip(ch, ESkill::kDisguise);
		case EFeat::kTracker:
			return (GetSkillWithoutEquip(ch, ESkill::kTrack) ||
				GetSkillWithoutEquip(ch, ESkill::kSense));
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
			return GetSkillWithoutEquip(ch, ESkill::kSteal) ||
				GetSkillWithoutEquip(ch, ESkill::kPickLock);
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
			return ch->HaveFeat(EFeat::kBowsFocus) && GetSkillWithoutEquip(ch, ESkill::kBows) > 39;
		case EFeat::kJeweller:
			return GetSkillWithoutEquip(ch, ESkill::kJewelry) > 59;
		case EFeat::kCutting:
			return ch->HaveFeat(EFeat::kShortsMaster) ||
				ch->HaveFeat(EFeat::kPicksMaster) ||
				ch->HaveFeat(EFeat::kLongsMaster) ||
				ch->HaveFeat(EFeat::kSpadesMaster);
		case EFeat::kScirmisher:
			return GetSkillWithoutEquip(ch, ESkill::kRescue);
		case EFeat::kTactician:
			return GetSkillWithoutEquip(ch, ESkill::kLeadership) > 99;
		case EFeat::kShadowThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				GetSkillWithoutEquip(ch, ESkill::kDarkMagic) > 120;
		case EFeat::kShadowDagger:
		case EFeat::kShadowSpear: [[fallthrough]];
		case EFeat::kShadowClub:
			return ch->HaveFeat(EFeat::kShadowThrower) &&
				GetSkillWithoutEquip(ch, ESkill::kDarkMagic) > 130;
		case EFeat::kDoubleThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				GetSkillWithoutEquip(ch, ESkill::kThrow) > 100;
		case EFeat::kTripleThrower:
			return ch->HaveFeat(EFeat::kDeadlyThrow) &&
				GetSkillWithoutEquip(ch, ESkill::kThrow) > 130;
		case EFeat::kPowerThrow:
			return GetSkillWithoutEquip(ch, ESkill::kThrow) > 90;
		case EFeat::kDeadlyThrow:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				GetSkillWithoutEquip(ch, ESkill::kThrow) > 110;
		case EFeat::kSerratedBlade:
			return ch->HaveFeat(EFeat::kCutting);
		default: return true;
	}
}

bool CheckVacantFeatSlot(CharData *ch, EFeat feat_id) {
	if (MUD::Class(ch->GetClass()).feats[feat_id].IsInborn()
		|| MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat_id)) {
		return true;
	}

	//сколько у нас вообще способностей, у которых слот меньше требуемого, и сколько - тех, у которых больше или равно?
	int lowfeat = 0;
	int hifeat = 0;

	//Мы не можем просто учесть кол-во способностей меньше требуемого и больше требуемого,
	//т.к. возможны свободные слоты меньше требуемого, и при этом верхние заняты все
	auto slot_list = std::vector<int>();
	for (const auto &feat : MUD::Class(ch->GetClass()).feats) {
		if (feat.IsInborn() || MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat.GetId())) {
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

void SetRaceFeats(CharData *ch) {
	const auto &race_features = MUD::PcRaces()[GET_RACE(ch)].GetFeatures();
	for (const EFeat feat_id : race_features) {
		if (CanGetFeat(ch, feat_id)) {
			ch->SetFeat(feat_id);
		}
	}
}

void UnsetRaceFeats(CharData *ch) {
	const auto &race_features = MUD::PcRaces()[GET_RACE(ch)].GetFeatures();
	for (const EFeat feat_id : race_features) {
		ch->UnsetFeat(feat_id);
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
	if (ch->IsFlagged(GetPrfWithFeatNumber(feat_id))) {
		DeactivateFeature(ch, feat_id);
	} else {
		ActivateFeat(ch, feat_id);
	}

	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 2);
	return true;
}

void ActivateFeat(CharData *ch, EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: ch->UnsetFlag(EPrf::kPerformAimingAttack);
			ch->UnsetFlag(EPrf::kPerformGreatAimingAttack);
			ch->UnsetFlag(EPrf::kPerformGreatPowerAttack);
			ch->SetFlag(EPrf::kPerformPowerAttack);
			break;
		case EFeat::kGreatPowerAttack: ch->UnsetFlag(EPrf::kPerformPowerAttack);
			ch->UnsetFlag(EPrf::kPerformAimingAttack);
			ch->UnsetFlag(EPrf::kPerformGreatAimingAttack);
			ch->SetFlag(EPrf::kPerformGreatPowerAttack);
			break;
		case EFeat::kAimingAttack: ch->UnsetFlag(EPrf::kPerformPowerAttack);
			ch->UnsetFlag(EPrf::kPerformGreatAimingAttack);
			ch->UnsetFlag(EPrf::kPerformGreatPowerAttack);
			ch->SetFlag(EPrf::kPerformAimingAttack);
			break;
		case EFeat::kGreatAimingAttack: ch->UnsetFlag(EPrf::kPerformPowerAttack);
			ch->UnsetFlag(EPrf::kPerformAimingAttack);
			ch->UnsetFlag(EPrf::kPerformGreatPowerAttack);
			ch->SetFlag(EPrf::kPerformGreatAimingAttack);
			break;
		case EFeat::kSerratedBlade: SendMsgToChar("Вы перехватили свои клинки особым хватом.\r\n", ch);
			act("$n0 ловко прокрутил$g между пальцев свои клинки.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetFlag(EPrf::kPerformSerratedBlade);
			break;
		case EFeat::kScirmisher:
			if (!AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar(fmt::format("Голос десятника Никифора вдруг рявкнул: \"{}, тюрюхайло! 'В шеренгу по одному' иначе сполняется!\"\r\n",
							ch->get_name()), ch);
				return;
			}
			if (ch->IsFlagged(EPrf::kSkirmisher)) {
				SendMsgToChar("Вы уже стоите в передовом строю.\r\n", ch);
				return;
			}
			ch->SetFlag(EPrf::kSkirmisher);
			SendMsgToChar("Вы протиснулись вперед и встали в строй.\r\n", ch);
			act("$n0 протиснул$u вперед и встал$g в строй.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			break;
		case EFeat::kDoubleThrower: ch->UnsetFlag(EPrf::kTripleThrow);
			ch->SetFlag(EPrf::kDoubleThrow);
			break;
		case EFeat::kTripleThrower: ch->UnsetFlag(EPrf::kDoubleThrow);
			ch->SetFlag(EPrf::kTripleThrow);
			break;
		default:
			SendMsgToChar("Эту способность невозможно применить таким образом.\r\n", ch);
			return;
	}
	SendMsgToChar(fmt::format("{}Вы решили использовать способность '{}'.{}\r\n",
							  kColorBoldGrn, MUD::Feat(feat_id).GetName(), kColorNrm), ch);
}

void DeactivateFeature(CharData *ch, EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: ch->UnsetFlag(EPrf::kPerformPowerAttack);
			break;
		case EFeat::kGreatPowerAttack: ch->UnsetFlag(EPrf::kPerformGreatPowerAttack);
			break;
		case EFeat::kAimingAttack: ch->UnsetFlag(EPrf::kPerformAimingAttack);
			break;
		case EFeat::kGreatAimingAttack: ch->UnsetFlag(EPrf::kPerformGreatAimingAttack);
			break;
		case EFeat::kSerratedBlade: SendMsgToChar("Вы ловко прокрутили свои клинки в обычный прямой хват.\r\n", ch);
			act("$n0 ловко прокрутил$g между пальцев свои клинки.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->UnsetFlag(EPrf::kPerformSerratedBlade);
			break;
		case EFeat::kScirmisher: ch->UnsetFlag(EPrf::kSkirmisher);
			if (AFF_FLAGGED(ch, EAffect::kGroup)) {
				SendMsgToChar("Вы решили, что в обозе вам будет спокойней.\r\n", ch);
				act("$n0 тактически отступил$g в тыл отряда.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			}
			break;
		case EFeat::kDoubleThrower: ch->UnsetFlag(EPrf::kDoubleThrow);
			break;
		case EFeat::kTripleThrower: ch->UnsetFlag(EPrf::kTripleThrow);
			break;
		default:
			SendMsgToChar("Эту способность невозможно применить таким образом.\r\n", ch);
			return;
	}

	SendMsgToChar(fmt::format("{}Вы прекратили использовать способность '{}'.{}\r\n",
							  kColorBoldGrn, MUD::Feat(feat_id).GetName(), kColorNrm), ch);
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
EPrf GetPrfWithFeatNumber(EFeat feat_id) {
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
		remort::GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot())/kLastFeatSlotLvl);
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
	info->name_ = MUD::FeatMessages().GetName(id);   // issue.thing-names: name from feat_msg.xml

	return info;
}

void FeatInfoBuilder::ParseEffects(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("talent_effects")) {
		info->effects.Build(node);
		node.GoToParent();
	}
}

void FeatInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print feat:" << "\r\n"
		   << " Id: " << kColorGrn << NAME_BY_ITEM<EFeat>(GetId()) << kColorNrm << "\r\n"
		   << " Name: " << kColorGrn << GetName() << kColorNrm << "\r\n"
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n";

	effects.Print(ch, buffer);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
