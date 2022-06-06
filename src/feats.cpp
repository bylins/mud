//#include "feats.h"
//#include "feats_constants.h"

#include <boost/algorithm/string.hpp>

//#include "game_abilities/abilities_constants.h"
#include "action_targeting.h"
#include "handler.h"
#include "entities/player_races.h"
#include "color.h"
#include "game_fight/pk.h"
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
	if (GET_REAL_REMORT(ch) < MUD::Class(ch->GetClass()).feats[feat_id].GetMinRemort()) {
		return false;
	};

	switch (feat_id) {
		case EFeat::kWeaponFinesse:
		case EFeat::kFInesseShot: return (GET_REAL_DEX(ch) > GET_REAL_STR(ch) && GET_REAL_DEX(ch) > 17);
			break;
		case EFeat::kParryArrow: return (GET_REAL_DEX(ch) > 15);
			break;
		case EFeat::kPowerAttack: return (GET_REAL_STR(ch) > 19);
			break;
		case EFeat::kGreatPowerAttack: return (GET_REAL_STR(ch) > 21);
			break;
		case EFeat::kAimingAttack: return (GET_REAL_DEX(ch) > 15);
			break;
		case EFeat::kGreatAimingAttack: return (GET_REAL_DEX(ch) > 17);
			break;
		case EFeat::kDoubleShot: return (ch->GetSkill(ESkill::kBows) > 39);
			break;
		case EFeat::kJeweller: return (ch->GetSkill(ESkill::kJewelry) > 59);
			break;
		case EFeat::kSkilledTrader: return ((GetRealLevel(ch) + GET_REAL_REMORT(ch) / 3) > 19);
			break;
		case EFeat::kMagicUser: return (GetRealLevel(ch) < 25);
			break;
		case EFeat::kLiveShield: return (ch->GetSkill(ESkill::kRescue) > 124);
			break;
		case EFeat::kShadowThrower: return (ch->GetSkill(ESkill::kDarkMagic) > 120);
			break;
		case EFeat::kAnimalMaster: return (ch->GetSkill(ESkill::kMindMagic) > 79);
			break;
		case EFeat::kScirmisher:
			return !(AFF_FLAGGED(ch, EAffect::kStopFight)
				|| AFF_FLAGGED(ch, EAffect::kMagicStopFight)
				|| GET_POS(ch) < EPosition::kFight);
			break;
		default: return true;
			break;
	}
	return true;
}

bool CanGetFocusFeat(const CharData *ch, const EFeat feat_id) {
	int count{0};
	for (auto focus_feat_id = EFeat::kPunchFocus; focus_feat_id <= EFeat::kBowsFocus; ++focus_feat_id) {
		if (ch->HaveFeat(focus_feat_id)) {
			count++;
		}
	}

	if (count >= 2 + GET_REAL_REMORT(ch) / 6) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchFocus:
			return ch->GetSkillWithoutEquip(ESkill::kPunch);
			break;
		case EFeat::kClubsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kClubs);
			break;
		case EFeat::kAxesFocus:
			return ch->GetSkillWithoutEquip(ESkill::kAxes);
			break;
		case EFeat::kLongsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kLongBlades);
			break;
		case EFeat::kShortsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kShortBlades);
			break;
		case EFeat::kNonstandartsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kNonstandart);
			break;
		case EFeat::kTwohandsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kTwohands);
			break;
		case EFeat::kPicksFocus:
			return ch->GetSkillWithoutEquip(ESkill::kPicks);
			break;
		case EFeat::kSpadesFocus:
			return ch->GetSkillWithoutEquip(ESkill::kSpades);
			break;
		case EFeat::kBowsFocus:
			return ch->GetSkillWithoutEquip(ESkill::kBows);
			break;
		default:
			return false;
			break;
	}
}

bool CanGetMasterFeat(const CharData *ch, const EFeat feat_id) {
	int count{0};
	for (auto master_feat = EFeat::kPunchMaster; master_feat <= EFeat::kBowsMaster; ++master_feat) {
		if (ch->HaveFeat(master_feat)) {
			count++;
		}
	}
	if (count >= 1 + GET_REAL_REMORT(ch) / 7) {
		return false;
	}

	switch (feat_id) {
		case EFeat::kPunchMaster:
			return ch->HaveFeat(EFeat::kPunchFocus) && ch->GetSkillWithoutEquip(ESkill::kPunch);
			break;
		case EFeat::kClubsMaster:
			return ch->HaveFeat(EFeat::kClubsFocus) && ch->GetSkillWithoutEquip(ESkill::kClubs);
			break;
		case EFeat::kAxesMaster:
			return ch->HaveFeat(EFeat::kAxesFocus) && ch->GetSkillWithoutEquip(ESkill::kAxes);
			break;
		case EFeat::kLongsMaster:
			return ch->HaveFeat(EFeat::kLongsFocus) && ch->GetSkillWithoutEquip(ESkill::kLongBlades);
			break;
		case EFeat::kShortsMaster:
			return ch->HaveFeat(EFeat::kShortsFocus) && ch->GetSkillWithoutEquip(ESkill::kShortBlades);
			break;
		case EFeat::kNonstandartsMaster:
			return ch->HaveFeat(EFeat::kNonstandartsFocus) && ch->GetSkillWithoutEquip(ESkill::kNonstandart);
			break;
		case EFeat::kTwohandsMaster:
			return ch->HaveFeat(EFeat::kTwohandsFocus) && ch->GetSkillWithoutEquip(ESkill::kTwohands);
			break;
		case EFeat::kPicksMaster:
			return ch->HaveFeat(EFeat::kPicksFocus) && ch->GetSkillWithoutEquip(ESkill::kPicks);
			break;
		case EFeat::kSpadesMaster:
			return ch->HaveFeat(EFeat::kSpadesFocus) && ch->GetSkillWithoutEquip(ESkill::kSpades);
			break;
		case EFeat::kBowsMaster:
			return ch->HaveFeat(EFeat::kBowsFocus) && ch->GetSkillWithoutEquip(ESkill::kBows);
			break;
		default:
			return false;
			break;
	}

	return false;
}

bool CanGetFeat(CharData *ch, EFeat feat) {
	if (MUD::Feat(feat).IsInvalid()) {
		return false;
	}

	if ((MUD::Class(ch->GetClass()).feats.IsUnavailable(feat) &&
		!PlayerRace::FeatureCheck(GET_KIN(ch), GET_RACE(ch), to_underlying(feat))) ||
		(GET_REAL_REMORT(ch) < MUD::Class(ch->GetClass()).feats[feat].GetMinRemort())) {
		return false;
	}

	if (!CheckVacantFeatSlot(ch, feat)) {
		return false;
	}

	switch (feat) {
		case EFeat::kParryArrow:
			return ch->GetSkillWithoutEquip(ESkill::kMultiparry) ||
				ch->GetSkillWithoutEquip(ESkill::kParry);
			break;
		case EFeat::kConnoiseur:
			return ch->GetSkillWithoutEquip(ESkill::kIdentify);
			break;
		case EFeat::kExorcist:
			return ch->GetSkillWithoutEquip(ESkill::kTurnUndead);
			break;
		case EFeat::kHealer:
			return ch->GetSkillWithoutEquip(ESkill::kFirstAid);
			break;
		case EFeat::kStealthy:
			return ch->GetSkillWithoutEquip(ESkill::kHide) ||
				ch->GetSkillWithoutEquip(ESkill::kSneak) ||
				ch->GetSkillWithoutEquip(ESkill::kDisguise);
			break;
		case EFeat::kTracker:
			return (ch->GetSkillWithoutEquip(ESkill::kTrack) ||
				ch->GetSkillWithoutEquip(ESkill::kSense));
			break;
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
			break;
		case EFeat::kWarriorSpirit:
			return ch->HaveFeat(EFeat::kGreatFortitude);
			break;
		case EFeat::kNimbleFingers:
			return ch->GetSkillWithoutEquip(ESkill::kSteal) ||
				ch->GetSkillWithoutEquip(ESkill::kPickLock);
			break;
		case EFeat::kGreatPowerAttack:
			return ch->HaveFeat(EFeat::kPowerAttack);
			break;
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
			break;
		case EFeat::kGreatAimingAttack:
			return ch->HaveFeat(EFeat::kAimingAttack);
			break;
		case EFeat::kDoubleShot:
			return ch->HaveFeat(EFeat::kBowsFocus) && ch->GetSkillWithoutEquip(ESkill::kBows) > 39;
			break;
		case EFeat::kJeweller:
			return ch->GetSkillWithoutEquip(ESkill::kJewelry) > 59;
			break;
		case EFeat::kCutting:
			return ch->HaveFeat(EFeat::kShortsMaster) ||
				ch->HaveFeat(EFeat::kPicksMaster) ||
				ch->HaveFeat(EFeat::kLongsMaster) ||
				ch->HaveFeat(EFeat::kSpadesMaster);
			break;
		case EFeat::kScirmisher:
			return ch->GetSkillWithoutEquip(ESkill::kRescue);
			break;
		case EFeat::kTactician:
			return ch->GetSkillWithoutEquip(ESkill::kLeadership) > 99;
			break;
		case EFeat::kShadowThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kDarkMagic) > 120;
			break;
		case EFeat::kShadowDagger:
		case EFeat::kShadowSpear: [[fallthrough]];
		case EFeat::kShadowClub:
			return ch->HaveFeat(EFeat::kShadowThrower) &&
				ch->GetSkillWithoutEquip(ESkill::kDarkMagic) > 130;
			break;
		case EFeat::kDoubleThrower:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 100;
			break;
		case EFeat::kTripleThrower:
			return ch->HaveFeat(EFeat::kDeadlyThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 130;
			break;
		case EFeat::kPowerThrow:
			return ch->GetSkillWithoutEquip(ESkill::kThrow) > 90;
			break;
		case EFeat::kDeadlyThrow:
			return ch->HaveFeat(EFeat::kPowerThrow) &&
				ch->GetSkillWithoutEquip(ESkill::kThrow) > 110;
			break;
		case EFeat::kSerratedBlade:
			return ch->HaveFeat(EFeat::kCutting);
			break;
		default: return true;
			break;
	}

	return true;
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
		if (feat.IsInborn() && !ch->HaveFeat(feat.GetId()) && CanGetFeat(ch, feat.GetId())) {
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
				SendMsgToChar(ch,
							  "Голос десятника Никифора вдруг рявкнул: \"%s, тюрюхайло! 'В шеренгу по одному' иначе сполняется!\"\r\n",
							  ch->get_name().c_str());
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
			break;
	}
	SendMsgToChar(ch,
				  "%sВы решили использовать способность '%s'.%s\r\n",
				  CCIGRN(ch, C_SPR),
				  MUD::Feat(feat_id).GetCName(),
				  CCNRM(ch, C_OFF));
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
			break;
	}
	SendMsgToChar(ch, "%sВы прекратили использовать способность '%s'.%s\r\n",
				  CCIGRN(ch, C_SPR), MUD::Feat(feat_id).GetCName(), CCNRM(ch, C_OFF));
}

EFeat FindWeaponMasterFeat(ESkill skill) {
	switch (skill) {
		case ESkill::kPunch: return EFeat::kPunchMaster;
			break;
		case ESkill::kClubs: return EFeat::kClubsMaster;
			break;
		case ESkill::kAxes: return EFeat::kAxesMaster;
			break;
		case ESkill::kLongBlades: return EFeat::kLongsMaster;
			break;
		case ESkill::kShortBlades: return EFeat::kShortsMaster;
			break;
		case ESkill::kNonstandart: return EFeat::kNonstandartsMaster;
			break;
		case ESkill::kTwohands: return EFeat::kTwohandsMaster;
			break;
		case ESkill::kPicks: return EFeat::kPicksMaster;
			break;
		case ESkill::kSpades: return EFeat::kSpadesMaster;
			break;
		case ESkill::kBows: return EFeat::kBowsMaster;
			break;
		default: return EFeat::kUndefined;
	}
}

/*
 TODO: при переписывании способностей переделать на композицию или интерфейс
*/
Bitvector GetPrfWithFeatNumber(EFeat feat_id) {
	switch (feat_id) {
		case EFeat::kPowerAttack: return EPrf::kPerformPowerAttack;
			break;
		case EFeat::kGreatPowerAttack: return EPrf::kPerformGreatPowerAttack;
			break;
		case EFeat::kAimingAttack: return EPrf::kPerformAimingAttack;
			break;
		case EFeat::kGreatAimingAttack: return EPrf::kPerformGreatAimingAttack;
			break;
		case EFeat::kSerratedBlade: return EPrf::kPerformSerratedBlade;
			break;
		case EFeat::kScirmisher: return EPrf::kSkirmisher;
			break;
		case EFeat::kDoubleThrower: return EPrf::kDoubleThrow;
			break;
		case EFeat::kTripleThrower: return EPrf::kTripleThrow;
			break;
		default: break;
	}

	return EPrf::kPerformPowerAttack;
}

int CalcMaxFeatSlotPerLvl(const CharData *ch) {
	return (kMinBaseFeatsSlotsAmount + GetRealLevel(ch)*(kMaxBaseFeatsSlotsAmount - 1 +
		GET_REAL_REMORT(ch)/ MUD::Class(ch->GetClass()).GetRemortsNumForFeatSlot())/kLastFeatSlotLvl);
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
	if (node.GoToChild("effects")) {
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
