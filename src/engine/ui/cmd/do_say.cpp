/**
\file do_say.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/core/constants.h"

void do_say(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (!*argument)
		SendMsgToChar("Вы задумались: \"Чего бы такого сказать?\"\r\n", ch);
	else {
		sprintf(buf, "$n сказал$g : '%s'", argument);

// для возможности игнорирования теллов в клетку
// пришлось изменить act в клетку на проход по клетке
		for (const auto to : world[ch->in_room]->people) {
			if (ch == to || ignores(to, ch, EIgnore::kSay)) {
				continue;
			}
			act(buf, false, ch, nullptr, to, kToVict | DG_NO_TRIG | kToNotDeaf);
		}

		if (ch->IsFlagged(EPrf::kNoRepeat)) {
			SendMsgToChar(OK, ch);
		} else {
			delete_doubledollar(argument);
			sprintf(buf, "Вы сказали : '%s'\r\n", argument);
			SendMsgToChar(buf, ch);
		}
		speech_mtrigger(ch, argument);
		speech_wtrigger(ch, argument);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
