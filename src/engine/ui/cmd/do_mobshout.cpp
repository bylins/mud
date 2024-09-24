/**
\file do_mobshout.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/ui/color.h"

void do_mobshout(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *i;

	// to keep pets, etc from being ordered to shout
	if (!(ch->IsNpc() || IS_IMMORTAL(ch)))
		return;
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	skip_spaces(&argument); //убираем пробел в начале сообщения
	sprintf(buf, "$n заорал$g : '%s'", argument);

	// now send all the strings out
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING
			&& i->character
			&& !i->character->IsFlagged(EPlrFlag::kWriting)
			&& i->character->GetPosition() > EPosition::kSleep) {
			SendMsgToChar(kColorBoldYel, i->character.get());
			act(buf, false, ch, nullptr, i->character.get(), kToVict | kToSleep | kToNotDeaf);
			SendMsgToChar(kColorNrm, i->character.get());
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
