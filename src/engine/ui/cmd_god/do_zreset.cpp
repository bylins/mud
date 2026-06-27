/**
\file DoZreset.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "utils/utils_time.h"
#include "administration/privilege.h"
#include "engine/olc/olc.h"
#include "engine/entities/zone.h"

void DoZreset(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	ZoneRnum i;
	UniqueList<ZoneRnum> zone_repop_list;
	one_argument(argument, arg);

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <= 0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (!*arg) {
		SendMsgToChar("Укажите зону.\r\n", ch);
		return;
	}
	// zreset *<номер> -- сброс всего комплекса: головная зона и её зоны typeA
	// (зоны, которые сбрасываются одновременно с головной).
	if (*arg == '*') {
		if (!*(arg + 1)) {
			SendMsgToChar("Укажите головную зону комплекса: zreset *<номер>.\r\n", ch);
			return;
		}
		const int zone_vnum = atoi(arg + 1);
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
		sprintf(buf, "Перегружаю комплекс зоны #%d: %s\r\n", zone_table[head].vnum, zone_table[head].name.c_str());
		SendMsgToChar(buf, ch);
		DecayObjectsOnRepop(zone_repop_list);
		for (const auto rn : zone_repop_list) {
			ResetZone(rn);
		}
		sprintf(buf, "(GC) %s reset complex %d (%s), delta %f",
				GET_NAME(ch), zone_table[head].vnum, zone_table[head].name.c_str(), timer.delta().count());
		mudlog(buf, NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset complex %d (%s)", GET_NAME(ch), zone_table[head].vnum, zone_table[head].name.c_str());
		return;
	}
	if (!privilege::IsImmortal(ch) && GET_OLC_ZONE(ch) != atoi(arg)) {
		SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
		return;
	}
	if (*arg == '.') {
		i = world[ch->in_room]->zone_rn;
	} else {
		i = GetZoneRnum(atoi(arg));
	}
	if (i >= 0 || *arg == '.') {
		utils::CExecutionTimer timer;

		sprintf(buf, "Перегружаю зону #%d: %s\r\n", zone_table[i].vnum, zone_table[i].name.c_str());
		SendMsgToChar(buf, ch);
		zone_repop_list.push_back(i);
		DecayObjectsOnRepop(zone_repop_list);
		ResetZone(i);
		sprintf(buf, "(GC) %s reset zone %d (%s), delta %f", GET_NAME(ch), i, zone_table[i].name.c_str(), timer.delta().count());
		mudlog(buf, NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name.c_str());
	} else {
		SendMsgToChar("Нет такой зоны.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
