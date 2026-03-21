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
	if (!IS_IMMORTAL(ch) && GET_OLC_ZONE(ch) != atoi(arg)) {
		SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
		return;
	}
	if (*arg == '*') {
		for (i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
			zone_repop_list.push_back(i);
		}
		DecayObjectsOnRepop(zone_repop_list);
		for (i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
			ResetZone(i);
		}
		SendMsgToChar("Перезагружаю мир.\r\n", ch);
		sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s reset entire world.", GET_NAME(ch));
		return;
	} else if (*arg == '.') {
		i = world[ch->in_room]->zone_rn;
	} else {
		i = GetZoneRnum(atoi(arg));
	}
	if (i > 0 || *arg == '.' || *arg == '1') {
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
