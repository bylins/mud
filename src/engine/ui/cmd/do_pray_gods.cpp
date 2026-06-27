/**
\file do_pray_gods.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "utils/grammar/gender.h"
#include "engine/network/descriptor_data.h"
#include "gameplay/communication/remember.h"
#include "engine/core/target_resolver.h"
#include "gameplay/fight/common.h"
#include "utils/utils_string.h"

void do_pray_gods(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	DescriptorData *i;
	CharData *victim = nullptr;

	skip_spaces(&argument);

	if (!ch->IsNpc() && (ch->IsFlagged(EPlrFlag::kDumbed) || ch->IsFlagged(EPlrFlag::kMuted))) {
		SendMsgToChar("Вам запрещено обращаться к Богам, вероятно, вы их замучили...\r\n", ch);
		return;
	}

	if (privilege::IsImmortal(ch)) {
		// Выделяем чара кому отвечают иммы
		argument = one_argument(argument, arg1);
		skip_spaces(&argument);
		if (!*arg1) {
			SendMsgToChar("Какому смертному вы собираетесь ответить?\r\n", ch);
			return;
		}
		victim = target_resolver::FindPlayerVis(ch, arg1);
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
		SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
	else {
		if (ch->IsNpc())
			return;
		if (privilege::IsImmortal(ch)) {
			sprintf(buf, "&RВы одарили СЛОВОМ %s : '%s'&n\r\n", GET_PAD(victim, 3), argument);
		} else {
			sprintf(buf, "&RВы воззвали к Богам с сообщением : '%s'&n\r\n", argument);
			SetWait(ch, 3, false);
		}
		SendMsgToChar(buf, ch);
		ch->remember_add(buf, Remember::PRAY_PERSONAL);
	}

	if (privilege::IsImmortal(ch)) {
		sprintf(buf, "&R%s ответил%s вам : '%s'&n\r\n", GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), argument);
		SendMsgToChar(buf, victim);
		victim->remember_add(buf, Remember::PRAY_PERSONAL);

		snprintf(buf1, kMaxStringLength, "&R%s ответил%s %s : '%s&n\r\n",
				 GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), GET_PAD(victim, 2), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, kMaxStringLength, "&R%s ответил%s на воззвание %s : '%s'&n\r\n",
				 GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), GET_PAD(victim, 1), argument);
	} else {
		snprintf(buf1, kMaxStringLength, "&R%s воззвал%s к богам : '%s&n\r\n",
				 GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), argument);
		ch->remember_add(buf1, Remember::PRAY);

		snprintf(buf, kMaxStringLength, "&R[%5d] %s воззвал%s к богам с сообщением : '%s'&n\r\n",
				 world[ch->in_room]->vnum, GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), argument);
	}

	for (i = descriptor_list; i; i = i->next) {
		if  (i->state == EConState::kPlaying) {
			if ((privilege::IsImmortal(i->character.get())
				|| (GET_GOD_FLAG(i->character.get(), EGf::kDemigod)
					&& (GetRealLevel(ch) < 6)))
				&& (i->character.get() != ch)) {
				CharData *god = i->character.get();
				// движковая дата + перенос по словам под ширину экрана получателя.
				// Переносим всю собранную строку, чтобы в расчёт ширины попал и
				// префикс ("[комната] имя воззвал к богам ..."), и таймстамп.
				// Цветокоды OutWordsList уже не считает за ширину.
				// движковый префикс с датой и временем (клиентскую дату игроки убирают)
				char ts[32];
				const time_t now = time(nullptr);
				strftime(ts, sizeof(ts), "[%d-%m-%Y %H:%M] ", localtime(&now));
				std::string line = std::string(ts) + buf;
				if (!god->IsNpc() && god->player_specials->saved.stringLength > 0) {
					// WrapText съедает хвостовой \r\n -- возвращаем его обратно
					line = utils::WrapText(line, god->player_specials->saved.stringLength) + "\r\n";
				}
				SendMsgToChar(line.c_str(), god);
				god->remember_add(buf, Remember::ALL);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
