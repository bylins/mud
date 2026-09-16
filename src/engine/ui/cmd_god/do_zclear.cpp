/**
\file do_clear_zone.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "utils/utils_time.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/entities/zone.h"

void DoClearZone(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	UniqueList<ZoneRnum> zone_repop_list;
	ZoneRnum zrn;

	char zone_arg[kMaxInputLength];
	one_argument(argument, zone_arg);
	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false))) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!*zone_arg) {
		SendMsgToChar("Укажите зону.\r\n", ch);
		return;
	}
	if (*zone_arg == '.') {
		zrn = world[ch->in_room]->zone_rn;
	} else {
		zrn = GetZoneRnum(atoi(zone_arg));
	}
	// Было "zrn > 0 || *arg == '.' || *arg == '1'": первая зона имеет rnum 0, и её
	// пропускали через проверку на ведущую единицу в номере. Но тогда и любая
	// несуществующая зона, номер которой начинается с единицы (17, 150), проходила
	// с zrn == -1 -- и команда лезла в zone_table[-1]. Проверяем сам rnum, как в zreset.
	if (zrn >= 0) {
		utils::CExecutionTimer timer;

		SendMsgToChar(fmt::format("Очищаю и перегружаю зону #{}: {}\r\n",
								  zone_table[zrn].vnum, zone_table[zrn].name), ch);
		for (auto rrn = zone_table[zrn].RnumRoomsLocation.first; rrn <= zone_table[zrn].RnumRoomsLocation.second; rrn++) {
			dungeons::ClearRoom(world[rrn]);
		}
		dungeons::ClearRoom(world[zone_table[zrn].RnumRoomsLocation.second + 1]); //виртуалку
		zone_repop_list.push_back(zrn);
		DecayObjectsOnRepop(zone_repop_list);
		ResetZone(zrn);
		mudlog(fmt::format("(GC) {} clear and reset zone {} ({}), delta {:f}",
						   GET_NAME(ch), zrn, zone_table[zrn].name, timer.delta().count()),
			   NRM, MAX(kLvlGreatGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s clear and reset zone %d (%s)", GET_NAME(ch), zrn, zone_table[zrn].name.c_str());
	} else {
		SendMsgToChar("Нет такой зоны.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
