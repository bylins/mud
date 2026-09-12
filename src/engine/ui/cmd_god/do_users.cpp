//
// Created by Sventovit on 22.02.2022.
//

#include "engine/ui/color.h"
#include <fmt/format.h>
#include "administration/privilege.h"
#include "gameplay/classes/pc_classes.h"
#include "engine/entities/char_data.h"
#include "engine/ui/modify.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/remort.h"
#include "gameplay/mechanics/sight.h"

#define USERS_FORMAT \
"Формат: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"
const int kMaxListLen = 200;
void do_users(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char idletime[10], classname[128];
	char state[30] = "\0", *timeptr, mode;
	char name_search[kMaxInputLength] = "\0", host_search[kMaxInputLength];
	char host_by_name[kMaxInputLength] = "\0";
	DescriptorData *list_players[kMaxListLen];
	DescriptorData *d_tmp;
	int count_pl;
	int cycle_i, is, flag_change;
	unsigned long a1, a2;
	int showremorts = 0, showemail = 0, locating = 0;
	char sorting = '!';
	DescriptorData *d;
	int low = 0, high = kLvlImplementator, num_can_see = 0;
	int outlaws = 0, playing = 0, deadweight = 0;
	ECharClass showclass{ECharClass::kUndefined};

	host_search[0] = name_search[0] = '\0';

	char rest[kMaxInputLength], option[kMaxInputLength], tail[kMaxInputLength];
	strl_cpy(rest, argument, sizeof(rest));
	while (*rest) {
		half_chop(rest, option, tail);
		if (*option == '-') {
			mode = *(option + 1);    // just in case; we destroy option in the switch
			switch (mode) {
				case 'o':
				case 'k': outlaws = 1;
					playing = 1;
					strl_cpy(rest, tail, sizeof(rest));
					break;
				case 'p': playing = 1;
					strl_cpy(rest, tail, sizeof(rest));
					break;
				case 'd': deadweight = 1;
					strl_cpy(rest, tail, sizeof(rest));
					break;
				case 'l':
					if (!privilege::IsGod(ch))
						return;
					playing = 1;
					half_chop(tail, option, rest);
					sscanf(option, "%d-%d", &low, &high);
					break;
				case 'n': playing = 1;
					half_chop(tail, name_search, rest);
					break;
				case 'h': playing = 1;
					half_chop(tail, host_search, rest);
					break;
				case 'u': playing = 1;
					half_chop(tail, host_by_name, rest);
					break;
				case 'w':
					if (!privilege::IsGrGod(ch))
						return;
					playing = 1;
					locating = 1;
					strl_cpy(rest, tail, sizeof(rest));
					break;
				case 'c': {
					playing = 1;
					half_chop(tail, option, rest);
/*					const size_t len = strlen(arg);
					for (size_t i = 0; i < len; i++) {
						showclass |= FindCharClassMask(arg[i]);
					}*/
					showclass = FindAvailableCharClassId(option);
					break;
				}
				case 'e': showemail = 1;
					strl_cpy(rest, tail, sizeof(rest));
					break;
				case 'r': showremorts = 1;
					strl_cpy(rest, tail, sizeof(rest));
					break;

				case 's':
					sorting = *(option + 2);
					strl_cpy(rest, tail, sizeof(rest));
					break;
				default: SendMsgToChar(USERS_FORMAT, ch);
					return;
			}    // end of switch

		} else {
			strl_cpy(name_search, option, sizeof(name_search));
			strl_cpy(rest, tail, sizeof(rest));
		}
	}            // end while (parser)

	// Ширина колонок - в символах, а не в байтах (issue #3681): поля ниже паддятся
// через native_text, поэтому формат содержит голые "%s".
	const char *format = "{:3} {:<7} {:<20} {:<17} {:<3} {:<8} ";
	std::string header = showemail
		? "Ном Професс    Имя                  Состояние         Idl Логин    Сайт       E-mail\r\n"
		: "Ном Професс    Имя                  Состояние         Idl Логин    Сайт\r\n";
	header += "--- ---------- -------------------- ----------------- --- -------- ----------------------------\r\n";
	SendMsgToChar(header, ch);

	if (strlen(host_by_name) != 0) {
		strcpy(host_search, "!");
	}

	for (d = descriptor_list, count_pl = 0; d && count_pl < kMaxListLen; d = d->next, count_pl++) {
		list_players[count_pl] = d;

		const auto character = d->get_character();
		if (!character) {
			continue;
		}

		if (isname(host_by_name, GET_NAME(character))) {
			strcpy(host_search, d->host);
		}
	}

	if (sorting != '!') {
		is = 1;
		while (is) {
			is = 0;
			for (cycle_i = 1; cycle_i < count_pl; cycle_i++) {
				flag_change = 0;
				d = list_players[cycle_i - 1];

				const auto t = d->get_character();

				d_tmp = list_players[cycle_i];

				const auto t_tmp = d_tmp->get_character();

				switch (sorting) {
					case 'n':
						if (0 < strcoll(t ? t->GetCharAliases().c_str() : "", t_tmp ? t_tmp->GetCharAliases().c_str() : "")) {
							flag_change = 1;
						}
						break;

					case 'e':
						if (strcoll(t ? GET_EMAIL(t) : "", t_tmp ? GET_EMAIL(t_tmp) : "") > 0)
							flag_change = 1;
						break;

					default: a1 = get_ip(const_cast<char *>(d->host));
						a2 = get_ip(const_cast<char *>(d_tmp->host));
						if (a1 > a2)
							flag_change = 1;
				}
				if (flag_change) {
					list_players[cycle_i - 1] = d_tmp;
					list_players[cycle_i] = d;
					is = 1;
				}
			}
		}
	}

	for (cycle_i = 0; cycle_i < count_pl; cycle_i++) {
		d = list_players[cycle_i];

		if (d->state != EConState::kPlaying && playing)
			continue;
		if (d->state == EConState::kPlaying && deadweight)
			continue;
		if (d->state == EConState::kPlaying) {
			const auto character = d->get_character();
			if (!character) {
				continue;
			}

			if (*host_search && !strstr(d->host, host_search)) {
				continue;
			}
			if (*name_search && !isname(name_search, GET_NAME(character))) {
				continue;
			}
			if (!sight::CanSee(ch, character) || GetRealLevel(character) < low || GetRealLevel(character) > high) {
				continue;
			}
			if (outlaws && !(ch)->IsFlagged(EPlrFlag::kKiller)) {
				continue;
			}
			if (showclass != ECharClass::kUndefined && showclass != character->GetClass()) {
				continue;
			}
			if (GET_INVIS_LEV(character) > GetRealLevel(ch)) {
				continue;
			}

			if (d->original) {
				if (showremorts) {
					sprintf(classname,
							"[%2d %2d %s]",
							GetRealLevel(d->original),
							remort::GetRealRemort(d->original),
							MUD::Class(d->original->GetClass()).GetAbbr().c_str());
				} else {
					sprintf(classname,
							"[%2d %s]   ",
							GetRealLevel(d->original),
							MUD::Class(d->original->GetClass()).GetAbbr().c_str());
				}
			} else if (showremorts) {
				sprintf(classname,
						"[%2d %2d %s]",
						GetRealLevel(d->character),
						remort::GetRealRemort(d->character),
						MUD::Class(d->character->GetClass()).GetAbbr().c_str());
			} else {
				sprintf(classname,
						"[%2d %s]   ",
						GetRealLevel(d->character),
						MUD::Class(d->character->GetClass()).GetAbbr().c_str());
			}
		} else {
			strcpy(classname, "      -      ");
		}

		if (GetRealLevel(ch) < kLvlImplementator && !ch->IsFlagged(EPrf::kCoderinfo)) {
			strcpy(classname, "      -      ");
		}

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (d->state == EConState::kPlaying && d->original) {
			strcpy(state, "Switched");
		} else {
			strcpy(state, GetConDescription(d->state));
		}

		if (d->character
			&& d->state == EConState::kPlaying
			&& !privilege::IsGod(d->character.get())) {
			sprintf(idletime, "%-3d", d->character->char_specials.timer *
				kSecsPerMudHour / kSecsPerRealMin);
		} else {
			strcpy(idletime, "   ");
		}

		std::string line;
		if (d->character) {
			line = fmt::format(fmt::runtime(format), d->desc_num, classname,
							   d->original ? d->original->GetCharAliases() : d->character->GetCharAliases(),
							   state, idletime, timeptr);
		} else {
			line = fmt::format(fmt::runtime(format), d->desc_num, "   -   ", "UNDEFINED",
							   state, idletime, timeptr);
		}

		if (*d->host) {
			line += fmt::format("[{}]", d->host);
		} else {
			line += "[Неизвестный хост]";
		}

		if (showemail) {
			line += fmt::format("[&S{}&s]",
								d->original ? GET_EMAIL(d->original) : d->character ? GET_EMAIL(d->character) : "");
		}

		// Комната ищется только по ключу -w с именем или хостом. Раньше строка с комнатой
		// приклеивалась и тогда, когда её не собрали, -- в вывод попадал прошлый кусок буфера.
		if (locating && (*name_search || *host_by_name) && d->state == EConState::kPlaying) {
			const auto ci = d->get_character();
			if (ci && sight::CanSee(ch, ci) && ci->in_room != kNowhere) {
				// имя комнаты бывает нулевым (конструктор RoomData), а fmt на нуле бросает исключение
				const char *room_name = world[ci->in_room]->name ? world[ci->in_room]->name : "";
				if (d->original && d->character) {
					line += fmt::format(" [{:7}] {} (in {})",
										GET_ROOM_VNUM(d->character->in_room), room_name, GET_NAME(d->character));
				} else {
					line += fmt::format(" [{:7}] {}", GET_ROOM_VNUM(ci->in_room), room_name);
				}
			}
		}

		line += "\r\n";
		if (d->state != EConState::kPlaying) {
			line = fmt::format("&g{}&n", line);
		}

		if (d->state != EConState::kPlaying || (d->state == EConState::kPlaying && d->character && sight::CanSee(ch, d->character))) {
			SendMsgToChar(line, ch);
			num_can_see++;
		}
	}

	page_string(ch->desc, fmt::format("\r\n{} видимых соединений.\r\n", num_can_see));
}