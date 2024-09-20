/**
\file do_summon.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/sight.h"

void do_summon(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	CharData *horse = nullptr;
	horse = ch->get_horse();
	if (!horse) {
		SendMsgToChar("У вас нет скакуна!\r\n", ch);
		return;
	}

	if (ch->in_room == horse->in_room) {
		SendMsgToChar("Но ваш cкакун рядом с вами!\r\n", ch);
		return;
	}

	SendMsgToChar("Ваш скакун появился перед вами.\r\n", ch);
	act("$n исчез$q в голубом пламени.", true, horse, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(horse);
	PlaceCharToRoom(horse, ch->in_room);
	look_at_room(horse, 0);
	act("$n появил$u из голубого пламени!", true, horse, nullptr, nullptr, kToRoom);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
