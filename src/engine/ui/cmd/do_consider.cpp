/**
\file consider.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"

void DoConsider(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	int diff;

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, EFind::kCharInRoom))) {
		SendMsgToChar("Кого вы хотите оценить?\r\n", ch);
		return;
	}
	if (victim == ch) {
		SendMsgToChar("Легко! Выберите параметр <Удалить персонаж>!\r\n", ch);
		return;
	}
	if (!victim->IsNpc()) {
		SendMsgToChar("Оценивайте игроков сами - тут я не советчик.\r\n", ch);
		return;
	}
	diff = (GetRealLevel(victim) - GetRealLevel(ch) - GetRealRemort(ch));

	if (diff <= -10)
		SendMsgToChar("Ути-пути, моя рыбонька.\r\n", ch);
	else if (diff <= -5)
		SendMsgToChar("\"Сделаем без шуму и пыли!\"\r\n", ch);
	else if (diff <= -2)
		SendMsgToChar("Легко.\r\n", ch);
	else if (diff <= -1)
		SendMsgToChar("Сравнительно легко.\r\n", ch);
	else if (diff == 0)
		SendMsgToChar("Равный поединок!\r\n", ch);
	else if (diff <= 1)
		SendMsgToChar("Вам понадобится немного удачи!\r\n", ch);
	else if (diff <= 2)
		SendMsgToChar("Вам потребуется везение!\r\n", ch);
	else if (diff <= 3)
		SendMsgToChar("Удача и хорошее снаряжение вам сильно пригодятся!\r\n", ch);
	else if (diff <= 5)
		SendMsgToChar("Вы берете на себя слишком много.\r\n", ch);
	else if (diff <= 10)
		SendMsgToChar("Ладно, войдете еще раз.\r\n", ch);
	else if (diff <= 100)
		SendMsgToChar("Срочно к психиатру - вы страдаете манией величия!\r\n", ch);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
