/**
\file do_wizlock.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "utils/grammar/declensions.h"

extern int circle_restrict;

void DoWizlock(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int value;
	const char *when;

	char value_arg[kMaxInputLength];
	one_argument(argument, value_arg);
	if (*value_arg) {
		value = atoi(value_arg);
		if (value > kLvlImplementator)
			value = kLvlImplementator; // 34е всегда должны иметь возможность зайти
		if (value < 0 || (value > GetRealLevel(ch) && !ch->IsFlagged(EPrf::kCoderinfo))) {
			SendMsgToChar("Неверное значение для wizlock.\r\n", ch);
			return;
		}
		circle_restrict = value;
		when = "теперь";
	} else
		when = "в настоящее время";

	std::string message;
	switch (circle_restrict) {
		case 0: message = fmt::format("Игра {} полностью открыта.\r\n", when);
			break;
		case 1: message = fmt::format("Игра {} закрыта для новых игроков.\r\n", when);
			break;
		default:
			message = fmt::format("Только игроки {} {} и выше могут {} войти в игру.\r\n",
								  circle_restrict, grammar::GetDeclensionInNumber(circle_restrict, grammar::EWhat::kLvl), when);
			break;
	}
	SendMsgToChar(message, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
