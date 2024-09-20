/**
\file do_antigods.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"

void do_antigods(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_IMMORTAL(ch)) {
		SendMsgToChar("Оно вам надо?\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kGodsShield)) {
		if (IsAffectedBySpell(ch, ESpell::kGodsShield)) {
			RemoveAffectFromChar(ch, ESpell::kGodsShield);
		}
		AFF_FLAGS(ch).unset(EAffect::kGodsShield);
		SendMsgToChar("Голубой кокон вокруг вашего тела угас.\r\n", ch);
		act("&W$n отринул$g защиту, дарованную богами.&n", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
	} else {
		SendMsgToChar("Вы и так не под защитой богов.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
