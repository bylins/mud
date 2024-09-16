/**
\file logon.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief description.
*/

#include "engine/network/logon.h"

#include "administration/accounts.h"
#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"

namespace network {

void add_logon_record(DescriptorData *d) {
	// Добавляем запись в LOG_LIST
	d->character->get_account()->add_login(std::string(d->host));

	const auto logon = std::find_if(LOGON_LIST(d->character).begin(), LOGON_LIST(d->character).end(),
									[&](const Logon &l) -> bool {
									  return !strcmp(l.ip, d->host);
									});

	if (logon == LOGON_LIST(d->character).end()) {
		const Logon cur_log = {str_dup(d->host), 1, time(nullptr), false};
		LOGON_LIST(d->character).push_back(cur_log);
	} else {
		++logon->count;
		logon->lasttime = time(nullptr);
	}

	int pos = d->character->get_pfilepos();
	if (pos >= 0) {
		if (player_table[pos].last_ip)
			free(player_table[pos].last_ip);
		player_table[pos].last_ip = str_dup(d->host);
		player_table[pos].last_logon = LAST_LOGON(d->character);
	}
}

} // namespace network

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
