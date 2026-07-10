/**
\file minions.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 30.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "minions.h"

#include "engine/core/comm.h"
#include "gameplay/core/constants.h"
#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"
#include "engine/ui/cmd/do_hire.h"
#include "utils/logger.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/core/remort.h"

float get_effective_cha(CharData *ch) {
	int key_value, key_value_add;

	key_value = ch->get_cha();
	auto max_cha = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kCha);
	key_value_add = std::min(max_cha - ch->get_cha(), GET_CHA_ADD(ch)); // до переделки формул ограничим кап так

	float eff_cha = 0.0;
	if (GetRealLevel(ch) <= 14) {
		eff_cha = key_value
			- 6 * (float) (14 - GetRealLevel(ch)) / 13.0 + key_value_add
			* (0.2 + 0.3 * (float) (GetRealLevel(ch) - 1) / 13.0);
	} else if (GetRealLevel(ch) <= 26) {
		eff_cha = key_value + key_value_add * (0.5 + 0.5 * (float) (GetRealLevel(ch) - 14) / 12.0);
	} else {
		eff_cha = key_value + key_value_add;
	}

	return VPOSI<float>(eff_cha, 1.0f, static_cast<float>(max_cha));
}

float CalcEffectiveWis(CharData *ch, ESpell spell_id) {
	int key_value, key_value_add;
	auto max_wis = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kWis);

	if (spell_id == ESpell::kResurrection || spell_id == ESpell::kAnimateDead) {
		key_value = ch->get_wis();
		key_value_add = std::min(max_wis - ch->get_wis(), GET_WIS_ADD(ch));
	} else {
		//если гдето вылезет косяком
		key_value = 0;
		key_value_add = 0;
	}

	float eff_wis = 0.0;
	if (GetRealLevel(ch) <= 14) {
		eff_wis = key_value
			- 6 * (float) (14 - GetRealLevel(ch)) / 13.0 + key_value_add
			* (0.4 + 0.6 * (float) (GetRealLevel(ch) - 1) / 13.0);
	} else if (GetRealLevel(ch) <= 26) {
		eff_wis = key_value + key_value_add * (0.5 + 0.5 * (float) (GetRealLevel(ch) - 14) / 12.0);
	} else {
		eff_wis = key_value + key_value_add;
	}

	return VPOSI<float>(eff_wis, 1.0f, static_cast<float>(max_wis));
}

float get_effective_int(CharData *ch) {
	int key_value, key_value_add;

	key_value = ch->get_int();
	auto max_int = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kInt);
	key_value_add = std::min(max_int - ch->get_int(), GET_INT_ADD(ch));

	float eff_int = 0.0;
	if (GetRealLevel(ch) <= 14) {
		eff_int = key_value
			- 6 * (float) (14 - GetRealLevel(ch)) / 13.0 + key_value_add
			* (0.2 + 0.3 * (float) (GetRealLevel(ch) - 1) / 13.0);
	} else if (GetRealLevel(ch) <= 26) {
		eff_int = key_value + key_value_add * (0.5 + 0.5 * (float) (GetRealLevel(ch) - 14) / 12.0);
	} else {
		eff_int = key_value + key_value_add;
	}

	return VPOSI<float>(eff_int, 1.0f, static_cast<float>(max_int));
}

int CalcCharmPoint(CharData *ch, ESpell spell_id) {
	float r_hp = 0;
	auto eff_cha{0.0};
	auto stat_cap{0.0};

	if (spell_id == ESpell::kResurrection || spell_id == ESpell::kAnimateDead) {
		eff_cha = CalcEffectiveWis(ch, spell_id);
		stat_cap = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kWis);
	} else {
		stat_cap = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kCha);
		eff_cha = get_effective_cha(ch);
	}

	if (spell_id != ESpell::kCharm) {
		eff_cha = std::min(stat_cap, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
	}

	if (eff_cha < stat_cap) {
		r_hp = (1 - eff_cha + (int) eff_cha) * ChaApp((int) eff_cha).charms +
			(eff_cha - (int) eff_cha) * ChaApp((int) eff_cha + 1).charms;
	} else {
		r_hp = (1 - eff_cha + (int) eff_cha) * ChaApp((int) eff_cha).charms;
	}
	float remort_coeff = 1.0 + (((float) remort::GetRealRemort(ch) - 9.0) * 1.2) / 100.0;
	if (remort_coeff > 1.0f) {
		r_hp *= remort_coeff;
	}

	if (ch->IsFlagged(EPrf::kTester))
		SendMsgToChar(ch, "&Gget_player_charms Расчет чарма r_hp = %f \r\n&n", r_hp);
	return (int) r_hp;
}

void ClearMinionTalents(CharData *ch) {
	ch->real_abils.Feats.reset();
	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		GET_SPELL_TYPE(ch, spell_id) = 0;
		GET_SPELL_MEM(ch, spell_id) = 0;
	}
	ch->clear_skills();
}

float CalcDamagePerRound(CharData *victim) {
	float dam_per_attack = GET_DR(victim) + str_bonus(victim->get_str(), STR_TO_DAM)
		+ victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2.0
		+ (AFF_FLAGGED(victim, EAffect::kCloudOfArrows) ? 14 : 0);
	int num_attacks = 1 + victim->mob_specials.extra_attack
		+ (GetSkill(victim, ESkill::kAddshot) ? 2 : 0);

	float dam_per_round = dam_per_attack * num_attacks;

	//Если дыхание - то дамаг умножается
	if (victim->IsFlagged(EMobFlag::kFireBreath) ||
		victim->IsFlagged(EMobFlag::kGasBreath) ||
		victim->IsFlagged(EMobFlag::kFrostBreath) ||
		victim->IsFlagged(EMobFlag::kAcidBreath) ||
		victim->IsFlagged(EMobFlag::kLightingBreath)) {
		dam_per_round *= 1.3f;
	}

	return dam_per_round;
}

int GetReformedCharmiceHp(CharData *ch, CharData *victim, ESpell spell_id) {
	auto r_hp{0.0};
	auto eff_cha{0.0};
	auto stat_cap{0.0};

	if (spell_id == ESpell::kResurrection || spell_id == ESpell::kAnimateDead) {
		eff_cha = CalcEffectiveWis(ch, spell_id);
		stat_cap = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kWis);
	} else {
		stat_cap = MUD::Class(ch->GetClass()).GetBaseStatCap(EBaseStat::kCha);
		eff_cha = get_effective_cha(ch);
	}

	if (spell_id != ESpell::kCharm) {
		eff_cha = std::min(stat_cap, eff_cha + 2); // Все кроме чарма кастится с бонусом в 2
	}

	// Интерполяция между значениями для целых значений обаяния
	if (eff_cha < stat_cap) {
		r_hp = victim->get_max_hit() + CalcDamagePerRound(victim) *
			((1 - eff_cha + (int) eff_cha) * ChaApp((int) eff_cha).dam_to_hit_rate +
				(eff_cha - (int) eff_cha) * ChaApp((int) eff_cha + 1).dam_to_hit_rate);
	} else {
		r_hp = victim->get_max_hit() + CalcDamagePerRound(victim) *
			((1 - eff_cha + (int) eff_cha) * ChaApp((int) eff_cha).dam_to_hit_rate);
	}

	if (ch->IsFlagged(EPrf::kTester))
		SendMsgToChar(ch, "&Gget_reformed_charmice_hp Расчет чарма r_hp = %f \r\n&n", r_hp);
	return (int) r_hp;
}

int  MaxCharmices(CharData *ch) {
	return std::max(1, (GetRealLevel(ch) + 9) / 10);
}

int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id) {
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = false, living_in_group = false;

	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)
			&& k->get_master() == ch) {
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->ch);
			reformed_hp_summ += GetReformedCharmiceHp(ch, k, spell_id);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (k->IsFlagged(EMobFlag::kCorpse)) {
				undead_in_group = true;
			} else {
				living_in_group = true;
			}
		}
	}

	if (undead_in_group && living_in_group) {
		mudlog("SYSERR: Undead and living in group simultaniously", NRM, kLvlGod, ERRLOG, true);
		return (false);
	}

	if (spell_id == ESpell::kCharm && undead_in_group) {
		SendMsgToChar("Ваша жертва боится ваших последователей.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kCharm && living_in_group) {
		SendMsgToChar("Ваш последователь мешает вам произнести это заклинание.\r\n", ch);
		return (false);
	}

	if (cha_summ >= MaxCharmices(ch)) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}
	if (spell_id != ESpell::kClone &&
		reformed_hp_summ + GetReformedCharmiceHp(ch, victim, spell_id) >= CalcCharmPoint(ch, spell_id)) {
		SendMsgToChar("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (false);
	}
	return (true);
}

bool IsCharmice(const CharData *ch) {
	return ch->IsNpc()
		&& (AFF_FLAGGED(ch, EAffect::kHelper)
			|| AFF_FLAGGED(ch, EAffect::kCharmed));
}

bool IsMortifier(const CharData *ch) {
	return ch->IsNpc()
		&& ch->has_master()
		&& ch->IsFlagged(EMobFlag::kCorpse);
}

// issue.chardata-cleaning: was CharData::low_charm -- a kCharm affect about to wear off.
bool IsCharmExpiring(const CharData *ch) {
	for (const auto &aff : ch->affected) {
		if (aff->type == ESpell::kCharm && aff->duration <= 1) {
			return true;
		}
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
