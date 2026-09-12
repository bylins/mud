/**
\file do_diagnose.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/target_resolver.h"

void do_diagnose(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	char name[kMaxInputLength];
	one_argument(argument, name);

	if (*name) {
		vict = target_resolver::FindCharInRoom(ch, name);
		if (!vict)
			SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
		else
			sight::diag_char_to_char(vict, ch);
	} else {
		if (ch->GetEnemy())
			sight::diag_char_to_char(ch->GetEnemy(), ch);
		else
			SendMsgToChar("На кого вы хотите взглянуть?\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
