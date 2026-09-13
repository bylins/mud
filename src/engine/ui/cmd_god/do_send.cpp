/**
\file do_send_msg_to_char.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "engine/core/target_resolver.h"

void DoSendMsgToChar(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;

	char name[kMaxInputLength];
	char message[kMaxStringLength];
	half_chop(argument, name, message);

	if (!*name) {
		SendMsgToChar("Послать что и кому (не путать с куда и кого :)\r\n", ch);
		return;
	}
	if (!(vict = target_resolver::FindPlayerVis(ch, name))) {
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
		return;
	}
	SendMsgToChar(message, vict);
	SendMsgToChar("\r\n", vict);
	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar("Послано.\r\n", ch);
	else {
		SendMsgToChar(fmt::format("Вы послали '{}' {}.\r\n", message, GET_PAD(vict, 2)), ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
