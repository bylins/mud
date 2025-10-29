/**
\file do_poof_msg.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "do_poof_msg.h"

#include "engine/entities/char_data.h"

void DoSetPoofMsg(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	char **msg;

	switch (subcmd) {
		case kScmdPoofin: msg = &(POOFIN(ch));
			break;
		case kScmdPoofout: msg = &(POOFOUT(ch));
			break;
		default: return;
	}

	skip_spaces(&argument);

	if (*msg)
		free(*msg);

	if (!*argument)
		*msg = nullptr;
	else
		*msg = str_dup(argument);

	SendMsgToChar(OK, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
