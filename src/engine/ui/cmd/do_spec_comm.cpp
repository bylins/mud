/**
\file do_spec_comm.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

// \todo Аналогично - распилить на отдельные команды, механику посыла сообщения убрать в communication
void do_spec_comm(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	const char *action_sing, *action_plur, *action_others, *vict1, *vict2;
	char vict3[kMaxInputLength];

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_WHISPER) {
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

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2) {
		sprintf(buf, "Что вы хотите %s.. и %s?\r\n", action_sing, vict1);
		SendMsgToChar(buf, ch);
	} else if (!(vict = get_char_vis(ch, buf, EFind::kCharInRoom)))
		SendMsgToChar(NOPERSON, ch);
	else if (vict == ch)
		SendMsgToChar("От ваших уст до ушей - всего одна ладонь...\r\n", ch);
	else if (ignores(vict, ch, subcmd == SCMD_WHISPER ? EIgnore::kWhisper : EIgnore::kAsk)) {
		sprintf(buf, "%s не желает вас слышать.\r\n", GET_NAME(vict));
		SendMsgToChar(buf, ch);
	} else {
		if (subcmd == SCMD_WHISPER)
			sprintf(vict3, "%s", GET_PAD(vict, 2));
		else
			sprintf(vict3, "у %s", GET_PAD(vict, 1));

		std::stringstream buffer;
		buffer << "$n " << action_plur << "$g " << vict2 << " : " << buf2;
//		sprintf(buf, "$n %s$g %s : '%s'", action_plur, vict2, buf2);
		act(buffer.str().c_str(), false, ch, nullptr, vict, kToVict | kToNotDeaf);

		if (ch->IsFlagged(EPrf::kNoRepeat))
			SendMsgToChar(OK, ch);
		else {
			std::stringstream buffer;
			buffer << "Вы " << action_plur << "и " << vict3 << " : '" << buf2 << "'" << "\r\n";
//			sprintf(buf, "Вы %sи %s : '%s'\r\n", action_plur, vict3, buf2);
			SendMsgToChar(buffer.str(), ch);
		}

		act(action_others, false, ch, nullptr, vict, kToNotVict);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
