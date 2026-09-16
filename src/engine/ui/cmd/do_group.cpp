/**
\file do_group.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/groups.h"

void do_group(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char mode[kMaxInputLength];
	argument = one_argument(argument, mode);

	if (!*mode) {
		group::print_group(ch);
		return;
	}

	if (!str_cmp(mode, "список")) {
		group::print_list_group(ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kRest) {
		SendMsgToChar("Трудно управлять группой в таком состоянии.\r\n", ch);
		return;
	}

	if (ch->has_master()) {
		act("Вы не можете управлять группой. Вы еще не ведущий.", false, ch, nullptr, nullptr, kToChar);
		return;
	}

	if (ch->followers.empty()) {
		SendMsgToChar("За вами никто не следует.\r\n", ch);
		return;
	}

	group::GoGroup(ch, mode, argument);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
