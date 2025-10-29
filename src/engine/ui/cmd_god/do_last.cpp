/**
\file do_last.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/db/global_objects.h"

void DoPageLastLogins(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Кого вы хотите найти?\r\n", ch);
		return;
	}

	Player t_chdata;
	Player *chdata = &t_chdata;
	if (LoadPlayerCharacter(arg, chdata, ELoadCharFlags::kFindId) < 0) {
		SendMsgToChar("Нет такого игрока.\r\n", ch);
		return;
	}
	if (GetRealLevel(chdata) > GetRealLevel(ch) && !IS_IMPL(ch) && !ch->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar("Вы не столь уж и божественны для этого.\r\n", ch);
	} else {
		time_t tmp_time = LAST_LOGON(chdata);
		sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
				GET_UID(chdata), GetRealLevel(chdata),
				MUD::Class(chdata->GetClass()).GetAbbr().c_str(), GET_NAME(chdata),
				GET_LASTIP(chdata)[0] ? GET_LASTIP(chdata) : "Unknown", ctime(&tmp_time));
		SendMsgToChar(buf, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
