/**
\file do_recall.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/noob.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/sight.h"

void do_recall(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		SendMsgToChar("Монстрам некуда возвращаться!\r\n", ch);
		return;
	}

	const int rent_room = GetRoomRnum(GET_LOADROOM(ch));
	if (rent_room == kNowhere || ch->in_room == kNowhere) {
		SendMsgToChar("Вам некуда возвращаться!\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch)
		&& (SECT(ch->in_room) == ESector::kSecret
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoMagic)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kDeathTrap)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kSlowDeathTrap)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kTunnel)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoRelocateIn)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoTeleportIn)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kIceTrap)
			|| ROOM_FLAGGED(ch->in_room, ERoomFlag::kGodsRoom)
			|| !Clan::MayEnter(ch, ch->in_room, kHousePortal)
			|| !Clan::MayEnter(ch, rent_room, kHousePortal))) {
		SendMsgToChar("У вас не получилось вернуться!\r\n", ch);
		return;
	}

	SendMsgToChar("Вам очень захотелось оказаться подальше от этого места!\r\n", ch);
	if (IS_GOD(ch) || Noob::is_noob(ch)) {
		if (ch->in_room != rent_room) {
			SendMsgToChar("Вы почувствовали, как чья-то огромная рука подхватила вас и куда-то унесла!\r\n", ch);
			act("$n поднял$a глаза к небу и внезапно исчез$q!", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, rent_room);
			look_at_room(ch, 0);
			act("$n внезапно появил$u в центре комнаты!", true, ch, nullptr, nullptr, kToRoom);
		} else {
			SendMsgToChar("Но тут и так достаточно мирно...\r\n", ch);
		}
	} else {
		SendMsgToChar("Ну и хотите себе на здоровье...\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
