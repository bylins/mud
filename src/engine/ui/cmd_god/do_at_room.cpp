/**
\file do_at_room.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

void DoAtRoom(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char command[kMaxInputLength];
	RoomRnum location, original_loc;

	half_chop(argument, buf, command);
	if (!*buf) {
		SendMsgToChar("Необходимо указать номер или название комнаты.\r\n", ch);
		return;
	}

	if (!*command) {
		SendMsgToChar("Что вы собираетесь там делать?\r\n", ch);
		return;
	}

	if ((location = FindRoomRnum(ch, buf, 0)) == kNowhere) {
		return;
	}

	// a location has been found.
	original_loc = ch->in_room;
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
	command_interpreter(ch, command);

	// check if the char is still there
	if (ch->in_room == location) {
		RemoveCharFromRoom(ch);
		PlaceCharToRoom(ch, original_loc);
	}
	ch->dismount();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
