/**
\file do_gold.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/economics/currencies.h"
#include "utils/grammar/declensions.h"

void do_gold(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (currencies::GetAmount(*ch, currencies::kKunaId) == 0)
		SendMsgToChar("Вы разорены!\r\n", ch);
	else if (currencies::GetAmount(*ch, currencies::kKunaId) == 1)
		SendMsgToChar("У вас есть всего лишь одна куна.\r\n", ch);
	else {
		sprintf(buf, "У Вас есть %ld %s.\r\n", currencies::GetAmount(*ch, currencies::kKunaId), grammar::GetDeclensionInNumber(currencies::GetAmount(*ch, currencies::kKunaId), grammar::EWhat::kMoneyA));
		SendMsgToChar(buf, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
