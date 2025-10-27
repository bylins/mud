/**
\file do_purge.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/ui/cmd/do_follow.h"

void DoPurge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	ObjData *obj, *next_o;

	one_argument(argument, buf);

	if (*buf) {        // argument supplied. destroy single object or char
		if ((vict = get_char_vis(ch, buf, EFind::kCharInRoom)) != nullptr) {
			if (!vict->IsNpc() && GetRealLevel(ch) <= GetRealLevel(vict) && !ch->IsFlagged(EPrf::kCoderinfo)) {
				SendMsgToChar("Да я вас за это...\r\n", ch);
				return;
			}
			act("$n обратил$g в прах $N3.", false, ch, nullptr, vict, kToNotVict);
			if (!vict->IsNpc()) {
				sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				mudlog(buf, CMP, std::max(kLvlImmortal, GET_INVIS_LEV(ch)), SYSLOG, true);
				imm_log("%s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				if (vict->desc) {
					STATE(vict->desc) = CON_CLOSE;
					vict->desc->character = nullptr;
					vict->desc = nullptr;
				}
			}
			ExtractCharFromWorld(vict, false);
		} else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room]->contents)) != nullptr) {
			act("$n просто разметал$g $o3 на молекулы.", false, ch, obj, nullptr, kToRoom);
			ExtractObjFromWorld(obj);
		} else {
			SendMsgToChar("Ничего похожего с таким именем нет.\r\n", ch);
			return;
		}
		SendMsgToChar(OK, ch);
	} else        // no argument. clean out the room
	{
		act("$n произнес$q СЛОВО... вас окружило пламя!", false, ch, nullptr, nullptr, kToRoom);
		SendMsgToRoom("Мир стал немного чище.\r\n", ch->in_room, false);
		for (obj = world[ch->in_room]->contents; obj; obj = next_o) { //сначала шмотки, иначе потетеряешь весь стаф с случайных чармисов
			next_o = obj->get_next_content();
			ExtractObjFromWorld(obj);
		}
		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (vict->IsNpc()) {
				if (vict->followers
					|| vict->has_master()) {
					die_follower(vict);
				}
				if (!vict->purged()) {
					ExtractCharFromWorld(vict, false);
				}
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
