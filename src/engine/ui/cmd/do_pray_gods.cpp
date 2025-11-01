/**
\file do_pray_gods.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "gameplay/communication/remember.h"
#include "engine/core/handler.h"
#include "gameplay/fight/common.h"

void do_pray_gods(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	DescriptorData *i;
	CharData *victim = nullptr;

	skip_spaces(&argument);

	if (!ch->IsNpc() && (ch->IsFlagged(EPlrFlag::kDumbed) || ch->IsFlagged(EPlrFlag::kMuted))) {
		SendMsgToChar("Вам запрещено обращаться к Богам, вероятно, вы их замучили...\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch)) {
		// Выделяем чара кому отвечают иммы
		argument = one_argument(argument, arg1);
		skip_spaces(&argument);
		if (!*arg1) {
			SendMsgToChar("Какому смертному вы собираетесь ответить?\r\n", ch);
			return;
		}
		victim = get_player_vis(ch, arg1, EFind::kCharInWorld);
		if (victim == nullptr) {
			SendMsgToChar("Такого нет в игре!\r\n", ch);
			return;
		}
	}

	if (!*argument) {
		sprintf(buf, "С чем вы хотите обратиться к Богам?\r\n");
		SendMsgToChar(buf, ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar(OK, ch);
	else {
		if (ch->IsNpc())
			return;
		if (IS_IMMORTAL(ch)) {
			sprintf(buf, "&RВы одарили СЛОВОМ %s : '%s'&n\r\n", GET_PAD(victim, 3), argument);
		} else {
			sprintf(buf, "&RВы воззвали к Богам с сообщением : '%s'&n\r\n", argument);
			SetWait(ch, 3, false);
		}
		SendMsgToChar(buf, ch);
		ch->remember_add(buf, Remember::PRAY_PERSONAL);
	}

	if (IS_IMMORTAL(ch)) {
		sprintf(buf, "&R%s ответил%s вам : '%s'&n\r\n", GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		SendMsgToChar(buf, victim);
		victim->remember_add(buf, Remember::PRAY_PERSONAL);

		snprintf(buf1, kMaxStringLength, "&R%s ответил%s %s : '%s&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), GET_PAD(victim, 2), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, kMaxStringLength, "&R%s ответил%s на воззвание %s : '%s'&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), GET_PAD(victim, 1), argument);
	} else {
		snprintf(buf1, kMaxStringLength, "&R%s воззвал%s к богам : '%s&n\r\n",
				 GET_NAME(ch), GET_CH_SUF_1(ch), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, kMaxStringLength, "&R[%5d] %s воззвал%s к богам с сообщением : '%s'&n\r\n",
				 world[ch->in_room]->vnum, GET_NAME(ch), GET_CH_SUF_1(ch), argument);
	}

	for (i = descriptor_list; i; i = i->next) {
		if  (i->connected == CON_PLAYING) {
			if ((IS_IMMORTAL(i->character.get())
				|| (GET_GOD_FLAG(i->character.get(), EGf::kDemigod)
					&& (GetRealLevel(ch) < 6)))
				&& (i->character.get() != ch)) {
				SendMsgToChar(buf, i->character.get());
				i->character->remember_add(buf, Remember::ALL);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
