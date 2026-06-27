/**
\file do_money.cpp - a part of the Bylins engine.
\authors Created by Claude.
\brief The "money"/"деньги" command: lists every currency the character holds
       as a table (currency / on-hand / in-bank), iterating the unified currency
       container. Base currencies (gold, glory) are also shown by "score"; this
       command shows the full list.
*/

#include "engine/entities/char_data.h"
#include "gameplay/economics/currencies.h"
#include "engine/db/global_objects.h"
#include "engine/ui/table_wrapper.h"
#include "utils/utils_string.h"

#include <sstream>

void do_money(CharData *ch, char * /*argument*/, int /*cmd*/, int /*subcmd*/) {
	const bool in_arena = ch->in_room != kNowhere
		&& ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena);

	table_wrapper::Table table;
	table << table_wrapper::kHeader << "Валюта" << "На руках" << "В лежне" << table_wrapper::kEndRow;

	std::size_t rows = 0;
	for (const auto &[id, amounts] : currencies::HeldByChar(*ch)) {
		if (amounts.hand <= 0 && amounts.bank <= 0) {
			continue;
		}
		const auto &info = MUD::Currencies().FindAvailableItem(id);
		if (info.GetId() < 0 || (info.IsArenaOnly() && !in_arena)) {
			continue;
		}
		table << utils::CAP(info.GetPluralName(grammar::ECase::kNom)) << amounts.hand << amounts.bank
			  << table_wrapper::kEndRow;
		++rows;
	}

	if (rows == 0) {
		SendMsgToChar("У вас нет ни гроша.\r\n", ch);
		return;
	}

	std::ostringstream out;
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(out, table);
	SendMsgToChar(out.str(), ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
