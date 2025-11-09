/**
\file armor.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "armor.h"

#include "engine/entities/char_data.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/skills/leadership.h"

int GetClaccAcLimit(CharData *ch);

int CalcBaseAc(CharData *ch) {
	int armorclass = GetRealAc(ch);

	if (AWAKE(ch)) {
		int high_stat = GetRealDex(ch);
		// у игроков для ац берется макс стат: ловкость или 3/4 инты
		if (!ch->IsNpc()) {
			high_stat = std::max(high_stat, GetRealInt(ch) * 3 / 4);
		}
		armorclass -= dex_ac_bonus(high_stat) * 10;
		armorclass += GetExtraAc0(ch->GetClass(), GetRealLevel(ch));
	};

	if (AFF_FLAGGED(ch, EAffect::kBerserk)) {
		armorclass -= (240 * ((ch->get_real_max_hit() / 2) - ch->get_hit()) / ch->get_real_max_hit());
	}

	if (ch->IsFlagged(EPrf::kIronWind)) {
		armorclass += ch->GetSkill(ESkill::kIronwind) / 2;
	}

	armorclass += (size_app[GET_POS_SIZE(ch)].ac * 10);

	if (ch->battle_affects.get(kEafPunctual)) {
		if (GET_EQ(ch, EEquipPos::kWield)) {
			if (GET_EQ(ch, EEquipPos::kHold))
				armorclass +=
					10 * std::max(-1,
								  (GET_EQ(ch, EEquipPos::kWield)->get_weight() +
									  GET_EQ(ch, EEquipPos::kHold)->get_weight()) / 5 - 6);
			else
				armorclass += 10 * std::max(-1, GET_EQ(ch, EEquipPos::kWield)->get_weight() / 5 - 6);
		}
		if (GET_EQ(ch, EEquipPos::kBoths))
			armorclass += 10 * std::max(-1, GET_EQ(ch, EEquipPos::kBoths)->get_weight() / 5 - 6);
	}

	if (CalcLeadership(ch)) {
		armorclass -= 20;
	}

	armorclass = std::min(100, armorclass);
	return (std::max(GetClaccAcLimit(ch), armorclass));
}

int GetRealAc(CharData *ch) {
	return (GET_AC(ch)+GET_AC_ADD(ch));
}

int GetClaccAcLimit(CharData *ch) {
	if (IS_CHARMICE(ch)) {
		return -200;
	};
	if (ch->IsNpc()) {
		return -300;
	};
	switch (ch->GetClass()) {
		case ECharClass::kPaladine: return -270;
		case ECharClass::kAssasine:
		case ECharClass::kThief: [[fallthrough]];
		case ECharClass::kGuard: return -250;
		case ECharClass::kMerchant:
		case ECharClass::kWarrior:
		case ECharClass::kRanger: [[fallthrough]];
		case ECharClass::kVigilant: return -200;
		case ECharClass::kSorcerer:
		case ECharClass::kMagus: return -170;
		case ECharClass::kConjurer:
		case ECharClass::kWizard:
		case ECharClass::kCharmer: [[fallthrough]];
		case ECharClass::kNecromancer: return -150;
		default: return -300;
	}
}

int CalcTotalAc(CharData *victim, int base_ac) {
	// Calculate the raw armor including magic armor.  Lower AC is better.
	base_ac += CalcBaseAc(victim);
	base_ac /= 10;
	//считаем штраф за голод
	if (!victim->IsNpc() && base_ac < 5) { //для голодных
		int p_ac = static_cast<int>((1 - victim->get_cond_penalty(P_AC)) * 40);
		if (p_ac) {
			if (base_ac + p_ac > 5) {
				base_ac = 5;
			} else {
				base_ac += p_ac;
			}
		}
	}

	if (victim->GetPosition() < EPosition::kFight)
		base_ac += 4;
	if (victim->GetPosition() < EPosition::kRest)
		base_ac += 3;
	if (AFF_FLAGGED(victim, EAffect::kHold))
		base_ac += 4;
	if (AFF_FLAGGED(victim, EAffect::kCrying))
		base_ac += 4;

	return base_ac;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
