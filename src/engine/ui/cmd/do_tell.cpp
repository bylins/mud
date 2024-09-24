/**
\file do_tell.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 23.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/communication/talk.h"
#include "engine/core/handler.h"

void do_tell(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;

	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	/* Непонятно нафига нужно
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kTribune))
	{
		SendMsgToChar(SOUNDPROOF, ch);
		return;
	}
	*/

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2) {
		SendMsgToChar("Что и кому вы хотите сказать?\r\n", ch);
	} else if (!(vict = get_player_vis(ch, buf, EFind::kCharInWorld))) {
		SendMsgToChar(NOPERSON, ch);
	} else if (vict->IsNpc())
		SendMsgToChar(NOPERSON, ch);
	else if (is_tell_ok(ch, vict)) {
		if (ch->IsFlagged(EPrf::kNoTell))
			SendMsgToChar("Ответить вам не смогут!\r\n", ch);
		perform_tell(ch, vict, buf2);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
