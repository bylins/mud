/**
\file do_sit.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"

void do_sit(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsOnHorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (ch->GetPosition()) {
		case EPosition::kStand: SendMsgToChar("Вы сели.\r\n", ch);
			act("$n сел$g.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kSit);
			break;
		case EPosition::kSit: SendMsgToChar("А вы и так сидите.\r\n", ch);
			break;
		case EPosition::kRest: SendMsgToChar("Вы прекратили отдыхать и сели.\r\n", ch);
			act("$n прервал$g отдых и сел$g.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kSit);
			break;
		case EPosition::kSleep: SendMsgToChar("Вам стоит проснуться.\r\n", ch);
			break;
		case EPosition::kFight: SendMsgToChar("Сесть? Во время боя? Вы явно не в себе.\r\n", ch);
			break;
		default: SendMsgToChar("Вы прекратили свой полет и сели.\r\n", ch);
			act("$n прекратил$g свой полет и сел$g.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kSit);
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
