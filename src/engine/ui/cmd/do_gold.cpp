/**
\file do_gold.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/economics/currencies.h"
#include "engine/db/global_objects.h"
#include "utils/grammar/declensions.h"

void do_gold(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	const long amount = currencies::GetHand(*ch, currencies::kGold);
	if (amount == 0) {
		SendMsgToChar("Вы разорены!\r\n", ch);
	} else {
		sprintf(buf, "У Вас есть %s.\r\n",
			MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(amount, grammar::ECase::kNom).c_str());
		SendMsgToChar(buf, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
