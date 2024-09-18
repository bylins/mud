/**
\file do_ungroup.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/groups.h"


void do_ungroup(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	one_argument(argument, buf);
	if (ch->has_master() || !(AFF_FLAGGED(ch, EAffect::kGroup))) {
		SendMsgToChar("Вы же не лидер группы!\r\n", ch);
		return;
	}
	GoUngroup(ch, buf);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
