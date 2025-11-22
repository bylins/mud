/**
\file talk.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Обмен репликами, разговор и все такое.
\detail Сюда нужно поместить весь код, связанный с теллми. реплиами в комнату и т.п.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "remember.h"

void do_echo(CharData *ch, char *argument, int cmd, int subcmd);

// Симуляция телла от моба
void tell_to_char(CharData *keeper, CharData *ch, const char *argument) {
	char local_buf[kMaxInputLength];
	if (AFF_FLAGGED(ch, EAffect::kDeafness) || ch->IsFlagged(EPrf::kNoTell)) {
		sprintf(local_buf, "жестами показал$g на свой рот и уши. Ну его, болезного ..");
		do_echo(keeper, local_buf, 0, kScmdEmote);
		return;
	}
	snprintf(local_buf, kMaxInputLength,
			 "%s сказал%s вам : '%s'", GET_NAME(keeper), GET_CH_SUF_1(keeper), argument);
	SendMsgToChar(ch, "%s%s%s\r\n",
				  kColorBoldCyn, utils::CAP(local_buf), kColorNrm);
}

bool tell_can_see(CharData *ch, CharData *vict) {
	if (CAN_SEE_CHAR(vict, ch) || IS_IMMORTAL(ch) || GET_INVIS_LEV(ch)) {
		return true;
	} else {
		return false;
	}
}

int is_tell_ok(CharData *ch, CharData *vict) {
	if (ch == vict) {
		SendMsgToChar("Вы начали потихоньку разговаривать с самим собой.\r\n", ch);
		return (false);
	} else if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам.\r\n", ch);
		return (false);
	} else if (!vict->IsNpc() && !vict->desc)    // linkless
	{
		act("$N потерял$G связь в этот момент.", false, ch, nullptr, vict, kToChar | kToSleep);
		return (false);
	} else if (vict->IsFlagged(EPlrFlag::kWriting)) {
		act("$N пишет сообщение - повторите попозже.", false, ch, nullptr, vict, kToChar | kToSleep);
		return (false);
	}

	if (IS_GOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
		return (true);

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kSoundproof))
		SendMsgToChar(SOUNDPROOF, ch);
	else if ((!vict->IsNpc() &&
		(vict->IsFlagged(EPrf::kNoTell) || ignores(vict, ch, EIgnore::kTell))) ||
		ROOM_FLAGGED(vict->in_room, ERoomFlag::kSoundproof)) {
		if (ch->IsNpc()) {
			if (ROOM_FLAGGED(vict->in_room, ERoomFlag::kSoundproof)) {
				log("NOTELL: name %s room %d soundproof", GET_NAME(vict), GET_ROOM_VNUM(vict->in_room));
			}
			if (vict->IsFlagged(EPrf::kNoTell))
				log("NOTELL: name %s глух как пробка", GET_NAME(vict));
		}
		act("$N не сможет вас услышать.", false, ch, nullptr, vict, kToChar | kToSleep);
	} else if (vict->GetPosition() < EPosition::kRest || AFF_FLAGGED(vict, EAffect::kDeafness))
		act("$N вас не услышит.", false, ch, nullptr, vict, kToChar | kToSleep);
	else
		return (true);

	return (false);
}

void perform_tell(CharData *ch, CharData *vict, char *arg) {
	if (vict->IsFlagged(EPrf::kNoInvistell)
		&& !CAN_SEE(vict, ch)
		&& GetRealLevel(ch) < kLvlImmortal
		&& !ch->IsFlagged(EPrf::kCoderinfo)) {
		act("$N не любит разговаривать с теми, кого не видит.", false, ch, nullptr, vict, kToChar | kToSleep);
		return;
	}

	// TODO: если в act() останется показ иммов, то это и эхо ниже переделать на act()
	if (tell_can_see(ch, vict)) {
		snprintf(buf, kMaxStringLength, "%s сказал%s вам : '%s'", GET_NAME(ch), GET_CH_SUF_1(ch), arg);
	} else {
		snprintf(buf, kMaxStringLength, "Кто-то сказал вам : '%s'", arg);
	}
	snprintf(buf1, kMaxStringLength, "%s%s%s\r\n", kColorBoldCyn, utils::CAP(buf), kColorNrm);
	SendMsgToChar(buf1, vict);
	if (!vict->IsNpc()) {
		vict->remember_add(buf1, Remember::ALL);
	}

	if (!vict->IsNpc() && !ch->IsNpc()) {
		snprintf(buf, kMaxStringLength, "%s%s : '%s'%s\r\n", kColorBoldCyn,
				 tell_can_see(ch, vict) ? GET_NAME(ch) : "Кто-то", arg, kColorNrm);
		vict->remember_add(buf, Remember::PERSONAL);
	}

	if (ch->IsFlagged(EPrf::kNoRepeat)) {
		SendMsgToChar(OK, ch);
	} else {
		snprintf(buf, kMaxStringLength, "%sВы сказали %s : '%s'%s\r\n", kColorBoldCyn,
				 tell_can_see(vict, ch) ? vict->player_data.PNames[ECase::kDat].c_str() : "кому-то", arg, kColorNrm);
		SendMsgToChar(buf, ch);
		if (!ch->IsNpc()) {
			ch->remember_add(buf, Remember::ALL);
		}
	}

	if (!vict->IsNpc() && !ch->IsNpc()) {
		vict->set_answer_id(ch->get_uid());
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
