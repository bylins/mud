/**
\file do_spec_comm.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "engine/core/target_resolver.h"

// \todo Аналогично - распилить на отдельные команды, механику посыла сообщения убрать в communication
void do_spec_comm(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	const char *action_sing, *action_plur, *action_others, *vict1, *vict2;
	char vict3[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(CommonMsg(ECommonMsg::kSilenced) + "\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (subcmd == kScmdWhisper) {
		action_sing = "шепнуть";
		vict1 = "кому";
		vict2 = "вам";
		action_plur = "прошептал";
		action_others = "$n что-то прошептал$g $N2.";
	} else {
		action_sing = "спросить";
		vict1 = "у кого";
		vict2 = "у вас";
		action_plur = "спросил";
		action_others = "$n задал$g $N2 вопрос.";
	}

	char name[kMaxInputLength];
	char message[kMaxStringLength];
	half_chop(argument, name, message);

	if (!*name || !*message) {
		SendMsgToChar(fmt::format("Что вы хотите {}.. и {}?\r\n", action_sing, vict1), ch);
	} else if (!(vict = target_resolver::FindCharInRoom(ch, name)))
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
	else if (vict == ch)
		SendMsgToChar("От ваших уст до ушей - всего одна ладонь...\r\n", ch);
	else if (ignores(vict, ch, subcmd == kScmdWhisper ? EIgnore::kWhisper : EIgnore::kAsk)) {
		SendMsgToChar(fmt::format("{} не желает вас слышать.\r\n", GET_NAME(vict)), ch);
	} else {
		if (subcmd == kScmdWhisper)
			sprintf(vict3, "%s", GET_PAD(vict, 2));
		else
			sprintf(vict3, "у %s", GET_PAD(vict, 1));

		act(fmt::format("$n {}$g {} : {}", action_plur, vict2, message),
			false, ch, nullptr, vict, kToVict | kToNotDeaf);

		if (ch->IsFlagged(EPrf::kNoRepeat))
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
		else {
			SendMsgToChar(fmt::format("Вы {}и {} : '{}'\r\n", action_plur, vict3, message), ch);
		}

		act(action_others, false, ch, nullptr, vict, kToNotVict);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
