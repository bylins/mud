//
// Created by Sventovit on 22.02.2022.
//

#include "color.h"
#include "game_classes/classes.h"
#include "entities/char_data.h"
#include "modify.h"
#include "structs/global_objects.h"
/*#include "utils/utils_string.h"
#include "utils/table_wrapper.h"
#include "utils/utils.h"*/

#define USERS_FORMAT \
"Формат: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"
const int kMaxListLen = 200;
void do_users(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char line[200], line2[220], idletime[10], classname[128];
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

	strcpy(buf, argument);
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (*arg == '-') {
			mode = *(arg + 1);    // just in case; we destroy arg in the switch
			switch (mode) {
				case 'o':
				case 'k': outlaws = 1;
					playing = 1;
					strcpy(buf, buf1);
					break;
				case 'p': playing = 1;
					strcpy(buf, buf1);
					break;
				case 'd': deadweight = 1;
					strcpy(buf, buf1);
					break;
				case 'l':
					if (!IS_GOD(ch))
						return;
					playing = 1;
					half_chop(buf1, arg, buf);
					sscanf(arg, "%d-%d", &low, &high);
					break;
				case 'n': playing = 1;
					half_chop(buf1, name_search, buf);
					break;
				case 'h': playing = 1;
					half_chop(buf1, host_search, buf);
					break;
				case 'u': playing = 1;
					half_chop(buf1, host_by_name, buf);
					break;
				case 'w':
					if (!IS_GRGOD(ch))
						return;
					playing = 1;
					locating = 1;
					strcpy(buf, buf1);
					break;
				case 'c': {
					playing = 1;
					half_chop(buf1, arg, buf);
/*					const size_t len = strlen(arg);
					for (size_t i = 0; i < len; i++) {
						showclass |= FindCharClassMask(arg[i]);
					}*/
					showclass = FindAvailableCharClassId(arg);
					break;
				}
				case 'e': showemail = 1;
					strcpy(buf, buf1);
					break;
				case 'r': showremorts = 1;
					strcpy(buf, buf1);
					break;

				case 's':
					sorting = *(arg + 2);
					strcpy(buf, buf1);
					break;
				default: send_to_char(USERS_FORMAT, ch);
					return;
			}    // end of switch

		} else {
			strcpy(name_search, arg);
			strcpy(buf, buf1);
		}
	}            // end while (parser)

	const char *format = "%3d %-7s %-20s %-17s %-3s %-8s ";
	if (showemail) {
		strcpy(line, "Ном Професс    Имя                  Состояние         Idl Логин    Сайт       E-mail\r\n");
	} else {
		strcpy(line, "Ном Професс    Имя                  Состояние         Idl Логин    Сайт\r\n");
	}
	strcat(line, "--- ---------- -------------------- ----------------- --- -------- ----------------------------\r\n");
	send_to_char(line, ch);

	one_argument(argument, arg);

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
						if (0 < strcoll(t ? t->get_pc_name().c_str() : "", t_tmp ? t_tmp->get_pc_name().c_str() : "")) {
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

		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (STATE(d) == CON_PLAYING) {
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
			if (!CAN_SEE(ch, character) || GetRealLevel(character) < low || GetRealLevel(character) > high) {
				continue;
			}
			if (outlaws && !PLR_FLAGGED((ch), PLR_KILLER)) {
				continue;
			}
			if (showclass != ECharClass::kUndefined && showclass != character->get_class()) {
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
							GET_REAL_REMORT(d->original),
							MUD::Classes()[d->original->get_class()].GetAbbr().c_str());
				} else {
					sprintf(classname,
							"[%2d %s]   ",
							GetRealLevel(d->original),
							MUD::Classes()[d->original->get_class()].GetAbbr().c_str());
				}
			} else if (showremorts) {
				sprintf(classname,
						"[%2d %2d %s]",
						GetRealLevel(d->character),
						GET_REAL_REMORT(d->character),
						MUD::Classes()[d->character->get_class()].GetAbbr().c_str());
			} else {
				sprintf(classname,
						"[%2d %s]   ",
						GetRealLevel(d->character),
						MUD::Classes()[d->character->get_class()].GetAbbr().c_str());
			}
		} else {
			strcpy(classname, "      -      ");
		}

		if (GetRealLevel(ch) < kLvlImplementator && !PRF_FLAGGED(ch, PRF_CODERINFO)) {
			strcpy(classname, "      -      ");
		}

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CON_PLAYING && d->original) {
			strcpy(state, "Switched");
		} else {
			sprinttype(STATE(d), connected_types, state);
		}

		if (d->character
			&& STATE(d) == CON_PLAYING
			&& !IS_GOD(d->character)) {
			sprintf(idletime, "%-3d", d->character->char_specials.timer *
				SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		} else {
			strcpy(idletime, "   ");
		}

		if (d->character
			&& d->character->get_pc_name().c_str()) {
			if (d->original) {
				sprintf(line,
						format,
						d->desc_num,
						classname,
						d->original->get_pc_name().c_str(),
						state,
						idletime,
						timeptr);
			} else {
				sprintf(line,
						format,
						d->desc_num,
						classname,
						d->character->get_pc_name().c_str(),
						state,
						idletime,
						timeptr);
			}
		} else {
			sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED", state, idletime, timeptr);
		}

		if (d && *d->host) {
			sprintf(line2, "[%s]", d->host);
			strcat(line, line2);
		} else {
			strcat(line, "[Неизвестный хост]");
		}

		if (showemail) {
			sprintf(line2, "[&S%s&s]",
					d->original ? GET_EMAIL(d->original) : d->character ? GET_EMAIL(d->character) : "");
			strcat(line, line2);
		}

		if (locating && (*name_search || *host_by_name)) {
			if (STATE(d) == CON_PLAYING) {
				const auto ci = d->get_character();
				if (ci
					&& CAN_SEE(ch, ci)
					&& ci->in_room != kNowhere) {
					if (d->original && d->character) {
						sprintf(line2, " [%5d] %s (in %s)",
								GET_ROOM_VNUM(IN_ROOM(d->character)),
								world[d->character->in_room]->name, GET_NAME(d->character));
					} else {
						sprintf(line2, " [%5d] %s",
								GET_ROOM_VNUM(IN_ROOM(ci)), world[ci->in_room]->name);
					}
				}

				strcat(line, line2);
			}
		}

		strcat(line, "\r\n");
		if (STATE(d) != CON_PLAYING) {
			sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}

		if (STATE(d) != CON_PLAYING || (STATE(d) == CON_PLAYING && d->character && CAN_SEE(ch, d->character))) {
			send_to_char(line, ch);
			num_can_see++;
		}
	}

	sprintf(line, "\r\n%d видимых соединений.\r\n", num_can_see);
	page_string(ch->desc, line, true);
}