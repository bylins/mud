/**
\file do_ban.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Команда бан. Должна использовать административную механику из ban.h
*/

#include "engine/ui/cmd_god/do_ban.h"

#include "administration/ban.h"
#include "engine/entities/char_data.h"

void do_ban(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!*argument) {
		ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
		return;
	}

	char flag[kMaxInputLength], site[kMaxInputLength];
	argument = two_arguments(argument, flag, site);

	if (!str_cmp(flag, "proxy")) {
		if (!*site) {
			ban->ShowBannedProxy(BanList::SORT_BY_NAME, ch);
			return;
		}
		if (site[0] == '-')
			switch (site[1]) {
				case 'n':
				case 'N': ban->ShowBannedProxy(BanList::SORT_BY_BANNER, ch);
					return;
				case 'i':
				case 'I': ban->ShowBannedProxy(BanList::SORT_BY_NAME, ch);
					return;
				default: SendMsgToChar("Usage: ban proxy [[-N | -I] | ip] \r\n", ch);
					SendMsgToChar(" -N : Sort by banner name \r\n", ch);
					SendMsgToChar(" -I : Sort by banned ip \r\n", ch);
					return;
			};
		std::string banned_ip(site);
		std::string banner_name(GET_NAME(ch));

		if (!ban->AddProxyBan(banned_ip, banner_name)) {
			SendMsgToChar("The site is already in the proxy ban list.\r\n", ch);
			return;
		}
		SendMsgToChar("Proxy banned.\r\n", ch);
		return;
	}

	if (!*site && flag[0] == '-')
		switch (flag[1]) {
			case 'n':
			case 'N': ban->ShowBannedIp(BanList::SORT_BY_BANNER, ch);
				return;
			case 'd':
			case 'D': ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
				return;
			case 'i':
			case 'I': ban->ShowBannedIp(BanList::SORT_BY_NAME, ch);
				return;
			default:;
		};

	if (!*flag || !*site) {
		SendMsgToChar("Usage: ban [[-N | -D | -I] | {all | select | new } ip duration [reason]] \r\n", ch);
		SendMsgToChar("or\r\n", ch);
		SendMsgToChar(" ban proxy [[-N | -I] | ip] \r\n", ch);
		SendMsgToChar(" -N : Sort by banner name \r\n", ch);
		SendMsgToChar(" -D : Sort by ban date \r\n", ch);
		SendMsgToChar(" -I : Sort by bannd ip \r\n", ch);
		return;
	}

	if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all")
		|| !str_cmp(flag, "new"))) {
		SendMsgToChar("Flag must be ALL, SELECT, or NEW.\r\n", ch);
		return;
	}

	char length[kMaxInputLength], *reason;
	int len, ban_type = BanList::BAN_ALL;
	reason = one_argument(argument, length);
	skip_spaces(&reason);
	len = atoi(length);
	if (!*length || len == 0) {
		SendMsgToChar("Usage: ban {all | select | new } ip duration [reason]\r\n", ch);
		return;
	}
	std::string banned_ip(site);
	std::string banner_name(GET_NAME(ch));
	std::string ban_reason(reason);
	for (int i = BanList::BAN_NEW; i <= BanList::BAN_ALL; i++)
		if (!str_cmp(flag, BanList::ban_types[i]))
			ban_type = i;

	if (!ban->AddBan(banned_ip, ban_reason, banner_name, len, ban_type)) {
		SendMsgToChar("That site has already been banned -- Unban it to change the ban type.\r\n", ch);
		return;
	}
	SendMsgToChar("Site banned.\r\n", ch);
}

void do_unban(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char site[kMaxInputLength];
	one_argument(argument, site);
	if (!*site) {
		SendMsgToChar("A site to Unban might help.\r\n", ch);
		return;
	}
	std::string unban_site(site);
	if (!ban->Unban(unban_site, ch)) {
		SendMsgToChar("The site is not currently banned.\r\n", ch);
		return;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
