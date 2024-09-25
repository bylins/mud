/**
\file do_rest.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"

void do_rest(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsOnHorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (ch->GetPosition()) {
		case EPosition::kStand: SendMsgToChar("Вы присели отдохнуть.\r\n", ch);
			act("$n присел$g отдохнуть.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kRest);
			break;
		case EPosition::kSit: SendMsgToChar("Вы пристроились поудобнее для отдыха.\r\n", ch);
			act("$n пристроил$u поудобнее для отдыха.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kRest);
			break;
		case EPosition::kRest: SendMsgToChar("Вы и так отдыхаете.\r\n", ch);
			break;
		case EPosition::kSleep: SendMsgToChar("Вам лучше сначала проснуться.\r\n", ch);
			break;
		case EPosition::kFight: SendMsgToChar("Отдыха в бою вам не будет!\r\n", ch);
			break;
		default: SendMsgToChar("Вы прекратили полет и присели отдохнуть.\r\n", ch);
			act("$n прекратил$g полет и пристроил$u поудобнее для отдыха.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kRest);
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
