//
// Created by Sventovit on 08.09.2024.
//

#include "do_wake.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

void do_wake(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	int self = 0;

	one_argument(argument, arg);

	if (subcmd == kScmdWakeUp) {
		if (!(*arg)) {
			SendMsgToChar("Кого будить то будем???\r\n", ch);
			return;
		}
	} else {
		*arg = 0;
	}

	if (*arg) {
		if (ch->GetPosition() == EPosition::kSleep)
			SendMsgToChar("Может быть вам лучше проснуться?\r\n", ch);
		else if ((vict = get_char_vis(ch, arg, EFind::kCharInRoom)) == nullptr)
			SendMsgToChar(NOPERSON, ch);
		else if (vict == ch)
			self = 1;
		else if (AWAKE(vict))
			act("$E и не спал$G.", false, ch, nullptr, vict, kToChar);
		else if (vict->GetPosition() < EPosition::kSleep)
			act("$M так плохо! Оставьте $S в покое!", false, ch, nullptr, vict, kToChar);
		else {
			act("Вы $S разбудили.", false, ch, nullptr, vict, kToChar);
			act("$n растолкал$g вас.", false, ch, nullptr, vict, kToVict | kToSleep);
			vict->SetPosition(EPosition::kSit);
		}
		if (!self)
			return;
	}
	if (AFF_FLAGGED(ch, EAffect::kSleep))
		SendMsgToChar("Вы не можете проснуться!\r\n", ch);
	else if (ch->GetPosition() > EPosition::kSleep)
		SendMsgToChar("А вы и не спали...\r\n", ch);
	else {
		SendMsgToChar("Вы проснулись и сели.\r\n", ch);
		act("$n проснул$u.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		ch->SetPosition(EPosition::kSit);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :