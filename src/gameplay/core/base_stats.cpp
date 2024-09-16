/**
\file base_stats.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Механика базовых параметров.
\detail Все, что связано с базовыми параметрами, боннсы-штрафы, нужно разместить тут или переместить сюда.
*/

#include "base_stats.h"

#include "utils/logger.h"
#include "engine/entities/char_data.h"

int str_bonus(int str, int type) {
	int bonus = 0;
	str = std::max(1, str);

	switch (type) {
		case STR_TO_HIT:
			// -5 ... 10
			if (str < 10) {
				bonus = (str - 11) / 2;
			} else {
				bonus = (str - 10) / 4;
			}
			break;
		case STR_TO_DAM:
			// -5 ... 36
			if (str < 15) {
				bonus = (str - 11) / 2;
			} else {
				bonus = str - 14;
			}
			break;
		case STR_CARRY_W:
			// 50 ... 2500
			bonus = str * 50;
			break;
		case STR_WIELD_W:
			if (str <= 20) {
				// w 1 .. 20
				bonus = str;
			} else if (str < 30) {
				// w 20,5 .. 24,5
				bonus = 20 + (str - 20) / 2;
			} else {
				// w >= 25
				bonus = 25 + (str - 30) / 4;
			}
			break;
		case STR_HOLD_W:
			if (str <= 29) {
				// w 1 .. 15
				bonus = (str + 1) / 2;
			} else {
				// w >= 16
				bonus = 15 + (str - 29) / 4;
			}
			break;
		default:
			log("SYSERROR: str=%d, type=%d (%s %s %d)",
				str, type, __FILE__, __func__, __LINE__);
	}

	return bonus;
}

int dex_bonus(int dex) {
	int bonus = 0;
	dex = std::max(1, dex);
	// -5 ... 40
	if (dex < 10) {
		bonus = (dex - 11) / 2;
	} else {
		bonus = dex - 10;
	}
	return bonus;
}

int dex_ac_bonus(int dex) {
	int bonus = 0;
	dex = std::max(1, dex);
	// -3 ... 35
	if (dex <= 15) {
		bonus = (dex - 10) / 3;
	} else {
		bonus = dex - 15;
	}
	return bonus;
}

int wis_bonus(int stat, int type) {
	int bonus = 0;
	stat = std::max(1, stat);

	switch (type) {
		case WIS_MAX_LEARN_L20:
			// 28 .. 175
			bonus = 28 + (stat - 1) * 3;
			break;
		case WIS_SPELL_SUCCESS:
			// -80 .. 116
			bonus = stat * 4 - 84;
			break;
		case WIS_MAX_SKILLS:
			// 1 .. 15
			if (stat <= 15) {
				bonus = stat;
			}
				// 15 .. 32
			else {
				bonus = 15 + (stat - 15) / 2;
			}
			break;
		case WIS_FAILS:
			// 34 .. 66
			if (stat <= 9) {
				bonus = 30 + stat * 4;
			}
				// 140 .. 940
			else {
				bonus = 120 + (stat - 9) * 20;
			}
			break;
		default:
			log("SYSERROR: stat=%d, type=%d (%s %s %d)",
				stat, type, __FILE__, __func__, __LINE__);
	}

	return bonus;
}

int calc_str_req(int weight, int type) {
	int str = 0;
	weight = std::max(1, weight);

	switch (type) {
		case STR_WIELD_W:
			if (weight <= 20) {
				str = weight;
			} else if (weight <= 24) {
				str = 2 * weight - 20;
			} else {
				str = 4 * weight - 70;
			}
			break;
		case STR_HOLD_W:
			if (weight <= 15) {
				str = 2 * weight - 1;
			} else {
				str = 4 * weight - 31;
			}
			break;
		case STR_BOTH_W:
			if (weight <= 31) {
				str = weight - (weight + 1) / 3;
			} else if (weight <= 40) {
				str = weight - 10;
			} else {
				str = (weight - 39) * 2 + 29 - (weight + 1) % 2;
			}
			break;
		default:
			log("SYSERROR: weight=%d, type=%d (%s %s %d)",
				weight, type, __FILE__, __func__, __LINE__);
	}

	return str;
}

int CAN_CARRY_N(const CharData *ch) {
	int n = 5 + GetRealDex(ch) / 2 + GetRealLevel(ch) / 2;
	if (ch->HaveFeat(EFeat::kJuggler)) {
		n += GetRealLevel(ch) / 2;
		if (CanUseFeat(ch, EFeat::kThrifty)) {
			n += 5;
		}
	}
	if (CanUseFeat(ch, EFeat::kThrifty)) {
		n += 5;
	}
	return std::max(n, 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
