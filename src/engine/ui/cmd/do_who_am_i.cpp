/**
\file do_who_am_i.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "utils/native_text.h"
#include "gameplay/clans/house.h"
#include "engine/db/player_index.h"
#include "gameplay/core/remort.h"

void DoWhoAmI(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	time_t birt = ch->player_data.time.birth;
	SendMsgToChar(fmt::format("Персонаж : {}\r\n"
							  "Падежи : &W{}&n/&W{}&n/&W{}&n/&W{}&n/&W{}&n/&W{}&n\r\n"
							  "Ваш e-mail : &S{}&s\r\n"
							  "Дата вашего рождения : {}\r\n"
							  "Ваш IP-адрес : {}\r\n",
							  GET_NAME(ch),
							  ch->get_name(), GET_PAD(ch, 1), GET_PAD(ch, 2),
							  GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5),
							  GET_EMAIL(ch),
							  rustime(localtime(&birt)),
							  ch->desc ? ch->desc->host : "Unknown"), ch);
	if (!(ch)->player_specials->saved.NameGod) {
		SendMsgToChar("Имя никем не одобрено!\r\n", ch);
	} else {
		const int god_level = (ch)->player_specials->saved.NameGod > 1000 ? (ch)->player_specials->saved.NameGod - 1000 : (ch)->player_specials->saved.NameGod;
		std::string god_name = GetNameById((ch)->player_specials->saved.NameIDGod);
		native_text::capitalize_first(god_name);

		static const char *by_rank_god = "Богом";
		static const char *by_rank_privileged = "привилегированным игроком";
		const char *by_rank = god_level < kLvlImmortal ? by_rank_privileged : by_rank_god;

		// Строка о запрете собиралась в общий буфер и никуда не отправлялась -- игрок
		// видел одобрение, а про запрет не узнавал вовсе. Печатаем обе ветки.
		if ((ch)->player_specials->saved.NameGod < 1000)
			SendMsgToChar(fmt::format("&RИмя запрещено {} {}&n\r\n", by_rank, god_name), ch);
		else
			SendMsgToChar(fmt::format("&WИмя одобрено {} {}&n\r\n", by_rank, god_name), ch);
	}
	SendMsgToChar(fmt::format("Перевоплощений: {}\r\n", remort::GetRealRemort(ch)), ch);
	Clan::CheckPkList(ch);
	if (ch->player_specials->saved.telegram_id != 0) { //тут прямое обращение, ибо базовый класс, а не наследник
		SendMsgToChar(ch, "Подключен Телеграм, chat_id: %lu\r\n", ch->player_specials->saved.telegram_id);
	}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
