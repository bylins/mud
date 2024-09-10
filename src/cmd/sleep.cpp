/**
\file sleep.cpp - a part of Bylins engine.
\authors Created by Sventovit.
\date 08.09.2024.
\brief 'Do sleep' command.
*/

#include "entities/char_data.h"

void do_sleep(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (GetRealLevel(ch) >= kLvlImmortal) {
		SendMsgToChar("Не время вам спать, родина в опасности!\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (ch->GetPosition()) {
		case EPosition::kStand:
		case EPosition::kSit:
		case EPosition::kRest: SendMsgToChar("Вы заснули.\r\n", ch);
			act("$n сладко зевнул$g и задал$g храпака.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kSleep);
			break;
		case EPosition::kSleep: SendMsgToChar("А вы и так спите.\r\n", ch);
			break;
		case EPosition::kFight: SendMsgToChar("Вам нужно сражаться! Отоспитесь после смерти.\r\n", ch);
			break;
		default: SendMsgToChar("Вы прекратили свой полет и отошли ко сну.\r\n", ch);
			act("$n прекратил$g летать и нагло заснул$g.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kSleep);
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
