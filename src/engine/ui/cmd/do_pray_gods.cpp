/**
\file do_pray_gods.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

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
		SendMsgToChar("С чем вы хотите обратиться к Богам?\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kNoRepeat))
		SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
	else {
		if (ch->IsNpc())
			return;
		std::string echo;
		if (privilege::IsImmortal(ch)) {
			echo = fmt::format("&RВы одарили СЛОВОМ {} : '{}'&n\r\n", GET_PAD(victim, 3), argument);
		} else {
			echo = fmt::format("&RВы воззвали к Богам с сообщением : '{}'&n\r\n", argument);
			SetWait(ch, 3, false);
		}
		SendMsgToChar(echo, ch);
		ch->remember_add(echo, Remember::PRAY_PERSONAL);
	}

	std::string to_gods;
	if (privilege::IsImmortal(ch)) {
		const std::string to_victim =
			fmt::format("&R{} ответил{} вам : '{}'&n\r\n",
						GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), argument);
		SendMsgToChar(to_victim, victim);
		victim->remember_add(to_victim, Remember::PRAY_PERSONAL);

		ch->remember_add(fmt::format("&R{} ответил{} {} : '{}&n\r\n",
									 GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1),
									 GET_PAD(victim, 2), argument),
						 Remember::PRAY);

		to_gods = fmt::format("&R{} ответил{} на воззвание {} : '{}'&n\r\n",
							  GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1),
							  GET_PAD(victim, 1), argument);
	} else {
		ch->remember_add(fmt::format("&R{} воззвал{} к богам : '{}&n\r\n",
									 GET_NAME(ch), grammar::SexEnding((ch)->get_sex(), 1), argument),
						 Remember::PRAY);

		to_gods = fmt::format("&R[{:7}] {} воззвал{} к богам с сообщением : '{}'&n\r\n",
							  world[ch->in_room]->vnum, GET_NAME(ch),
							  grammar::SexEnding((ch)->get_sex(), 1), argument);
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
				std::string line = std::string(ts) + to_gods;
				if (!god->IsNpc() && god->player_specials->saved.stringLength > 0) {
					// WrapText съедает хвостовой \r\n -- возвращаем его обратно
					line = utils::WrapText(line, god->player_specials->saved.stringLength) + "\r\n";
				}
				SendMsgToChar(line.c_str(), god);
				god->remember_add(to_gods, Remember::ALL);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
