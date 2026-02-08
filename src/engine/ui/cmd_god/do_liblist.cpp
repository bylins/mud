/**
\file liblist.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "do_liblist.h"

#include "engine/entities/char_data.h"
#include "engine/db/obj_prototypes.h"
#include "engine/olc/olc.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "administration/privilege.h"
#include "engine/entities/zone.h"

#include <third_party_libs/fmt/include/fmt/format.h>

int PrintOlist(const CharData *ch, int first, int last, std::string &out);

namespace mob_list {
	void Print(CharData *ch, int first, int last, const std::string &options);
	std::string PrintFlag(CharData *mob, const std::string &options);
	std::string PrintRole(CharData *mob);
	std::string PrintRace(CharData *mob);
}

void do_liblist(CharData *ch, char *argument, int cmd, int subcmd) {
	int first, last, nr, found = 0;

	argument = two_arguments(argument, buf, buf2);
	first = atoi(buf);
	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) != first)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!*buf || (!*buf2 && (subcmd == kScmdZlist))) {
		switch (subcmd) {
			case kScmdRlist:
				SendMsgToChar("Использование: ксписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			case kScmdOlist:
				SendMsgToChar("Использование: осписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			case kScmdMlist:
				SendMsgToChar("Использование: мсписок <начальный номер или номер зоны> [<конечный номер>] [role race]\r\n",
							  ch);
				break;
			case kScmdZlist: SendMsgToChar("Использование: зсписок <начальный номер> <конечный номер>\r\n", ch);
				break;
			case kScmdClist:
				SendMsgToChar("Использование: ксписок <начальный номер или номер зоны> [<конечный номер>]\r\n",
							  ch);
				break;
			default: sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
				mudlog(buf, BRF, kLvlGod, SYSLOG, true);
				break;
		}
		return;
	}

	if (*buf2 && a_isdigit(buf2[0])) {
		last = atoi(buf2);
	} else {
		first *= 100;
		last = first + 99;
	}

	if ((first < 0) || (first > kMaxProtoNumber) || (last < 0) || (last > kMaxProtoNumber)) {
		sprintf(buf, "Значения должны быть между 0 и %d.\n\r", kMaxProtoNumber);
		SendMsgToChar(buf, ch);
		return;
	}

	if (first > last) {
		std::swap(first, last);
	}

	if (first + 100 < last) {
		SendMsgToChar("Максимальный показываемый промежуток - 100.\n\r", ch);
		return;
	}

	char buf_[256];
	std::string out;

	switch (subcmd) {
		case kScmdRlist:
			snprintf(buf_, sizeof(buf_),
					 "Список комнат от Vnum %d до %d\r\n", first, last);
			out += buf_;
			for (nr = kFirstRoom; nr <= top_of_world && (world[nr]->vnum <= last); nr++) {
				if (world[nr]->vnum >= first) {
					snprintf(buf_, sizeof(buf_), "%5d. [%5d] (%3d) %s",
							 ++found, world[nr]->vnum, nr, world[nr]->name);
					out += buf_;
					if (!world[nr]->proto_script->empty()) {
						out += " - есть скрипты -";
						for (const auto trigger_vnum : *world[nr]->proto_script) {
							sprintf(buf1, " [%d]", trigger_vnum);
							out += buf1;
						}
						out += "\r\n";
					} else {
						out += " - нет скриптов\r\n";
					}
				}
			}
			break;
		case kScmdOlist: found = PrintOlist(ch, first, last, out);
			break;

		case kScmdMlist: {
			std::string option;
			if (*buf2 && !a_isdigit(buf2[0])) {
				option = buf2;
			}
			option += " ";
			option += argument;
			mob_list::Print(ch, first, last, option);
			return;
		}
		case kScmdZlist:
			snprintf(buf_, sizeof(buf_),
					 "Список зон от %d до %d\r\n"
					 "(флаги, номер, резет, уровень/средний уровень мобов, группа, имя)\r\n",
					 first, last);
			out += buf_;

			for (nr = 0; nr < static_cast<ZoneRnum>(zone_table.size()) && (zone_table[nr].vnum <= last); nr++) {
				if (zone_table[nr].vnum >= first) {
					snprintf(buf_, sizeof(buf_),
							 "%5d. [%s%s] [%5d] (%3d) (%2d/%2d) (%2d) %s\r\n",
							 ++found,
							 zone_table[nr].locked ? "L" : " ",
							 zone_table[nr].under_construction ? "T" : " ",
							 zone_table[nr].vnum,
							 zone_table[nr].lifespan,
							 zone_table[nr].level,
							 zone_table[nr].mob_level,
							 zone_table[nr].group,
							 zone_table[nr].name.c_str());
					out += buf_;
				}
			}
			break;

		case kScmdClist: out = "Заглушка. Возможно, будет использоваться в будущем\r\n";
			break;

		default: sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
			mudlog(buf, BRF, kLvlGod, SYSLOG, true);
			return;
	}

	if (!found) {
		switch (subcmd) {
			case kScmdRlist: SendMsgToChar("Нет комнат в этом промежутке.\r\n", ch);
				break;
			case kScmdOlist: SendMsgToChar("Нет объектов в этом промежутке.\r\n", ch);
				break;
			case kScmdZlist: SendMsgToChar("Нет зон в этом промежутке.\r\n", ch);
				break;
			default: sprintf(buf, "SYSERR:: invalid SCMD passed to do_build_list!");
				mudlog(buf, BRF, kLvlGod, SYSLOG, true);
				break;
		}
		return;
	}

	page_string(ch->desc, out);
}

int PrintOlist(const CharData *ch, int first, int last, std::string &out) {
	int result = 0;

	char buf_[256] = {0};
	std::stringstream ss;
	snprintf(buf_, sizeof(buf_), "Список объектов Vnum %d до %d\r\n", first, last);
	ss << buf_;

	auto from = obj_proto.vnum2index().lower_bound(first);
	auto to = obj_proto.vnum2index().upper_bound(last);
	for (auto i = from; i != to; ++i) {
		const auto vnum = i->first;
		const auto rnum = i->second;
		const auto& prototype = obj_proto[rnum];

		ss << fmt::format("{:>5}. {} [{:>5}] [ilvl ={} : mort ={}]", ++result,
						  colored_name(prototype->get_short_description().c_str(), 45), vnum, prototype->get_ilevel(), prototype->get_auto_mort_req());

		if (GetRealLevel(ch) >= kLvlGreatGod
			|| ch->IsFlagged(EPrf::kCoderinfo)) {
			ss << fmt::format(" Игра:{} Пост:{} Макс:{}", static_cast<int>(obj_proto.total_online(rnum)), static_cast<int>(obj_proto.stored(rnum)), GetObjMIW(static_cast<ObjRnum>(rnum)));
			const auto &script = prototype->get_proto_script();

			if (!script.empty()) {
				ss << " - есть скрипты -";
				for (const auto trigger_vnum : script) {
					ss << fmt::format(" [{}]", trigger_vnum);
				}
			} else {
				ss << " - нет скриптов";
			}
		}
		ss << "\r\n";
	}

	out = ss.str();
	return result;
}

namespace mob_list {

std::string PrintRace(CharData *mob) {
	std::string out;
	if (GET_RACE(mob) <= ENpcRace::kLastNpcRace) {
		out += npc_race_types[GET_RACE(mob) - ENpcRace::kBasic];
	} else {
		out += "UNDEF";
	}
	return out;
}

std::string PrintRole(CharData *mob) {
	std::string out;
	if (mob->get_role_bits().any()) {
		print_bitset(mob->get_role_bits(), npc_role_types, ",", out);
	} else {
		out += "---";
	}
	return out;
}

std::string PrintFlag(CharData *mob, const std::string &options) {
	std::vector<std::string> option_list = utils::SplitAny(options, ",. ");
	auto out = fmt::memory_buffer();

	for (const auto & i : option_list) {
		if (isname(i, "race")) {
			fmt::format_to(std::back_inserter(out), " [раса: {}{}{} ]",
					  kColorCyn, PrintRace(mob), kColorNrm);
		}
		if (isname(i, "role")) {
			fmt::format_to(std::back_inserter(out), " [роли: {}{}{} ]",
					  kColorCyn, PrintRole(mob), kColorNrm);
		}
		fmt::format_to(std::back_inserter(out), " [спец-проц: {}{}{} ]",
				  kColorCyn, print_special(mob), kColorNrm);
	}
	return to_string(out);
}

void Print(CharData *ch, int first, int last, const std::string &options) {
	auto out = fmt::memory_buffer();

	fmt::format_to(std::back_inserter(out), "Список мобов от {} до {}\r\n", first, last);
	int cnt = 0;
	for (int i = 0; i <= top_of_mobt; ++i) {
		if (mob_index[i].vnum >= first && mob_index[i].vnum <= last) {
			fmt::format_to(std::back_inserter(out), "{:5}. {:<45} [{:<6}] [{:<2}]{}",
					  ++cnt, mob_proto[i].get_name_str().substr(0, 45),
					  mob_index[i].vnum, mob_proto[i].GetLevel(),
					  PrintFlag(mob_proto + i, options));
			if (!mob_proto[i].proto_script->empty()) {
				fmt::format_to(std::back_inserter(out), " - есть скрипты -");
				for (const auto trigger_vnum : *mob_proto[i].proto_script) {
					fmt::format_to(std::back_inserter(out), " {}", trigger_vnum);
				}
			} else {
				fmt::format_to(std::back_inserter(out), " - нет скриптов -");
			}
			fmt::format_to(std::back_inserter(out), " Загружено в мир: {}, максимально: {}.\r\n", mob_index[i].total_online, mob_index[i].total_online);
		}
	}
	if (cnt == 0) {
		SendMsgToChar("Нет мобов в этом промежутке.\r\n", ch);
	} else {
		page_string(ch->desc, to_string(out));
	}
}

} // namespace Mlist

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
