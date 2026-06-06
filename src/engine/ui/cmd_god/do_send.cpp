/**
\file do_send_msg_to_char.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

void DoSendMsgToChar(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	half_chop(argument, arg, buf);

	if (!*arg) {
		SendMsgToChar("Послать что и кому (не путать с куда и кого :)\r\n", ch);
		return;
	}
	if (!(vict = get_player_vis(ch, arg, EFind::kCharInWorld))) {
		SendMsgToChar(NOPERSON, ch);
		return;
	}
	SendMsgToChar(buf, vict);
	SendMsgToChar("\r\n", vict);
	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar("Послано.\r\n", ch);
	else {
		snprintf(buf2, kMaxStringLength, "Вы послали '%s' %s.\r\n", buf, GET_PAD(vict, 2));
		SendMsgToChar(buf2, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
