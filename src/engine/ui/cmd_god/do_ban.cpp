/**
\file do_ban.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Команда бан. Должна использовать административную механику из ban.h
*/

#include "engine/ui/cmd_god/do_ban.h"

#include "administration/ban.h"
#include "engine/entities/char_data.h"

namespace {

// Что команда умеет. Печатается на пустой вызов и на непонятый ввод: раньше формат не
// показывался вовсе, пока игрок не угадает полуправильный набор слов.
void ShowBanUsage(CharData *ch) {
	SendMsgToChar("&WЗапреты по адресу.&n Формат:\r\n"
				  "  запрет                             -- список запретов (он же: запрет список)\r\n"
				  "  запрет -n | -d | -i                -- он же, по богу, по дате, по адресу\r\n"
				  "  запрет <адрес>                     -- запреты, начинающиеся с этого адреса\r\n"
				  "  запрет <all|select|new> <адрес> <часов> [причина]   -- поставить запрет\r\n"
				  "  запрет прокси                      -- список запретов на прокси\r\n"
				  "  запрет прокси <адрес>              -- запретить прокси\r\n"
				  "Виды: &Gnew&n -- нельзя создать нового героя, &Gselect&n -- только с меткой site-ok,\r\n"
				  "      &Gall&n -- вход закрыт всем.\r\n"
				  "Снять запрет: &Wunban <адрес>&n. То же, что \"запрет <адрес>\", даёт &Wshow ban <адрес>&n.\r\n", ch);
}

// Слова, которые команда понимает как указание вида запрета, а не как адрес.
bool IsBanTypeWord(const char *word) {
	return !str_cmp(word, "all") || !str_cmp(word, "select") || !str_cmp(word, "new");
}

}  // namespace

void do_ban(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!*argument) {
		ShowBanUsage(ch);
		ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
		return;
	}

	char flag[kMaxInputLength], site[kMaxInputLength];
	argument = two_arguments(argument, flag, site);

	// "запрет список" -- то, что бог набирает первым делом; раньше это слово уходило в разбор
	// вида запрета и отвечало "Flag must be ALL, SELECT, or NEW".
	if (!str_cmp(flag, "список") || !str_cmp(flag, "list")) {
		ban->ShowBannedIp(BanList::SORT_BY_DATE, ch);
		return;
	}

	if (!str_cmp(flag, "proxy") || !str_cmp(flag, "прокси")) {
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
				default: SendMsgToChar("Формат: запрет прокси [-n | -i | <адрес>]\r\n"
									   "  -n : по имени бога\r\n"
									   "  -i : по адресу\r\n", ch);
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

	// Один аргумент, и он не вид запрета -- значит адрес: показываем, что по нему есть. Это
	// ровно то, что делает "show ban <адрес>"; раньше такой ввод отвечал только форматом.
	if (*flag && !*site && flag[0] != '-' && !IsBanTypeWord(flag)) {
		ban->ShowBannedIpByMask(BanList::SORT_BY_DATE, ch, flag);
		return;
	}

	if (!*flag || !*site) {
		ShowBanUsage(ch);
		return;
	}

	if (!IsBanTypeWord(flag)) {
		SendMsgToChar("Вид запрета бывает all, select или new.\r\n", ch);
		ShowBanUsage(ch);
		return;
	}

	char length[kMaxInputLength], *reason;
	int len, ban_type = BanList::BAN_ALL;
	reason = one_argument(argument, length);
	skip_spaces(&reason);
	len = atoi(length);
	if (!*length || len == 0) {
		SendMsgToChar("Сколько часов держать запрет?\r\n", ch);
		ShowBanUsage(ch);
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
