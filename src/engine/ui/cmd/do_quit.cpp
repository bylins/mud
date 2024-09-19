/**
\file do_quit.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd/do_quit.h"

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/clans/house.h"
#include "gameplay/fight/fight.h"
#include "engine/core/handler.h"

void do_quit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	DescriptorData *d, *next_d;

	if (ch->IsNpc() || !ch->desc)
		return;

	if (subcmd != kScmdQuit)
		SendMsgToChar("Вам стоит набрать эту команду полностью во избежание недоразумений!\r\n", ch);
	else if (ch->GetPosition() == EPosition::kFight)
		SendMsgToChar("Угу! Щаз-з-з! Вы, батенька, деретесь!\r\n", ch);
	else if (ch->GetPosition() < EPosition::kStun) {
		SendMsgToChar("Вас пригласила к себе владелица косы...\r\n", ch);
		die(ch, nullptr);
	}
	else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		if (GET_SEX(ch) == EGender::kMale)
			SendMsgToChar("Сдался салага? Не выйдет...", ch);
		else
			SendMsgToChar("Сдалась салага? Не выйдет...", ch);
		return;
	} else if (AFF_FLAGGED(ch, EAffect::kSleep)) {
		return;
	} else if (*argument) {
		SendMsgToChar("Если вы хотите выйти из игры с потерей всех вещей, то просто наберите 'конец'.\r\n", ch);
	} else {
//		int loadroom = ch->in_room;
		if (NORENTABLE(ch)) {
			SendMsgToChar("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
			return;
		}
		if (!GET_INVIS_LEV(ch))
			act("$n покинул$g игру.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		sprintf(buf, "%s quit the game.", GET_NAME(ch));
		mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		SendMsgToChar("До свидания, странник... Мы ждем тебя снова!\r\n", ch);

		long depot_cost = static_cast<long>(Depot::get_total_cost_per_day(ch));
		if (depot_cost) {
			SendMsgToChar(ch,
						  "За вещи в хранилище придется заплатить %ld %s в день.\r\n",
						  depot_cost,
						  GetDeclensionInNumber(depot_cost, EWhat::kMoneyU));
			long deadline = ((ch->get_gold() + ch->get_bank()) / depot_cost);
			SendMsgToChar(ch, "Твоих денег хватит на %ld %s.\r\n", deadline,
						  GetDeclensionInNumber(deadline, EWhat::kDay));
		}
		Depot::exit_char(ch);
		Clan::clan_invoice(ch, false);

		/*
		 * kill off all sockets connected to the same player as the one who is
		 * trying to quit.  Helps to maintain sanity as well as prevent duping.
		 */
		for (d = descriptor_list; d; d = next_d) {
			next_d = d->next;
			if (d == ch->desc)
				continue;
			if (d->character && (GET_UID(d->character) == GET_UID(ch)))
				STATE(d) = CON_DISCONNECT;
		}
		ExtractCharFromWorld(ch, false);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
