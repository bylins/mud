/**
\file do_goto.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/char_handler.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/sight.h"

void DoGoto(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	RoomRnum location;

	if ((location = FindRoomRnum(ch, argument, 0)) == kNowhere)
		return;

	act(POOFOUT(ch) ? fmt::format("$n {}", POOFOUT(ch)) : "$n растворил$u в клубах дыма.",
		true, ch, nullptr, nullptr, kToRoom);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, location);
	mount::Dismount(ch);

	act(POOFIN(ch) ? fmt::format("$n {}", POOFIN(ch)) : "$n возник$q посреди комнаты.",
		true, ch, nullptr, nullptr, kToRoom);
	sight::look_at_room(ch, 0);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
