/**
\file do_beep.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/target_resolver.h"
#include "engine/ui/color.h"

void perform_beep(CharData *ch, CharData *vict);

void do_beep(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict = nullptr;

	char name[kMaxInputLength];
	one_argument(argument, name);

	if (!*name)
		SendMsgToChar("Кого вызывать?\r\n", ch);
	else if (!(vict = target_resolver::FindCharInWorld(ch, name)) || vict->IsNpc())
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	else if (ch == vict)
		SendMsgToChar("\007\007Вы вызвали себя!\r\n", ch);
	else if (ch->IsFlagged(EPrf::kNoTell))
		SendMsgToChar("Вы не можете пищать в режиме обращения.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof))
		SendMsgToChar("Стены заглушили ваш писк.\r\n", ch);
	else if (!vict->IsNpc() && !vict->desc)    // linkless
		act("$N потерял связь.", false, ch, nullptr, vict, kToChar | kToSleep);
	else if (vict->IsFlagged(EPlrFlag::kWriting))
		act("$N пишет сейчас; Попищите позже.", false, ch, nullptr, vict, kToChar | kToSleep);
	else
		perform_beep(ch, vict);
}

void perform_beep(CharData *ch, CharData *vict) {
	SendMsgToChar(kColorRed, vict);
	act("\007\007 $n вызывает вас!", false, ch, nullptr, vict, kToVict | kToSleep);
	SendMsgToChar(kColorNrm, vict);

	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
	else {
		SendMsgToChar(kColorRed, ch);
		act("Вы вызвали $N3.", false, ch, nullptr, vict, kToChar | kToSleep);
		SendMsgToChar(kColorNrm, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
