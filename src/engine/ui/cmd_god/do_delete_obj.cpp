/**
\file do_delete_obj.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "engine/db/world_objects.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/communication/parcel.h"
#include "engine/db/obj_prototypes.h"
#include "engine/db/player_index.h"

void DoDeleteObj(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int vnum;
	char arg_vnum[kMaxInputLength];
	one_argument(argument, arg_vnum);
	int num = 0;
	if (!*arg_vnum || !a_isdigit(*arg_vnum)) {
		SendMsgToChar("Usage: delete <number>\r\n", ch);
		return;
	}
	if ((vnum = atoi(arg_vnum)) < 0) {
		SendMsgToChar("Указан неверный VNUM объекта !\r\n", ch);
		return;
	}

	world_objects.foreach_with_vnum(vnum, [&num](const ObjData::shared_ptr &k) {
	  k->set_timer(0);
	  ++num;
	});
//	кланхран вещи в игре их не надо чистить, удалены выше.
//	num += Clan::delete_obj(vnum);
	num += Depot::delete_obj(vnum);
	num += Parcel::delete_obj(vnum);
	SendMsgToChar(fmt::format("Удалено всего предметов: {}, смотрим ренту.\r\n", num), ch);
	num = 0;
	for (std::size_t pt_num = 0; pt_num< player_table.size(); pt_num++) {
		bool need_save = false;
		// рента
		if (player_table[pt_num].timer) {
			for (auto i = player_table[pt_num].timer->time.begin(),
					 iend = player_table[pt_num].timer->time.end(); i != iend; ++i) {
				if (i->vnum == vnum && i->timer > 0) {
					num++;
					SendMsgToChar(fmt::format("Player {} : item [{}] deleted\r\n",
											  player_table[pt_num].name(), i->vnum), ch);
					i->timer = -1;
					int rnum = GetObjRnum(i->vnum);
					if (rnum >= 0) {
						obj_proto.dec_stored(rnum);
					}
					need_save = true;
				}
			}
		}
		if (need_save) {
			if (!Crash_write_timer(pt_num)) {
				// Сообщение собиралось в buf, а богу уходил buf2 -- то есть предыдущая
				// строка "item deleted" вместо ошибки записи файла таймеров.
				SendMsgToChar(fmt::format("SYSERROR: [TO] Error writing timer file for {}\r\n",
										  player_table[pt_num].name()), ch);
			}
		}
	}
	SendMsgToChar(fmt::format("Удалено еще предметов: {}.\r\n", num), ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
