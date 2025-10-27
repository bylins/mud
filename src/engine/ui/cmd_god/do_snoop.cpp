/**
\file do_snoop.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

void StopSnooping(CharData *ch) {
	if (!ch->desc->snooping)
		SendMsgToChar("Вы не подслушиваете.\r\n", ch);
	else {
		SendMsgToChar("Вы прекратили подслушивать.\r\n", ch);
		ch->desc->snooping->snoop_by = nullptr;
		ch->desc->snooping = nullptr;
	}
}

void DoSnoop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim, *tch;

	if (!ch->desc)
		return;

	argument = one_argument(argument, arg);

	if (!*arg)
		StopSnooping(ch);
	else if (!(victim = get_player_vis(ch, arg, EFind::kCharInWorld)))
		SendMsgToChar("Нет такого создания в игре.\r\n", ch);
	else if (!victim->desc)
		act("Вы не можете $S подслушать - он$G потерял$G связь..\r\n",
			false, ch, nullptr, victim, kToChar);
	else if (victim == ch)
		StopSnooping(ch);
	else if (victim->desc->snooping == ch->desc)
		SendMsgToChar("Вы уже подслушиваете.\r\n", ch);
	else if (victim->desc->snoop_by && victim->desc->snoop_by != ch->desc)
		SendMsgToChar("Дык его уже кто-то из богов подслушивает.\r\n", ch);
	else {
		if (victim->desc->original)
			tch = victim->desc->original.get();
		else
			tch = victim;

		const int god_level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		const int victim_level = tch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(tch);

		if (victim_level >= god_level) {
			SendMsgToChar("Вы не можете.\r\n", ch);
			return;
		}
		SendMsgToChar(OK, ch);

		ch->desc->snoop_with_map = false;
		if (god_level >= kLvlImplementator && argument && *argument) {
			skip_spaces(&argument);
			if (isname(argument, "map") || isname(argument, "карта")) {
				ch->desc->snoop_with_map = true;
			}
		}

		if (ch->desc->snooping)
			ch->desc->snooping->snoop_by = nullptr;

		ch->desc->snooping = victim->desc;
		victim->desc->snoop_by = ch->desc;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
