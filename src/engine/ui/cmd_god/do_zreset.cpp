/**
\file DoZreset.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "utils/utils_time.h"
#include "administration/privilege.h"
#include "engine/olc/olc.h"
#include "engine/entities/zone.h"

void DoZreset(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	ZoneRnum i;
	UniqueList<ZoneRnum> zone_repop_list;
	char zone_arg[kMaxInputLength];
	one_argument(argument, zone_arg);

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <= 0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (!*zone_arg) {
		SendMsgToChar("Укажите зону.\r\n", ch);
		return;
	}
	// zreset *<номер> -- сброс всего комплекса: головная зона и её зоны typeA
	// (зоны, которые сбрасываются одновременно с головной).
	if (*zone_arg == '*') {
		if (!*(zone_arg + 1)) {
			SendMsgToChar("Укажите головную зону комплекса: zreset *<номер>.\r\n", ch);
			return;
		}
		const int zone_vnum = atoi(zone_arg + 1);
		if (!privilege::IsImmortal(ch) && GET_OLC_ZONE(ch) != zone_vnum) {
			SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
			return;
		}
		const ZoneRnum head = GetZoneRnum(zone_vnum);
		if (head < 0) {
			SendMsgToChar("Нет такой зоны.\r\n", ch);
			return;
		}
		if (zone_table[head].typeA_count <= 0) {
			SendMsgToChar("Это не головная зона комплекса, обратитесь к Богу.\r\n", ch);
			return;
		}
		utils::CExecutionTimer timer;
		zone_repop_list.push_back(head);
		for (int a = 0; a < zone_table[head].typeA_count; a++) {
			const ZoneRnum rn = GetZoneRnum(zone_table[head].typeA_list[a]);
			if (rn >= 0) {
				zone_repop_list.push_back(rn);
			}
		}
		SendMsgToChar(fmt::format("Перегружаю комплекс зоны #{}: {}\r\n",
								  zone_table[head].vnum, zone_table[head].name), ch);
		DecayObjectsOnRepop(zone_repop_list);
		for (const auto rn : zone_repop_list) {
			ResetZone(rn);
		}
		mudlog(fmt::format("(GC) {} reset complex {} ({}), delta {:f}",
						   GET_NAME(ch), zone_table[head].vnum, zone_table[head].name, timer.delta().count()),
			   NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset complex %d (%s)", GET_NAME(ch), zone_table[head].vnum, zone_table[head].name.c_str());
		return;
	}
	if (!privilege::IsImmortal(ch) && GET_OLC_ZONE(ch) != atoi(zone_arg)) {
		SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
		return;
	}
	if (*zone_arg == '.') {
		i = world[ch->in_room]->zone_rn;
	} else {
		i = GetZoneRnum(atoi(zone_arg));
	}
	if (i >= 0 || *zone_arg == '.') {
		utils::CExecutionTimer timer;

		SendMsgToChar(fmt::format("Перегружаю зону #{}: {}\r\n", zone_table[i].vnum, zone_table[i].name), ch);
		zone_repop_list.push_back(i);
		DecayObjectsOnRepop(zone_repop_list);
		ResetZone(i);
		mudlog(fmt::format("(GC) {} reset zone {} ({}), delta {:f}",
						   GET_NAME(ch), i, zone_table[i].name, timer.delta().count()),
			   NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name.c_str());
	} else {
		SendMsgToChar("Нет такой зоны.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
