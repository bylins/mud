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
		r_hp = (1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].charms +
			(eff_cha - (int) eff_cha) * cha_app[(int) eff_cha + 1].charms;
	} else {
		r_hp = (1 - eff_cha + (int) eff_cha) * cha_app[(int) eff_cha].charms;
	}
	float remort_coeff = 1.0 + (((float) GetRealRemort(ch) - 9.0) * 1.2) / 100.0;
	if (remort_coeff > 1.0f) {
		r_hp *= remort_coeff;
	}

	if (ch->IsFlagged(EPrf::kTester))
		SendMsgToChar(ch, "&Gget_player_charms Расчет чарма r_hp = %f \r\n&n", r_hp);
	return (int) r_hp;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
