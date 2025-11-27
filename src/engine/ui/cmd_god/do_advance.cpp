/**
\file DoAdvance.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/classes/pc_classes.h"
#include "engine/core/handler.h"
#include "gameplay/core/game_limits.h"

void DoAdvance(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	char *name = arg, *level = buf2;
	int newlevel, oldlevel;

	two_arguments(argument, name, level);

	if (*name) {
		if (!(victim = get_player_vis(ch, name, EFind::kCharInWorld))) {
			SendMsgToChar("Не найду такого игрока.\r\n", ch);
			return;
		}
	} else {
		SendMsgToChar("Повысить кого?\r\n", ch);
		return;
	}

	if (GetRealLevel(ch) <= GetRealLevel(victim) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar("Нелогично.\r\n", ch);
		return;
	}
	if (!*level || (newlevel = atoi(level)) <= 0) {
		SendMsgToChar("Это не похоже на уровень.\r\n", ch);
		return;
	}
	if (newlevel > kLvlImplementator) {
		sprintf(buf, "%d - максимальный возможный уровень.\r\n", kLvlImplementator);
		SendMsgToChar(buf, ch);
		return;
	}
	if (newlevel > GetRealLevel(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar("Вы не можете установить уровень выше собственного.\r\n", ch);
		return;
	}
	if (newlevel == GetRealLevel(victim)) {
		act("$E и так этого уровня.", false, ch, nullptr, victim, kToChar);
		return;
	}
	oldlevel = GetRealLevel(victim);
	if (newlevel < oldlevel) {
		SendMsgToChar("Вас окутало облако тьмы.\r\n" "Вы почувствовали себя лишенным чего-то.\r\n", victim);
	} else {
		act("$n сделал$g несколько странных пасов.\r\n"
			"Вам показалось, будто неземное тепло разлилось по каждой клеточке\r\n"
			"Вашего тела, наполняя его доселе невиданными вами ощущениями.\r\n",
			false, ch, nullptr, victim, kToVict);
	}

	SendMsgToChar(OK, ch);
	if (newlevel < oldlevel) {
		log("(GC) %s demoted %s from level %d to %d.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
		imm_log("%s demoted %s from level %d to %d.", GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
	} else {
		log("(GC) %s has advanced %s to level %d (from %d)",
			GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
		imm_log("%s has advanced %s to level %d (from %d)", GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
	}

	gain_exp_regardless(victim, GetExpUntilNextLvl(victim, newlevel)
		- victim->get_exp());
	victim->save_char();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
