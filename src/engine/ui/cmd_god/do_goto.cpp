/**
\file do_goto.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/sight.h"

void DoGoto(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	RoomRnum location;

	if ((location = FindRoomRnum(ch, argument, 0)) == kNowhere)
		return;

	if (ch->player_specials->poofout)
		sprintf(buf, "$n %s", ch->player_specials->poofout);
	else
		strcpy(buf, "$n растворил$u в клубах дыма.");

	act(buf, true, ch, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
	ch->dismount();

	if (ch->player_specials->poofin)
		sprintf(buf, "$n %s", ch->player_specials->poofin);
	else
		strcpy(buf, "$n возник$q посреди комнаты.");
	act(buf, true, ch, nullptr, nullptr, kToRoom);
	look_at_room(ch, 0);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
