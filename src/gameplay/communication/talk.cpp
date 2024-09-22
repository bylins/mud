/**
\file talk.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Обмен репликами, разговор и все такое.
\detail Сюда нужно поместить весь код, связанный с теллми. реплиами в комнату и т.п.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"

void do_echo(CharData *ch, char *argument, int cmd, int subcmd);

// Симуляция телла от моба
void tell_to_char(CharData *keeper, CharData *ch, const char *argument) {
	char local_buf[kMaxInputLength];
	if (AFF_FLAGGED(ch, EAffect::kDeafness) || ch->IsFlagged(EPrf::kNoTell)) {
		sprintf(local_buf, "жестами показал$g на свой рот и уши. Ну его, болезного ..");
		do_echo(keeper, local_buf, 0, SCMD_EMOTE);
		return;
	}
	snprintf(local_buf, kMaxInputLength,
			 "%s сказал%s вам : '%s'", GET_NAME(keeper), GET_CH_SUF_1(keeper), argument);
	SendMsgToChar(ch, "%s%s%s\r\n",
				  kColorBoldCyn, CAP(local_buf), kColorNrm);
}

bool tell_can_see(CharData *ch, CharData *vict) {
	if (CAN_SEE_CHAR(vict, ch) || IS_IMMORTAL(ch) || GET_INVIS_LEV(ch)) {
		return true;
	} else {
		return false;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
