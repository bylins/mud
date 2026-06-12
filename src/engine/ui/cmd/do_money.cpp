/**
\file do_money.cpp - a part of the Bylins engine.
\authors Created by Claude.
\brief The "money"/"деньги" command: lists every currency the character holds,
       iterating the unified currency container. Base currencies (gold, glory)
       are shown by "score"; this command shows the full list.
*/

#include "engine/entities/char_data.h"
#include "gameplay/economics/currencies.h"
#include "engine/db/global_objects.h"
#include "utils/grammar/declensions.h"

#include <sstream>

void do_money(CharData *ch, char * /*argument*/, int /*cmd*/, int /*subcmd*/) {
	const bool in_arena = ch->in_room != kNowhere
		&& ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena);

	std::ostringstream out;
	for (const auto &[id, amounts] : ch->currency_storage().data()) {
		if (amounts.hand <= 0 && amounts.bank <= 0) {
			continue;
		}
		const auto &info = MUD::Currencies().FindAvailableItem(id);
		if (info.GetId() < 0 || (info.IsArenaOnly() && !in_arena)) {
			continue;
		}
		if (amounts.hand > 0) {
			out << amounts.hand << " " << info.GetNameWithAmount(amounts.hand, grammar::ECase::kNom) << " на руках.\r\n";
		}
		if (amounts.bank > 0) {
			out << amounts.bank << " " << info.GetNameWithAmount(amounts.bank, grammar::ECase::kNom) << " в лежне.\r\n";
		}
	}

	const std::string result = out.str();
	if (result.empty()) {
		SendMsgToChar("У вас нет ни гроша.\r\n", ch);
	} else {
		SendMsgToChar(result, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
