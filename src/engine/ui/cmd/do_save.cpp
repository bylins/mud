/**
\file do_save.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/alias.h"

void do_save(CharData *ch, char * /*argument*/, int cmd, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->desc) {
		return;
	}

	// Only tell the char we're saving if they actually typed "save"
	if (cmd) {
		SendMsgToChar("Сохраняю игрока, синонимы и вещи.\r\n", ch);
		SetWaitState(ch, 3 * kBattleRound);
	}
	write_aliases(ch);
	ch->save_char();
	Crash_crashsave(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
