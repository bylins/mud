//
// Created by Sventovit on 22.02.2022.
//

#include <regex>

#include "color.h"
#include "game_classes/classes.h"
#include "entities/char_data.h"
#include "modify.h"
#include "structs/global_objects.h"
#include "utils/utils_string.h"
#include "utils/table_wrapper.h"
#include "utils/utils.h"

struct UsersOpt {
	int minlevel{0};
	int maxlevel{kLvlImplementator};
	bool showoutlaws{false};
	bool showplaying{false};
	bool locating{false};
	bool showemail{false};
	bool showlinkdrop{false};
	bool showremorts{false};
	ECharClass char_class{ECharClass::kUndefined};
	std::string name;
	std::string host;
	std::string host_name;
	std::string sorting;
};

using UserOptOptional = std::optional<UsersOpt>;

UserOptOptional ParseCommandOptions(const std::string &input, CharData *ch) {
	auto users_opt = std::make_optional<UsersOpt>();
	auto tokens = utils::SplitString(input, ' ');
	for (unsigned i = 0; i < tokens.size(); ++i) {
		if (tokens[i][0] != '-' || tokens[i].size() < 2) {
			continue;
		}

		switch (tokens[i][1]) {
			case 'o': [[fallthrough]];
			case 'k':
				users_opt.value().showoutlaws = true;
				users_opt.value().showplaying = true;
			break;
			case 'p':
				users_opt.value().showplaying = true;
			break;
			case 'd':
				users_opt.value().showlinkdrop = true;
			break;
			case 'l':
				if (!IS_GOD(ch)) {
					send_to_char("Вы не столь божественны!", ch);
					return std::nullopt;
				}

				users_opt.value().showplaying = true;

				try  {
					users_opt->minlevel = std::stoi(tokens[i + 1]);
				} catch (std::exception &) {
					return std::nullopt;
				}
				try  {
					users_opt->minlevel = std::stoi(tokens[i + 1]);
				} catch (std::exception &) {
				}

				break;
			case 'n':
				users_opt.value().showplaying = true;
				try  {
					users_opt.value().name = tokens[i + 1];
				} catch (std::exception &) {
					return std::nullopt;
				}
				break;
			case 'h':
				users_opt.value().showplaying = true;
				try  {
					users_opt.value().host = tokens[i + 1];
				} catch (std::exception &) {
					return std::nullopt;
				}
				break;
			case 'u':
				users_opt.value().showplaying = true;
				users_opt.value().showplaying = true;
				try  {
					users_opt.value().host_name = tokens[i + 1];
				} catch (std::exception &) {
					return std::nullopt;
				}
				break;
			case 'w':
				if (!IS_GRGOD(ch)) {
					send_to_char("Вы не столь божественны!", ch);
					return std::nullopt;
				}
				users_opt.value().showplaying = true;
				users_opt.value().locating = true;
				break;
			case 'c': {
				users_opt.value().showplaying = true;
				users_opt.value().showplaying = true;
				try  {
					users_opt.value().char_class = FindAvailableCharClassId(tokens[i + 1]);
				} catch (std::exception &) {
					return std::nullopt;
				}
				break;
			}
			case 'e':
				users_opt.value().showemail = true;
				break;
			case 'r':
				users_opt.value().showremorts = true;
				break;
			case 's':
				try  {
					users_opt.value().sorting = tokens[i + 1];
				} catch (std::exception &) {
					return std::nullopt;
				}
				break;
			default:
				return std::nullopt;
		}
	}

	return users_opt;
}

std::vector<DescriptorData *> &BuildPlayersRoster(std::vector<DescriptorData *> &roster) {
	const int kReserveAmount = 200;
	std::vector<DescriptorData *> ld_players_roster;
	roster.reserve(kReserveAmount);
	for (auto d = descriptor_list; d; d = d->next) {
		const auto character = d->get_character();
		if (!character) {
			ld_players_roster.push_back(d);
			continue;
		} else {
			roster.push_back(d);
		}
	}
	roster.insert(roster.end(), ld_players_roster.begin(), ld_players_roster.end());

	return roster;
}

void SortPlayersRoster(std::vector<DescriptorData *> &roster, UsersOpt &options) {
	if (options.sorting == "name") {
		std::sort(roster.begin(), roster.end(), [] (auto &c1, auto &c2) {
			if (!c1->get_character()) {
				return false;
			}
			return c1->get_character()->get_name() > c2->get_character()->get_name();
		});
	} else if (options.sorting == "email") {
		std::sort(roster.begin(), roster.end(), [] (auto &c1, auto &c2) {
			if (!c1->get_character()) {
				return false;
			}
			return GET_EMAIL(c1->get_character()) > GET_EMAIL(c2->get_character());
		});
	} else {
		std::sort(roster.begin(), roster.end(), [] (auto &c1, auto &c2) {
			return get_ip(const_cast<char *>(c1->host)) > get_ip(const_cast<char *>(c2->host));
		});
	}
}

void do_users(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	static const std::string kUsersFormat{"Формат: users [-l minlvl[-maxlvl]] [-n name] [-h host] [-c class] [-h host] [-u hostname] [-s name|email] [-d] [-e] [-k] [-o] [-p] [-r] [-w]\r\n"};

	auto parse_options_result = ParseCommandOptions(argument, ch);
	if (!parse_options_result) {
		send_to_char(kUsersFormat, ch);
		return;
	}
	auto &options = parse_options_result.value();

	char line[200], line2[220], idletime[10], classname[128];
	char state[30] = "\0", *timeptr, mode;
	char name_search[kMaxInputLength] = "\0", host_search[kMaxInputLength];
	char host_by_name[kMaxInputLength] = "\0";
	int cycle_i, is, flag_change;
	unsigned long a1, a2;
	int num_can_see = 0;
	const char *format = "%3d %-7s %-20s %-17s %-3s %-8s ";
	// ========================================

	if (strlen(host_by_name) != 0) {
		strcpy(host_search, "!");
	}

	std::vector<DescriptorData *> players_roster;
	players_roster = BuildPlayersRoster(players_roster);

	if (isname(host_by_name, GET_NAME(players_roster[0]->character))) {
		strcpy(host_search, players_roster[0]->host);
	}

	if (!options.sorting.empty()) {
		SortPlayersRoster(players_roster, options);
	}

	fort::char_table table;
	table << fort::header << "#" << "Class" << "Name" << "Connection state" << "Idle" << "Login" << "Host";
	if (options.showemail) {
		table << "E-mail" << fort::endr;
	} else {
		table << fort::endr;
	}

	for (auto d : players_roster) {

		if (STATE(d) != CON_PLAYING && options.showplaying) {
			continue;
		}
		if (STATE(d) == CON_PLAYING && options.showlinkdrop) {
			continue;
		}
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
			if (!CAN_SEE(ch, character)
				|| GetRealLevel(character) < options.minlevel
				|| GetRealLevel(character) > options.maxlevel) {
				continue;
			}
			if (options.showoutlaws && !PLR_FLAGGED((ch), PLR_KILLER)) {
				continue;
			}
			if (options.char_class != ECharClass::kUndefined && options.char_class != character->get_class()) {
				continue;
			}
			if (GET_INVIS_LEV(character) > GetRealLevel(ch)) {
				continue;
			}

			if (d->original) {
				if (options.showremorts) {
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
			} else if (options.showremorts) {
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

		if (options.showemail) {
			sprintf(line2, "[&S%s&s]",
					d->original ? GET_EMAIL(d->original) : d->character ? GET_EMAIL(d->character) : "");
			strcat(line, line2);
		}

		options.host_name;
		if (options.locating && (*name_search || *host_by_name)) {
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
