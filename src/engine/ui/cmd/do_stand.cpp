/**
\file do_stand.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"

void do_stand(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetPosition() > EPosition::kSleep && AFF_FLAGGED(ch, EAffect::kSleep)) {
		SendMsgToChar("Вы сладко зевнули и решили еще немного подремать.\r\n", ch);
		act("$n сладко зевнул$a и решил$a еще немного подремать.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		ch->SetPosition(EPosition::kSleep);
	}

	if (ch->IsOnHorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (ch->GetPosition()) {
		case EPosition::kStand: SendMsgToChar("А вы уже стоите.\r\n", ch);
			break;
		case EPosition::kSit: SendMsgToChar("Вы встали.\r\n", ch);
			act("$n поднял$u.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			// Will be sitting after a successful bash and may still be fighting.
			ch->GetEnemy() ? ch->SetPosition(EPosition::kFight) : ch->SetPosition(EPosition::kStand);
			break;
		case EPosition::kRest: SendMsgToChar("Вы прекратили отдыхать и встали.\r\n", ch);
			act("$n прекратил$g отдых и поднял$u.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->GetEnemy() ? ch->SetPosition(EPosition::kFight) : ch->SetPosition(EPosition::kStand);
			break;
		case EPosition::kSleep: SendMsgToChar("Пожалуй, сначала стоит проснуться!\r\n", ch);
			break;
		case EPosition::kFight: SendMsgToChar("Вы дрались лежа? Это что-то новенькое.\r\n", ch);
			break;
		default: SendMsgToChar("Вы прекратили летать и опустились на грешную землю.\r\n", ch);
			act("$n опустил$u на землю.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			ch->SetPosition(EPosition::kStand);
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
