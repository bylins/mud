/**
\file do_sdemigods.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/communication/remember.h"

void DoSendMsgToDemigods(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("Что Вы хотите сообщить ?\r\n", ch);
		return;
	}
	sprintf(buf1, "&c%s демигодам: '%s'&n\r\n", GET_NAME(ch), argument);

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING) {
			if ((GET_GOD_FLAG(d->character, EGf::kDemigod)) || (GetRealLevel(d->character) == kLvlImplementator)) {
				if ((!d->character->IsFlagged(EPlrFlag::kWriting)) &&
					(!d->character->IsFlagged(EPlrFlag::kMailing)) &&
					(!d->character->IsFlagged(EPrf::kDemigodChat))) {
					d->character->remember_add(buf1, Remember::ALL);
					SendMsgToChar(buf1, d->character.get());
				}
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
