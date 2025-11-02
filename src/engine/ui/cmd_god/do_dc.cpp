/**
\file DoDropConnect.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/comm.h"

void DoDropConnect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	int num_to_dc;
	one_argument(argument, arg);
	if (!(num_to_dc = atoi(arg))) {
		SendMsgToChar("Usage: DC <user number> (type USERS for a list)\r\n", ch);
		return;
	}
	for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

	if (!d) {
		SendMsgToChar("Нет такого соединения.\r\n", ch);
		return;
	}

	if (d->character) //Чтоб не крешило при попытке разъединить незалогиненного
	{
		int victim_level = d->character->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(d->character);
		int god_level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
		if (victim_level >= god_level) {
			if (!CAN_SEE(ch, d->character))
				SendMsgToChar("Нет такого соединения.\r\n", ch);
			else
				SendMsgToChar("Да уж.. Это не есть праффильная идея...\r\n", ch);
			return;
		}
	}

	/* We used to just close the socket here using close_socket(), but
	 * various people pointed out this could cause a crash if you're
	 * closing the person below you on the descriptor list.  Just setting
	 * to CON_CLOSE leaves things in a massively inconsistent state so I
	 * had to add this new flag to the descriptor.
	 *
	 * It is a much more logical extension for a CON_DISCONNECT to be used
	 * for in-game socket closes and CON_CLOSE for out of game closings.
	 * This will retain the stability of the close_me hack while being
	 * neater in appearance. -gg 12/1/97
	 */
	if (d->state == EConState::kDisconnect || d->state == EConState::kClose) {
		SendMsgToChar("Соединение уже разорвано.\r\n", ch);
	} else {
		/*
		 * Remember that we can disconnect people not in the game and
		 * that rather confuses the code when it expected there to be
		 * a character context.
		 */
		if (d->state == EConState::kPlaying) {
			d->state = EConState::kDisconnect;
		} else {
			d->state = EConState::kClose;
		}

		sprintf(buf, "Соединение #%d закрыто.\r\n", num_to_dc);
		SendMsgToChar(buf, ch);
		imm_log("Connect closed by %s.", GET_NAME(ch));
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
