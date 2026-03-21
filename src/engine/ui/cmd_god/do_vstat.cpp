/**
\file do_vstat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "administration/privilege.h"
#include "engine/olc/olc.h"
#include "do_stat.h"
#include "engine/db/world_objects.h"

void DoVstat(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	CharData *mob;
	MobVnum number;    // or ObjVnum ...
	MobRnum r_num;        // or ObjRnum ...

	two_arguments(argument, buf, buf2);
	int first = atoi(buf2) / 100;

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <= 0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	if (!*buf || !*buf2 || !a_isdigit(*buf2)) {
		SendMsgToChar("Usage: vstat { obj | mob } <number>\r\n", ch);
		return;
	}


	if (!IS_IMMORTAL(ch) && GET_OLC_ZONE(ch) != first) {
		SendMsgToChar("Доступ к данной зоне запрещен!\r\n", ch);
		return;
	}

	if ((number = atoi(buf2)) < 0) {
		SendMsgToChar("Отрицательный номер? Оригинально!\r\n", ch);
		return;
	}
	if (utils::IsAbbr(buf, "mob")) {
		if ((r_num = GetMobRnum(number)) < 0) {
			SendMsgToChar("Обратитесь в Арктику - там ОН живет.\r\n", ch);
			return;
		}
		mob = ReadMobile(r_num, kReal);
		PlaceCharToRoom(mob, 1);
		do_stat_character(ch, mob, 1);
		ExtractCharFromWorld(mob, false);
	} else if (utils::IsAbbr(buf, "obj")) {
		if ((r_num = GetObjRnum(number)) < 0) {
			SendMsgToChar("Этот предмет явно перенесли в РМУД.\r\n", ch);
			return;
		}

		const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
		do_stat_object(ch, obj.get(), 1);
		ExtractObjFromWorld(obj.get());
	} else
		SendMsgToChar("Тут должно быть что-то типа 'obj' или 'mob'.\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
