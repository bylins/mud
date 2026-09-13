/**
\file do_wimpy.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"

void do_wimpy(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int wimp_lev;

	// 'wimp_level' is a player_special. -gg 2/25/98
	if (ch->IsNpc())
		return;

	char level_arg[kMaxInputLength];
	one_argument(argument, level_arg);

	if (!*level_arg) {
		if (GET_WIMP_LEV(ch)) {
			SendMsgToChar(fmt::format("Вы попытаетесь бежать при {} ХП.\r\n", GET_WIMP_LEV(ch)), ch);
			return;
		} else {
			SendMsgToChar("Вы будете драться, драться и драться (пока не помрете, ессно...).\r\n", ch);
			return;
		}
	}
	if (a_isdigit(*level_arg)) {
		if ((wimp_lev = atoi(level_arg)) != 0) {
			if (wimp_lev < 0)
				SendMsgToChar("Да, перегрев похоже. С такими хитами вы и так помрете :)\r\n", ch);
			else if (wimp_lev > ch->get_real_max_hit())
				SendMsgToChar("Осталось только дожить до такого количества ХП.\r\n", ch);
			else if (wimp_lev > (ch->get_real_max_hit() / 2))
				SendMsgToChar("Размечтались. Сбечь то можно, но не более половины максимальных ХП.\r\n", ch);
			else {
				SendMsgToChar(fmt::format("Ладушки. Вы сбегите (или сбежите) по достижению {} ХП.\r\n", wimp_lev), ch);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		} else {
			SendMsgToChar("Вы будете сражаться до конца (скорее всего своего ;).\r\n", ch);
			GET_WIMP_LEV(ch) = 0;
		}
	} else
		SendMsgToChar("Уточните, при достижении какого количества ХП вы планируете сбежать (0 - драться до смерти)\r\n",
					  ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
