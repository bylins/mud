/**
\file base_stats.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Механика базовых параметров.
\detail Все, что связано с базовыми параметрами, боннсы-штрафы, нужно разместить тут или переместить сюда.
*/

#ifndef BYLINS_SRC_GAMEPLAY_CORE_BASE_STATS_H_
#define BYLINS_SRC_GAMEPLAY_CORE_BASE_STATS_H_

enum { STR_TO_HIT, STR_TO_DAM, STR_CARRY_W, STR_WIELD_W, STR_HOLD_W, STR_BOTH_W, STR_SHIELD_W };
enum { WIS_MAX_LEARN_L20, WIS_SPELL_SUCCESS, WIS_MAX_SKILLS, WIS_FAILS };

class CharData;

int str_bonus(int str, int type);
int dex_bonus(int dex);
int dex_ac_bonus(int dex);
int wis_bonus(int stat, int type);
int calc_str_req(int weight, int type);
int CAN_CARRY_N(const CharData *ch);
// \todo Переделать на функцию
#define CAN_CARRY_W(ch) ((str_bonus(GetRealStr(ch), STR_CARRY_W) * ((ch)->HaveFeat(EFeat::kPorter) ? 110 : 100))/100)

#endif //BYLINS_SRC_GAMEPLAY_CORE_BASE_STATS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
