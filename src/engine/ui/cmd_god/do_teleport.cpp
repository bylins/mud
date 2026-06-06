/**
\file do_teleport.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/sight.h"

void DoTeleport(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	RoomRnum target;

	two_arguments(argument, buf, buf2);

	if (!*buf)
		SendMsgToChar("Кого вы хотите переместить?\r\n", ch);
	else if (!(victim = get_char_vis(ch, buf, EFind::kCharInWorld)))
		SendMsgToChar(NOPERSON, ch);
	else if (victim == ch)
		SendMsgToChar("Используйте 'прыжок' для собственного перемещения.\r\n", ch);
	else if (GetRealLevel(victim) >= GetRealLevel(ch) && !ch->IsFlagged(EPrf::kCoderinfo))
		SendMsgToChar("Попробуйте придумать что-то другое.\r\n", ch);
	else if (!*buf2)
		act("Куда вы хотите $S переместить?", false, ch, nullptr, victim, kToChar);
	else if ((target = FindRoomRnum(ch, buf2, 0)) != kNowhere) {
		SendMsgToChar(OK, ch);
		act("$n растворил$u в клубах дыма.", false, victim, nullptr, nullptr, kToRoom);
		RemoveCharFromRoom(victim);
		PlaceCharToRoom(victim, target);
		victim->dismount();
		act("$n появил$u, окутанн$w розовым туманом.", false, victim, nullptr, nullptr, kToRoom);
		act("$n переместил$g вас!", false, ch, nullptr, (char *) victim, kToVict);
		look_at_room(victim, 0);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
