/**
\file do_group_say.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"
#include "utils/grammar/gender.h"
#include "gameplay/communication/remember.h"
#include "gameplay/core/constants.h"
#include "gameplay/communication/talk.h"
#include "gameplay/communication/ignores.h"

void do_gsay(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *k;

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(CommonMsg(ECommonMsg::kSilenced) + "\r\n", ch);
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

		const std::string to_group = fmt::format("$n сообщил$g группе : '{}'", argument);

		if (AFF_FLAGGED(k, EAffect::kGroup)
			&& k != ch
			&& !ignores(k, ch, EIgnore::kGroup)) {
			act(to_group, false, ch, nullptr, k, kToVict | kToSleep | kToNotDeaf);
			if (!AFF_FLAGGED(k, EAffect::kDeafness)
				&& k->GetPosition() > EPosition::kDead) {
				const std::string remembered =
					fmt::format("{} сообщил{} группе : '{}'\r\n",
								tell_can_see(ch, k) ? ch->get_name() : "Кто-то",
								grammar::VisSexEnding(sight::CanSee((k), (ch)), (ch)->get_sex(), 1),
								argument);
				k->remember_add(remembered, Remember::ALL);
				k->remember_add(remembered, Remember::GROUP);
			}
		}
		for (auto *f : k->followers) {
			if (AFF_FLAGGED(f, EAffect::kGroup)
				&& (f != ch)
				&& !ignores(f, ch, EIgnore::kGroup)) {
				act(to_group, false, ch, nullptr, f, kToVict | kToSleep | kToNotDeaf);
				if (!AFF_FLAGGED(f, EAffect::kDeafness)
					&& f->GetPosition() > EPosition::kDead) {
					const std::string remembered =
						fmt::format("{} сообщил{} группе : '{}'\r\n",
									tell_can_see(ch, f) ? ch->get_name() : "Кто-то",
									grammar::VisSexEnding(sight::CanSee((f), (ch)), (ch)->get_sex(), 1),
									argument);
					f->remember_add(remembered, Remember::ALL);
					f->remember_add(remembered, Remember::GROUP);
				}
			}
		}

		if (ch->IsFlagged(EPrf::kNoRepeat))
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
		else {
			const std::string echo = fmt::format("Вы сообщили группе : '{}'\r\n", argument);
			SendMsgToChar(echo, ch);
			ch->remember_add(echo, Remember::ALL);
			ch->remember_add(echo, Remember::GROUP);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
