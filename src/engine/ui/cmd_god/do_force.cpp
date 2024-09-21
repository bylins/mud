/**
\file force.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/core/handler.h"

void do_force(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *i, *next_desc;
	char to_force[kMaxInputLength + 2];

	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n принудил$g вас '%s'.", to_force);

	if (!*arg || !*to_force) {
		SendMsgToChar("Кого и что вы хотите принудить сделать?\r\n", ch);
	} else if (!IS_GRGOD(ch) || (str_cmp("all", arg) && str_cmp("room", arg) && str_cmp("все", arg)
		&& str_cmp("здесь", arg))) {
		const auto vict = get_char_vis(ch, arg, EFind::kCharInWorld);
		if (!vict) {
			SendMsgToChar(NOPERSON, ch);
		} else if (!vict->IsNpc() && GetRealLevel(ch) <= GetRealLevel(vict) && !ch->IsFlagged(EPrf::kCoderinfo)) {
			SendMsgToChar("Господи, только не это!\r\n", ch);
		} else {
			char *pstr;
			SendMsgToChar(OK, ch);
			act(buf1, true, ch, nullptr, vict, kToVict);
			sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			while ((pstr = strchr(buf, '%')) != nullptr) {
				pstr[0] = '*';
			}
			mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			imm_log("%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			command_interpreter(vict, to_force);
		}
	} else if (!str_cmp("room", arg)
		|| !str_cmp("здесь", arg)) {
		SendMsgToChar(OK, ch);
		sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force);
		mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force);

		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (!vict->IsNpc()
				&& GetRealLevel(vict) >= GetRealLevel(ch)
				&& !ch->IsFlagged(EPrf::kCoderinfo)) {
				continue;
			}

			act(buf1, true, ch, nullptr, vict, kToVict);
			command_interpreter(vict, to_force);
		}
	} else        // force all
	{
		SendMsgToChar(OK, ch);
		sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
		mudlog(buf, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s forced all to %s", GET_NAME(ch), to_force);

		for (i = descriptor_list; i; i = next_desc) {
			next_desc = i->next;

			const auto vict = i->character;
			if (STATE(i) != CON_PLAYING
				|| !vict
				|| (!vict->IsNpc() && GetRealLevel(vict) >= GetRealLevel(ch)
					&& !ch->IsFlagged(EPrf::kCoderinfo))) {
				continue;
			}

			act(buf1, true, ch, nullptr, vict.get(), kToVict);
			command_interpreter(vict.get(), to_force);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
