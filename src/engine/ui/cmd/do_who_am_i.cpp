/**
\file do_who_am_i.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"

void DoWhoAmI(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	sprintf(buf, "Персонаж : %s\r\n", GET_NAME(ch));
	sprintf(buf + strlen(buf),
			"Падежи : &W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n/&W%s&n\r\n",
			ch->get_name().c_str(), GET_PAD(ch, 1), GET_PAD(ch, 2),
			GET_PAD(ch, 3), GET_PAD(ch, 4), GET_PAD(ch, 5));

	sprintf(buf + strlen(buf), "Ваш e-mail : &S%s&s\r\n", GET_EMAIL(ch));
	time_t birt = ch->player_data.time.birth;
	sprintf(buf + strlen(buf), "Дата вашего рождения : %s\r\n", rustime(localtime(&birt)));
	sprintf(buf + strlen(buf), "Ваш IP-адрес : %s\r\n", ch->desc ? ch->desc->host : "Unknown");
	SendMsgToChar(buf, ch);
	if (!NAME_GOD(ch)) {
		sprintf(buf, "Имя никем не одобрено!\r\n");
		SendMsgToChar(buf, ch);
	} else {
		const int god_level = NAME_GOD(ch) > 1000 ? NAME_GOD(ch) - 1000 : NAME_GOD(ch);
		sprintf(buf1, "%s", GetNameById(NAME_ID_GOD(ch)));
		*buf1 = UPPER(*buf1);

		static const char *by_rank_god = "Богом";
		static const char *by_rank_privileged = "привилегированным игроком";
		const char *by_rank = god_level < kLvlImmortal ? by_rank_privileged : by_rank_god;

		if (NAME_GOD(ch) < 1000)
			snprintf(buf, kMaxStringLength, "&RИмя запрещено %s %s&n\r\n", by_rank, buf1);
		else
			snprintf(buf, kMaxStringLength, "&WИмя одобрено %s %s&n\r\n", by_rank, buf1);
		SendMsgToChar(buf, ch);
	}
	sprintf(buf, "Перевоплощений: %d\r\n", GetRealRemort(ch));
	SendMsgToChar(buf, ch);
	Clan::CheckPkList(ch);
	if (ch->player_specials->saved.telegram_id != 0) { //тут прямое обращение, ибо базовый класс, а не наследник
		SendMsgToChar(ch, "Подключен Телеграм, chat_id: %lu\r\n", ch->player_specials->saved.telegram_id);
	}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
