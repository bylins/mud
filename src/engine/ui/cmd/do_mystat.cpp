/**
\file do_mystat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "gameplay/statistics/char_stat.h"
#include "engine/entities/char_data.h"
#include "utils/utils_string.h"

void do_mystat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);
	if (utils::IsAbbr(argument, "очистить") || utils::IsAbbr(argument, "clear")) {
		ch->ClearStatistics();
		SendMsgToChar("Статистика очищена.\r\n", ch);
	} else {
		CharStat::Print(ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
