//#include "feats.h"
//#include "feats_constants.h"

#include <fmt/core.h>

#include "gameplay/mechanics/player_races.h"
#include "engine/ui/color.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/remort.h"
#include "feats.h"                              // issue.perk-action-patching
#include "gameplay/magic/spells_info.h"         // issue.perk-action-patching: SpellInfo::talent_patches
#include <unordered_map>
#include <cstring>

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

// issue.perk-action-patching: map an effect-kind name ("kArea"/"kDamage"/...) to EAction, for the
// has_effect= selector and op="modify" effect=. Returns false on an unknown name.
static bool ParseEActionKind(const char *name, talents_actions::EAction &out) {
	using talents_actions::EAction;
	static const std::unordered_map<std::string, EAction> kByName = {
		{"kDamage", EAction::kDamage}, {"kArea", EAction::kArea}, {"kPoints", EAction::kPoints},
		{"kAffect", EAction::kAffect}, {"kUnaffect", EAction::kUnaffect}, {"kSummon", EAction::kSummon},
		{"kCreation", EAction::kCreation}, {"kAlterObj", EAction::kAlterObj},
	};
	const auto it = kByName.find(name);
	if (it == kByName.end()) { return false; }
	out = it->second;
	return true;
}

// issue.perk-action-patching: parse a feat talent_patches block into FeatInfo::talent_patches. Each
// <talent_patch> carries the target spell + op + optional action/effect/anchor ids; its <action> child
// block(s) are the perk-provided payload, parsed by the normal action parser (Actions::Build).
static void ParseTalentPatches(FeatInfo &info, DataNode &node) {
	if (!node.GoToChild("talent_patches")) {
		return;
	}
	for (auto &pn : node.Children()) {
		try {
			TalentPatch sp;
			if (const char *sv = pn.GetValue("spell"); sv && *sv) {
				sp.target_spell = parse::ReadAsConstant<ESpell>(sv);
			}
			if (const char *ev = pn.GetValue("element"); ev && *ev) {
				sp.element = parse::ReadAsConstant<EElement>(ev);
			}
			if (const char *bsk = pn.GetValue("base_skill"); bsk && *bsk) {
				sp.base_skill = parse::ReadAsConstant<ESkill>(bsk);
			}
			if (const char *fv = pn.GetValue("flag"); fv && *fv) {
				sp.flag_sel = static_cast<Bitvector>(parse::ReadAsConstant<EMagic>(fv));
			}
			if (const char *al = pn.GetValue("all"); al && *al
					&& (strcmp(al, "true") == 0 || strcmp(al, "Y") == 0 || strcmp(al, "1") == 0)) {
				sp.match_all = true;
			}
			if (const char *he = pn.GetValue("has_effect"); he && *he) {
				if (ParseEActionKind(he, sp.has_effect_kind)) { sp.has_has_effect = true; }
				else { err_log("talent patch: unknown has_effect [%s].", he); }
			}
			if (const char *cv = pn.GetValue("category"); cv && *cv) { sp.category = cv; }
			if (const char *afv = pn.GetValue("affect"); afv && *afv) {
				try { sp.target_affect = ITEM_BY_NAME<EAffect>(afv); }
				catch (const std::exception &) { err_log("talent patch: unknown affect [%s].", afv); }
			}
			if (const char *rv = pn.GetValue("relative"); rv && *rv) {
				if (strcmp(rv, "master") == 0) { sp.relative = TalentPatch::ERelative::kMaster; }
				else if (strcmp(rv, "group_leader") == 0) { sp.relative = TalentPatch::ERelative::kGroupLeader; }
				else if (strcmp(rv, "self") == 0) { sp.relative = TalentPatch::ERelative::kSelf; }
				else { err_log("talent patch: unknown relative [%s] (self/master/group_leader).", rv); }
			}
			const std::string op = (pn.GetValue("op") && *pn.GetValue("op")) ? pn.GetValue("op") : "append";
			if      (op == "append")      { sp.op = EPatchOp::kAppend; }
			else if (op == "insert")      { sp.op = EPatchOp::kInsert; }
			else if (op == "replace")     { sp.op = EPatchOp::kReplace; }
			else if (op == "remove")      { sp.op = EPatchOp::kRemove; }
			else if (op == "add_effect")  { sp.op = EPatchOp::kAddEffect; }
			else if (op == "replace_all") { sp.op = EPatchOp::kReplaceAll; }
			else if (op == "modify")      { sp.op = EPatchOp::kModify; }
			else { err_log("talent patch: unknown op [%s].", op.c_str()); continue; }
			if (const char *bv = pn.GetValue("by"); bv && *bv && strcmp(bv, "target") == 0) {
				sp.scope = TalentPatch::EScope::kTarget;
			}
			if (const char *av = pn.GetValue("action"); av && *av) { sp.action_id = av; }
			if (const char *ev = pn.GetValue("effect"); ev && *ev) { sp.effect_id = ev; }
			if (const char *bef = pn.GetValue("before"); bef && *bef) {
				sp.anchor_id = bef; sp.anchor_before = true;
			} else if (const char *aft = pn.GetValue("after"); aft && *aft) {
				sp.anchor_id = aft; sp.anchor_before = false;
			}
			if (!sp.HasSelector()) {
				err_log("talent patch on feat [%s]: no selector (need spell/element/base_skill/flag/has_effect/all).",
						NAME_BY_ITEM<EFeat>(info.GetId()).c_str());
				continue;
			}
			if (sp.op == EPatchOp::kModify) {
				if (!sp.effect_id.empty() && !ParseEActionKind(sp.effect_id.c_str(), sp.mod_kind)) {
					err_log("talent patch: op=modify effect [%s] is not a manifestation kind.", sp.effect_id.c_str());
				}
				if (pn.GoToChild("modify")) {
					if (const char *fld = pn.GetValue("field"); fld && *fld) { sp.mod_field = fld; }
					if (const char *mv = pn.GetValue("mul"); mv && *mv) {
						sp.mod_op = TalentPatch::EModOp::kMul; sp.mod_value = parse::ReadAsDouble(mv);
					} else if (const char *ad = pn.GetValue("add"); ad && *ad) {
						sp.mod_op = TalentPatch::EModOp::kAdd; sp.mod_value = parse::ReadAsDouble(ad);
					} else if (const char *st = pn.GetValue("set"); st && *st) {
						sp.mod_op = TalentPatch::EModOp::kSet; sp.mod_value = parse::ReadAsDouble(st);
					}
					pn.GoToParent();
				} else {
					err_log("talent patch: op=modify without a <modify field=...> child.");
				}
			} else {
				sp.payload.Build(pn);
			}
			info.talent_patches.push_back(std::move(sp));
		} catch (std::exception &e) {
			err_log("Feat talent_patch parse error (%s).", e.what());
		}
	}
	node.GoToParent();
}

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
	ParseTalentPatches(*info, node);   // issue.perk-action-patching
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

// issue.perk-action-patching: validate a patch target/anchor ids against the spell action blocks.
static bool ValidatePatch(const spells::SpellInfo &spell, const TalentPatch &p, EFeat fid) {
	auto has_block = [&](const std::string &id) {
		if (id.empty()) { return false; }
		for (const auto &b : spell.actions.list()) {
			if (b.GetId() == id) { return true; }
		}
		return false;
	};
	switch (p.op) {
		case EPatchOp::kReplace:
		case EPatchOp::kRemove:
		case EPatchOp::kAddEffect:
			if (!has_block(p.action_id)) {
				err_log("talent patch (feat [%s] -> spell [%s]): action id [%s] not found.",
						NAME_BY_ITEM<EFeat>(fid).c_str(), spell.GetEngCName(), p.action_id.c_str());
				return false;
			}
			break;
		case EPatchOp::kInsert:
			if (!p.anchor_id.empty() && !has_block(p.anchor_id)) {
				err_log("talent patch (feat [%s] -> spell [%s]): insert anchor [%s] not found.",
						NAME_BY_ITEM<EFeat>(fid).c_str(), spell.GetEngCName(), p.anchor_id.c_str());
				return false;
			}
			break;
		case EPatchOp::kModify: {
			bool ok = false;
			for (const auto &b : spell.actions.list()) {
				if (b.Contains(p.mod_kind)) { ok = true; break; }
			}
			if (!ok) {
				err_log("talent patch (feat [%s] -> spell [%s]): op=modify target manifestation absent.",
						NAME_BY_ITEM<EFeat>(fid).c_str(), spell.GetEngCName());
				return false;
			}
			break;
		}
		default:
			break;
	}
	return true;
}

// issue.perk-action-patching (Phase 1): does a patch's selector match this spell? All present predicates
// are AND-combined; an absent one is vacuously true. Evaluated once per (patch, spell) at boot only.
static bool PatchMatchesSpell(const TalentPatch &p, const spells::SpellInfo &s) {
	if (p.target_spell != ESpell::kUndefined && s.GetId() != p.target_spell) { return false; }
	if (p.element != EElement::kUndefined && s.GetElement() != p.element) { return false; }
	if (p.base_skill != ESkill::kUndefined
			&& s.GetPotencyRoll().GetBaseSkill() != p.base_skill
			&& s.GetSuccessRoll().GetBaseSkill() != p.base_skill) { return false; }
	if (p.flag_sel != 0 && !s.IsFlagged(p.flag_sel)) { return false; }
	if (!p.category.empty() && !s.HasTag(p.category)) { return false; }
	if (p.has_has_effect) {
		bool found = false;
		for (const auto &b : s.actions.list()) {
			if (b.Contains(p.has_effect_kind)) { found = true; break; }
		}
		if (!found) { return false; }
	}
	return true;
}

// issue.soullink-affect-patching: affect-targeting patches, bucketed by affect type (underlying int key
// to avoid needing std::hash<EAffect>). Filled at boot by BuildTalentPatchIndex, read by the affect runner.
static std::unordered_map<int, std::vector<AffectPatchRef>> g_affect_patches;

const std::vector<AffectPatchRef> &AffectTalentPatches(EAffect affect_type) {
	static const std::vector<AffectPatchRef> kEmpty;
	const auto it = g_affect_patches.find(to_underlying(affect_type));
	return it != g_affect_patches.end() ? it->second : kEmpty;
}

CharData *ResolvePatchRelative(CharData *owner, TalentPatch::ERelative relative) {
	if (!owner) { return nullptr; }
	switch (relative) {
		case TalentPatch::ERelative::kSelf: return owner;
		case TalentPatch::ERelative::kMaster: return owner->has_master() ? owner->get_master() : nullptr;
		case TalentPatch::ERelative::kGroupLeader: {
			CharData *l = owner;
			while (l->has_master()) { l = l->get_master(); }
			return l;
		}
	}
	return owner;
}

void BuildTalentPatchIndex() {
	g_affect_patches.clear();
	for (auto sid = ESpell::kFirst; sid <= ESpell::kLast; ++sid) {
		if (MUD::Spells().IsKnown(sid)) {
			MUD::Spell(sid).talent_patches.clear();
		}
	}
	size_t linked = 0, dropped = 0;
	for (const auto &fi : MUD::Feats()) {
		const EFeat fid = fi.GetId();
		for (const auto &patch : fi.talent_patches) {
			if (!patch.HasSelector()) {
				err_log("talent patch on feat [%s]: no selector; skipped.", NAME_BY_ITEM<EFeat>(fid).c_str());
				++dropped;
				continue;
			}
			if (patch.target_affect != EAffect::kUndefined) {
				// issue.soullink-affect-patching: an affect patch -- bucket by affect type, not onto a spell.
				g_affect_patches[to_underlying(patch.target_affect)].push_back(AffectPatchRef{fid, &patch});
				++linked;
				continue;
			}
			size_t matched = 0, applied = 0, skipped = 0;
			auto consider = [&](const spells::SpellInfo &spell) {
				if (!PatchMatchesSpell(patch, spell)) { return; }
				++matched;
				if (!ValidatePatch(spell, patch, fid)) { ++skipped; return; }
				spell.talent_patches.push_back(spells::TalentPatchRef{fid, &patch});
				++applied;
			};
			// Specific-spell selector -> O(1); a class selector -> one linear scan of all spells (boot only).
			if (patch.target_spell != ESpell::kUndefined) {
				if (MUD::Spells().IsKnown(patch.target_spell)) {
					consider(MUD::Spell(patch.target_spell));
				} else {
					err_log("talent patch on feat [%s]: unknown target spell.", NAME_BY_ITEM<EFeat>(fid).c_str());
				}
			} else {
				for (const auto &spell : MUD::Spells()) { consider(spell); }
			}
			linked += applied;
			if (matched == 0) {
				err_log("talent patch on feat [%s]: matched no spell.", NAME_BY_ITEM<EFeat>(fid).c_str());
				++dropped;
			} else if (matched > 1 || skipped > 0) {
				log("issue.perk-action-patching: feat [%s] class patch matched %zu, applied %zu, skipped %zu.",
					NAME_BY_ITEM<EFeat>(fid).c_str(), matched, applied, skipped);
			}
		}
	}
	log("issue.perk-action-patching: linked %zu talent patch(es), dropped %zu.", linked, dropped);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
