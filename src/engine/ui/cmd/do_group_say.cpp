/**
\file do_group_say.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/communication/remember.h"
#include "gameplay/core/constants.h"
#include "gameplay/communication/talk.h"
#include "gameplay/communication/ignores.h"

void do_gsay(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *k;
	struct FollowerType *f;

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	if (!AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Вы не являетесь членом группы!\r\n", ch);
		return;
	}

	skip_spaces(&argument);
	if (!*argument) {
		SendMsgToChar("О чем вы хотите сообщить своей группе?\r\n", ch);
	} else {
		if (ch->has_master()) {
			k = ch->get_master();
		} else {
			k = ch;
		}

		sprintf(buf, "$n сообщил$g группе : '%s'", argument);

		if (AFF_FLAGGED(k, EAffect::kGroup)
			&& k != ch
			&& !ignores(k, ch, EIgnore::kGroup)) {
			act(buf, false, ch, nullptr, k, kToVict | kToSleep | kToNotDeaf);
			if (!AFF_FLAGGED(k, EAffect::kDeafness)
				&& k->GetPosition() > EPosition::kDead) {
				sprintf(buf1,
						"%s сообщил%s группе : '%s'\r\n",
						tell_can_see(ch, k) ? ch->get_name().c_str() : "Кто-то",
						GET_CH_VIS_SUF_1(ch, k),
						argument);
				k->remember_add(buf1, Remember::ALL);
				k->remember_add(buf1, Remember::GROUP);
			}
		}
		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->follower, EAffect::kGroup)
				&& (f->follower != ch)
				&& !ignores(f->follower, ch, EIgnore::kGroup)) {
				act(buf, false, ch, nullptr, f->follower, kToVict | kToSleep | kToNotDeaf);
				if (!AFF_FLAGGED(f->follower, EAffect::kDeafness)
					&& f->follower->GetPosition() > EPosition::kDead) {
					sprintf(buf1,
							"%s сообщил%s группе : '%s'\r\n",
							tell_can_see(ch, f->follower) ? ch->get_name().c_str() : "Кто-то",
							GET_CH_VIS_SUF_1(ch, f->follower),
							argument);
					f->follower->remember_add(buf1, Remember::ALL);
					f->follower->remember_add(buf1, Remember::GROUP);
				}
			}
		}

		if (ch->IsFlagged(EPrf::kNoRepeat))
			SendMsgToChar(OK, ch);
		else {
			sprintf(buf, "Вы сообщили группе : '%s'\r\n", argument);
			SendMsgToChar(buf, ch);
			ch->remember_add(buf, Remember::ALL);
			ch->remember_add(buf, Remember::GROUP);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
