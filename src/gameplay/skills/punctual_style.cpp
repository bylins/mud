/**
\file punctual_style.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "punctual_style.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "gameplay/magic/spells_constants.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/handler.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/mechanics/equipment.h"

void ImposeHaemorrhage(CharData *ch, int percent);
void PerformPunctualHit(CharData *ch, CharData *victim, HitData &hit_data);

void ProcessPunctualStyle(CharData *ch, CharData *victim, HitData &hit_data) {
	if (hit_data.dam && hit_data.GetFlags()[fight::kCritHit] && hit_data.dam_critic) {
		PerformPunctualHit(ch, victim, hit_data);
		hit_data.SetFlag(fight::kIgnoreBlink);
	}
}

void PerformPunctualHit(CharData *ch, CharData *victim, HitData &hit_data) {
	const char *to_char = nullptr, *to_vict = nullptr;
	Affect<EApply> af[4];
	ObjData *obj;
	int unequip_pos = 0;

	for (auto & i : af) {
		i.type = ESpell::kUndefined;
		i.location = EApply::kNone;
		i.bitvector = 0;
		i.modifier = 0;
		i.battleflag = 0;
		i.duration = CalcDuration(victim, 2, 0, 0, 0, 0);
	}

	switch (number(1, 10)) {
		case 1:
		case 2:
		case 3:
		case 4:        // FEETS
			switch (hit_data.dam_critic) {
				case 1:
				case 2:
				case 3:
					// Nothing
					return;
				case 4:    // Hit genus, victim bashed, speed/2
					victim->battle_affects.set(kEafSlow);
					hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual) / 10), 20);
					if (victim->GetPosition() > EPosition::kSit) {
						victim->SetPosition(EPosition::kSit);
					}
					victim->DropFromHorse();
					SetWaitState(victim, 2 * kBattleRound);
					to_char = "повалило $N3 на землю";
					to_vict = "повредило вам колено, повалив на землю";
					break;
				case 5:    // victim bashed
					if (victim->GetPosition() > EPosition::kSit) {
						victim->SetPosition(EPosition::kSit);
					}
					victim->DropFromHorse();
					SetWaitState(victim, 2 * kBattleRound);
					to_char = "повалило $N3 на землю";
					to_vict = "повредило вам колено, повалив на землю";
					break;
		case 6:    // foot damaged, speed/2
					hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual) / 9), 23);
					to_char = "замедлило движения $N1";
					to_vict = "сломало вам лодыжку";
					victim->battle_affects.set(kEafSlow);
					break;
				case 7:
				case 9:    // armor damaged else foot damaged, speed/4
					if (GET_EQ(victim, EEquipPos::kLegs))
						DamageEquipment(victim, EEquipPos::kLegs, 100, 100);
					else {
						hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 8, 25);
						to_char = "замедлило движения $N1";
						to_vict = "сломало вам ногу";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kNoFlee);
						victim->battle_affects.set(kEafSlow);
					}
					break;
				case 8:    // femor damaged, no speed
					hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 7, 29);
					to_char = "сильно замедлило движения $N1";
					to_vict = "сломало вам бедро";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 20);
					victim->battle_affects.set(kEafSlow);
					break;
				case 10:    // genus damaged, no speed, -2HR
					hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 7, 29);
					to_char = "сильно замедлило движения $N1";
					to_vict = "раздробило вам колено";
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					victim->battle_affects.set(kEafSlow);
					break;
				case 11:    // femor damaged, no speed, no attack
					hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 7, 29);
					to_char = "вывело $N3 из строя";
					to_vict = "раздробило вам бедро";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 20);
					victim->battle_affects.set(kEafSlow);
					break;
				default:    // femor damaged, no speed, no attack
					if (hit_data.dam_critic > 12)
						hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					else
						hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					to_char = "вывело $N3 из строя";
					to_vict = "изуродовало вам ногу";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 50);
					victim->battle_affects.set(kEafSlow);
					break;
			}
			break;
		case 5:        //  ABDOMINAL
			switch (hit_data.dam_critic) {
				case 1:
				case 2:
				case 3:
					// nothing
					return;
				case 4:    // waits 1d6
					SetWaitState(victim, number(2, 6) * kBattleRound);
					to_char = "сбило $N2 дыхание";
					to_vict = "сбило вам дыхание";
					break;

		case 5:    // abdomin damaged, waits 1, speed/2
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 8, 25);
					SetWaitState(victim, 2 * kBattleRound);
					to_char = "ранило $N3 в живот";
					to_vict = "ранило вас в живот";
					victim->battle_affects.set(kEafSlow);
					break;
				case 6:    // armor damaged else dam*3, waits 1d6
					SetWaitState(victim, number(2, 6) * kBattleRound);
					if (GET_EQ(victim, EEquipPos::kWaist))
						DamageEquipment(victim, EEquipPos::kWaist, 100, 100);
					else
						hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 7, 29);
					to_char = "повредило $N2 живот";
					to_vict = "повредило вам живот";
					break;
				case 7:
				case 8:    // abdomin damage, speed/2, HR-2
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					to_char = "ранило $N3 в живот";
					to_vict = "ранило вас в живот";
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					victim->battle_affects.set(kEafSlow);
					break;
				case 9:    // armor damaged, abdomin damaged, speed/2, HR-2
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					DamageEquipment(victim, EEquipPos::kBody, 100, 100);
					to_char = "ранило $N3 в живот";
					to_vict = "ранило вас в живот";
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 20);
					victim->battle_affects.set(kEafSlow);
					break;
				case 10:    // abdomin damaged, no speed, no attack
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 4, 67);
					to_char = "повредило $N2 живот";
					to_vict = "повредило вам живот";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 20);
					victim->battle_affects.set(kEafSlow);
					break;
				case 11:    // abdomin damaged, no speed, no attack
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 3, 67);
					to_char = "разорвало $N2 живот";
					to_vict = "разорвало вам живот";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 40);
					victim->battle_affects.set(kEafSlow);
					break;
				default:    // abdomin damaged, hits = 0
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					to_char = "размозжило $N2 живот";
					to_vict = "размозжило вам живот";
					ImposeHaemorrhage(victim, 60);
					victim->battle_affects.set(kEafSlow);
					break;
			}
			break;
		case 6:
		case 7:        // CHEST
			switch (hit_data.dam_critic) {
				case 1:
				case 2:
				case 3:
					// nothing
					return;
				case 4:    // waits 1d4, bashed
					SetWaitState(victim, number(2, 5) * kBattleRound);
					if (victim->GetPosition() > EPosition::kSit)
						victim->SetPosition(EPosition::kSit);
					victim->DropFromHorse();
					to_char = "повредило $N2 грудь, свалив $S с ног";
					to_vict = "повредило вам грудь, свалив вас с ног";
					break;
				case 5:    // chest damaged, waits 1, speed/2
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					SetWaitState(victim, 2 * kBattleRound);
					to_char = "повредило $N2 туловище";
					to_vict = "повредило вам туловище";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					victim->battle_affects.set(kEafSlow);
					break;
				case 6:    // shield damaged, chest damaged, speed/2
					DamageEquipment(victim, EEquipPos::kShield, 100, 100);
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					to_char = "повредило $N2 туловище";
					to_vict = "повредило вам туловище";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					victim->battle_affects.set(kEafSlow);
					break;
				case 7:    // srmor damaged, chest damaged, speed/2, HR-2
					DamageEquipment(victim, EEquipPos::kBody, 100, 100);
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					to_char = "повредило $N2 туловище";
					to_vict = "повредило вам туловище";
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					victim->battle_affects.set(kEafSlow);
					break;
				case 8:    // chest damaged, no speed, no attack
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					to_char = "вывело $N3 из строя";
					to_vict = "повредило вам туловище";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 20);
					victim->battle_affects.set(kEafSlow);
					break;
				case 9:    // chest damaged, speed/2, HR-2
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 4, 67);
					to_char = "заставило $N3 ослабить натиск";
					to_vict = "сломало вам ребра";
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 20);
					victim->battle_affects.set(kEafSlow);
					break;
				case 10:    // chest damaged, no speed, no attack
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 4, 67);
					to_char = "вывело $N3 из строя";
					to_vict = "сломало вам ребра";
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 40);
					victim->battle_affects.set(kEafSlow);
					break;
				case 11:    // chest crushed, hits 0
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					ImposeHaemorrhage(victim, 50);
					to_char = "вывело $N3 из строя";
					to_vict = "разорвало вам грудь";
					break;
				default:    // chest crushed, killing
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					ImposeHaemorrhage(victim, 60);
					to_char = "вывело $N3 из строя";
					to_vict = "размозжило вам грудь";
					break;
			}
			break;
		case 8:
		case 9:        // HANDS
			switch (hit_data.dam_critic) {
				case 1:
				case 2:
				case 3: return;
				case 4:    // hands damaged, weapon/shield putdown
					to_char = "ослабило натиск $N1";
					to_vict = "ранило вам руку";
					if (GET_EQ(victim, EEquipPos::kBoths))
						unequip_pos = EEquipPos::kBoths;
					else if (GET_EQ(victim, EEquipPos::kWield))
						unequip_pos = EEquipPos::kWield;
					else if (GET_EQ(victim, EEquipPos::kHold))
						unequip_pos = EEquipPos::kHold;
					else if (GET_EQ(victim, EEquipPos::kShield))
						unequip_pos = EEquipPos::kShield;
					break;
				case 5:    // hands damaged, shield damaged/weapon putdown
					to_char = "ослабило натиск $N1";
					to_vict = "ранило вас в руку";
					if (GET_EQ(victim, EEquipPos::kShield))
						DamageEquipment(victim, EEquipPos::kShield, 100, 100);
					else if (GET_EQ(victim, EEquipPos::kBoths))
						unequip_pos = EEquipPos::kBoths;
					else if (GET_EQ(victim, EEquipPos::kWield))
						unequip_pos = EEquipPos::kWield;
					else if (GET_EQ(victim, EEquipPos::kHold))
						unequip_pos = EEquipPos::kHold;
					break;

				case 6:    // hands damaged, HR-2, shield putdown
					to_char = "ослабило натиск $N1";
					to_vict = "сломало вам руку";
					if (GET_EQ(victim, EEquipPos::kShield))
						unequip_pos = EEquipPos::kShield;
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					break;
				case 7:    // armor damaged, hand damaged if no armour
					if (GET_EQ(victim, EEquipPos::kArms))
						DamageEquipment(victim, EEquipPos::kArms, 100, 100);
					else
						DamageEquipment(victim, EEquipPos::kHands, 100, 100);
					if (!GET_EQ(victim, EEquipPos::kArms) && !GET_EQ(victim, EEquipPos::kHands))
						hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 7, 29);
					to_char = "ослабило атаку $N1";
					to_vict = "повредило вам руку";
					break;
				case 8:    // shield damaged, hands damaged, waits 1
					DamageEquipment(victim, EEquipPos::kShield, 100, 100);
					SetWaitState(victim, 2 * kBattleRound);
					hit_data.dam *= std::min((ch->GetSkill(ESkill::kPunctual)) / 7, 29);
					to_char = "придержало $N3";
					to_vict = "повредило вам руку";
					break;
				case 9:    // weapon putdown, hands damaged, waits 1d4
					SetWaitState(victim, number(2, 4) * kBattleRound);
					if (GET_EQ(victim, EEquipPos::kBoths))
						unequip_pos = EEquipPos::kBoths;
					else if (GET_EQ(victim, EEquipPos::kWield))
						unequip_pos = EEquipPos::kWield;
					else if (GET_EQ(victim, EEquipPos::kHold))
						unequip_pos = EEquipPos::kHold;
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					to_char = "придержало $N3";
					to_vict = "повредило вам руку";
					break;
				case 10:    // hand damaged, no attack this
					if (!AFF_FLAGGED(victim, EAffect::kStopRight)) {
						to_char = "ослабило атаку $N1";
						to_vict = "изуродовало вам правую руку";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kStopRight);
						af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else if (!AFF_FLAGGED(victim, EAffect::kStopLeft)) {
						to_char = "ослабило атаку $N1";
						to_vict = "изуродовало вам левую руку";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kStopLeft);
						af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else {
						to_char = "вывело $N3 из строя";
						to_vict = "вывело вас из строя";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kStopFight);
						af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					}
					ImposeHaemorrhage(victim, 20);
					break;
				default:    // no hand attack, no speed, dam*2 if >= 13
					if (!AFF_FLAGGED(victim, EAffect::kStopRight)) {
						to_char = "ослабило натиск $N1";
						to_vict = "изуродовало вам правую руку";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kStopRight);
						af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else if (!AFF_FLAGGED(victim, EAffect::kStopLeft)) {
						to_char = "ослабило натиск $N1";
						to_vict = "изуродовало вам левую руку";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kStopLeft);
						af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					} else {
						to_char = "вывело $N3 из строя";
						to_vict = "вывело вас из строя";
						af[0].type = ESpell::kBattle;
						af[0].bitvector = to_underlying(EAffect::kStopFight);
						af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
						af[0].battleflag = kAfBattledec | kAfPulsedec;
					}
					af[1].type = ESpell::kBattle;
					af[1].bitvector = to_underlying(EAffect::kNoFlee);
					ImposeHaemorrhage(victim, 30);
					if (hit_data.dam_critic >= 13)
						hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 5, 40);
					victim->battle_affects.set(kEafSlow);
					break;
			}
			break;
		default:        // HEAD
			switch (hit_data.dam_critic) {
				case 1:
				case 2:
				case 3:
					// nothing
					return;
				case 4:    // waits 1d6
					SetWaitState(victim, number(2, 6) * kBattleRound);
					to_char = "помутило $N2 сознание";
					to_vict = "помутило ваше сознание";
					break;

				case 5:    // head damaged, cap putdown, waits 1, HR-2 if no cap
					SetWaitState(victim, 2 * kBattleRound);
					if (GET_EQ(victim, EEquipPos::kHead))
						unequip_pos = EEquipPos::kHead;
					else {
						af[0].type = ESpell::kBattle;
						af[0].location = EApply::kHitroll;
						af[0].modifier = -2;
					}
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 4, 67);
					to_char = "повредило $N2 голову";
					to_vict = "повредило вам голову";
					break;
				case 6:    // head damaged
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -2;
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 4, 67);
					to_char = "повредило $N2 голову";
					to_vict = "повредило вам голову";
					break;
				case 7:    // cap damaged, waits 1d6, speed/2, HR-4
					SetWaitState(victim, 2 * kBattleRound);
					DamageEquipment(victim, EEquipPos::kHead, 100, 100);
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kHitroll;
					af[0].modifier = -4;
					af[0].bitvector = to_underlying(EAffect::kNoFlee);
					to_char = "ранило $N3 в голову";
					to_vict = "ранило вас в голову";
					break;
				case 8:    // cap damaged, hits 0
					SetWaitState(victim, 4 * kBattleRound);
					DamageEquipment(victim, EEquipPos::kHead, 100, 100);
					//dam = GET_HIT(victim);
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					to_char = "отбило у $N1 сознание";
					to_vict = "отбило у вас сознание";
					ImposeHaemorrhage(victim, 20);
					break;
				case 9:    // head damaged, no speed, no attack
					af[0].type = ESpell::kBattle;
					af[0].bitvector = to_underlying(EAffect::kStopFight);
					af[0].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[0].battleflag = kAfBattledec | kAfPulsedec;
					ImposeHaemorrhage(victim, 30);
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 3, 67);
					to_char = "повергло $N3 в оцепенение";
					to_vict = "повергло вас в оцепенение";
					break;
				case 10:    // head damaged, -1 INT/WIS/CHA
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kInt;
					af[0].modifier = -1;
					af[0].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[0].battleflag = kAfDeadkeep;
					af[1].type = ESpell::kBattle;
					af[1].location = EApply::kWis;
					af[1].modifier = -1;
					af[1].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[1].battleflag = kAfDeadkeep;
					af[2].type = ESpell::kBattle;
					af[2].location = EApply::kCha;
					af[2].modifier = -1;
					af[2].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[2].battleflag = kAfDeadkeep;
					af[3].type = ESpell::kBattle;
					af[3].bitvector = to_underlying(EAffect::kStopFight);
					af[3].duration = CalcDuration(victim, 8, 0, 0, 0, 0);
					af[3].battleflag = kAfBattledec | kAfPulsedec;
					ImposeHaemorrhage(victim, 50);
					to_char = "сорвало у $N1 крышу";
					to_vict = "сорвало у вас крышу";
					break;
				case 11:    // hits 0, WIS/2, INT/2, CHA/2
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kInt;
					af[0].modifier = -victim->get_int() / 2;
					af[0].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[0].battleflag = kAfDeadkeep;
					af[1].type = ESpell::kBattle;
					af[1].location = EApply::kWis;
					af[1].modifier = -victim->get_wis() / 2;
					af[1].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[1].battleflag = kAfDeadkeep;
					af[2].type = ESpell::kBattle;
					af[2].location = EApply::kCha;
					af[2].modifier = -victim->get_cha() / 2;
					af[2].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[2].battleflag = kAfDeadkeep;
					ImposeHaemorrhage(victim, 60);
					to_char = "сорвало у $N1 крышу";
					to_vict = "сорвало у вас крышу";
					break;
				default:    // killed
					af[0].type = ESpell::kBattle;
					af[0].location = EApply::kInt;
					af[0].modifier = -victim->get_int() / 2;
					af[0].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[0].battleflag = kAfDeadkeep;
					af[1].type = ESpell::kBattle;
					af[1].location = EApply::kWis;
					af[1].modifier = -victim->get_wis() / 2;
					af[1].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[1].battleflag = kAfDeadkeep;
					af[2].type = ESpell::kBattle;
					af[2].location = EApply::kCha;
					af[2].modifier = -victim->get_cha() / 2;
					af[2].duration = CalcDuration(victim, number(1, 6) * 24, 0, 0, 0, 0);
					af[2].battleflag = kAfDeadkeep;
					hit_data.dam *= std::min(ch->GetSkill(ESkill::kPunctual) / 2, 100);
					to_char = "размозжило $N2 голову";
					to_vict = "размозжило вам голову";
					ImposeHaemorrhage(victim, 90);
					break;
			}
			break;
	}
	if (to_char) {
		sprintf(buf, "&G&qВаше точное попадание %s.&Q&n", to_char);
		act(buf, false, ch, nullptr, victim, kToChar);
		sprintf(buf, "Точное попадание $n1 %s.", to_char);
		act(buf, true, ch, nullptr, victim, kToNotVict | kToArenaListen);
	}

	if (to_vict) {
		sprintf(buf, "&R&qМеткое попадание $n1 %s.&Q&n", to_vict);
		act(buf, false, ch, nullptr, victim, kToVict);
	}
	if (unequip_pos && GET_EQ(victim, unequip_pos)) {
		obj = UnequipChar(victim, unequip_pos, CharEquipFlags());
		switch (unequip_pos) {
			case 6:        //WEAR_HEAD
				sprintf(buf, "%s слетел%s с вашей головы.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToVict);
				sprintf(buf, "%s слетел%s с головы $N1.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToChar);
				act(buf, true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				break;

			case 11:    //WEAR_SHIELD
				sprintf(buf, "%s слетел%s с вашей руки.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToVict);
				sprintf(buf, "%s слетел%s с руки $N1.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToChar);
				act(buf, true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				break;

			case 16:    //WEAR_WIELD
			case 17:    //WEAR_HOLD
				sprintf(buf, "%s выпал%s из вашей руки.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToVict);
				sprintf(buf, "%s выпал%s из руки $N1.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToChar);
				act(buf, true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				break;

			case 18:    //WEAR_BOTHS
				sprintf(buf, "%s выпал%s из ваших рук.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToVict);
				sprintf(buf, "%s выпал%s из рук $N1.", obj->get_PName(ECase::kNom).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, false, ch, nullptr, victim, kToChar);
				act(buf, true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				break;
		}
		if (!victim->IsNpc() && ROOM_FLAGGED(victim->in_room, ERoomFlag::kArena))
			PlaceObjToInventory(obj, victim);
		else
			PlaceObjToRoom(obj, victim->in_room);
		CheckObjDecay(obj);
	}
	if (!victim->IsNpc()) {
		hit_data.dam /= 5;
	}
	if (victim->IsFlagged(EMobFlag::kNotKillPunctual)) {
		hit_data.dam /= 1.5;
	}
	hit_data.dam = ApplyResist(victim, EResist::kVitality, hit_data.dam);
	for (auto & i : af) {
		if (i.type > ESpell::kUndefined) {
			ImposeAffect(victim, i, true, false, true, false);
		}
	}
}

void ImposeHaemorrhage(CharData *ch, int percent) {
	Affect<EApply> af[3];

	af[0].type = ESpell::kHaemorrhage;
	af[0].location = EApply::kHpRegen;
	af[0].modifier = -percent;
	//TODO: Отрицательное время, если тело больше 31?
	af[0].duration = CalcDuration(ch, number(1, 31 - GetRealCon(ch)), 0, 0, 0, 0);
	af[0].bitvector = 0;
	af[0].battleflag = 0;
	af[1].type = ESpell::kHaemorrhage;
	af[1].location = EApply::kMoveRegen;
	af[1].modifier = -percent;
	af[1].duration = af[0].duration;
	af[1].bitvector = 0;
	af[1].battleflag = 0;
	af[2].type = ESpell::kHaemorrhage;
	af[2].location = EApply::kManaRegen;
	af[2].modifier = -percent;
	af[2].duration = af[0].duration;
	af[2].bitvector = 0;
	af[2].battleflag = 0;

	for (auto &i : af) {
		ImposeAffect(ch, i, true, false, true, false);
	}
}

int CalcPunctualCritDmg(CharData *ch, CharData * /*victim*/, ObjData *wielded) {
	int dam_critic = 0, wapp = 0;

	if (wielded) {
		wapp = (int) ((static_cast<ESkill>(wielded->get_spec_param()) == ESkill::kBows) &&
			GET_EQ(ch, EEquipPos::kBoths)) ? wielded->get_weight() * 1 / 3 : wielded->get_weight();
	}
	if (wapp < 10)
		dam_critic = RollDices(1, 6);
	else if (wapp < 19)
		dam_critic = RollDices(2, 5);
	else if (wapp < 27)
		dam_critic = RollDices(3, 4);
	else if (wapp < 36)
		dam_critic = RollDices(3, 5);
	else if (wapp < 44)
		dam_critic = RollDices(3, 6);
	else
		dam_critic = RollDices(4, 5);

	const int skill = 1 + ch->GetSkill(ESkill::kPunctual) / 6;
	dam_critic = std::min(number(4, skill), dam_critic);

	return dam_critic;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
