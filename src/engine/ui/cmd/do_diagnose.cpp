/**
\file do_diagnose.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/handler.h"

void do_diagnose(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom)))
			SendMsgToChar(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	} else {
		if (ch->GetEnemy())
			diag_char_to_char(ch->GetEnemy(), ch);
		else
			SendMsgToChar("На кого вы хотите взглянуть?\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
