/**
\file look_around.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"

void DoLookAround(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int i;

	if (!ch->desc)
		return;

	if (ch->GetPosition() <= EPosition::kSleep)
		SendMsgToChar("Виделся часто сон беспокойный...\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffect::kBlind))
		SendMsgToChar("Вы ослеплены!\r\n", ch);
	else {
		skip_hide_on_look(ch);
		SendMsgToChar("Вы посмотрели по сторонам.\r\n", ch);
		for (i = 0; i < EDirection::kMaxDirNum; i++) {
			look_in_direction(ch, i, 0);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
