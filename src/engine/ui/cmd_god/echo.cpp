/**
\file echo.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/communication/ignores.h"

extern const char *deaf_social;

void do_echo(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вы не в состоянии что-либо продемонстрировать окружающим.\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("И что вы хотите выразить столь красочно?\r\n", ch);
	} else {
		if (subcmd == SCMD_EMOTE) {
			if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed)) {
				if (ch->get_master()->IsFlagged(EPlrFlag::kDumbed)) {
					SendMsgToChar("Ваши последователи так же немы, как и вы!\r\n", ch->get_master());
					return;
				}
			}
			sprintf(buf, "&K$n %s.&n", argument);
		} else {
			strcpy(buf, argument);
		}

		for (const auto to : world[ch->in_room]->people) {
			if (to == ch
				|| ignores(to, ch, EIgnore::kEmote)) {
				continue;
			}

			act(buf, false, ch, nullptr, to, kToVict | kToNotDeaf);
			act(deaf_social, false, ch, nullptr, to, kToVict | kToDeaf);
		}

		if (ch->IsFlagged(EPrf::kNoRepeat)) {
			SendMsgToChar(OK, ch);
		} else {
			act(buf, false, ch, nullptr, nullptr, kToChar);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
