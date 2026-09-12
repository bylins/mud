/**
\file force.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "engine/network/descriptor_data.h"
#include "engine/core/target_resolver.h"
#include "utils/utils_string.h"

void do_force(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *i, *next_desc;

	std::string to_force;
	const std::string arg = utils::ExtractFirstArgumentLower(argument ? argument : "", to_force);

	const std::string to_vict = fmt::format("$n принудил$g вас '{}'.", to_force);

	// command_interpreter правит переданную строку на месте, поэтому каждой жертве
	// достаётся своя копия -- иначе второй в списке получил бы огрызок команды.
	const auto force_command = [&to_force](CharData *victim) {
		char command[kMaxInputLength + 2];
		strl_cpy(command, to_force.c_str(), sizeof(command));
		command_interpreter(victim, command);
	};

	if (arg.empty() || to_force.empty()) {
		SendMsgToChar("Кого и что вы хотите принудить сделать?\r\n", ch);
	} else if (!privilege::IsGrGod(ch) || (str_cmp("all", arg) && str_cmp("room", arg) && str_cmp("все", arg)
		&& str_cmp("здесь", arg))) {
		CharData *vict = nullptr;
		vict = target_resolver::FindCharInWorld(ch, arg);
		if (!vict) {
			SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
		} else if (!vict->IsNpc() && GetRealLevel(ch) <= GetRealLevel(vict) && !ch->IsFlagged(EPrf::kCoderinfo)) {
			SendMsgToChar("Господи, только не это!\r\n", ch);
		} else {
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			act(to_vict, true, ch, nullptr, vict, kToVict);
			std::string log_line = fmt::format("(GC) {} forced {} to {}", GET_NAME(ch), GET_NAME(vict), to_force);
			std::replace(log_line.begin(), log_line.end(), '%', '*');
			mudlog(log_line, NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
			imm_log("%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force.c_str());
			force_command(vict);
		}
	} else if (!str_cmp("room", arg)
		|| !str_cmp("здесь", arg)) {
		SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
		mudlog(fmt::format("(GC) {} forced room {} to {}", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force), NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s forced room %d to %s", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), to_force.c_str());

		const auto people_copy = world[ch->in_room]->people;
		for (const auto vict : people_copy) {
			if (!vict->IsNpc()
				&& GetRealLevel(vict) >= GetRealLevel(ch)
				&& !ch->IsFlagged(EPrf::kCoderinfo)) {
				continue;
			}

			act(to_vict, true, ch, nullptr, vict, kToVict);
			force_command(vict);
		}
	} else        // force all
	{
		SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
		mudlog(fmt::format("(GC) {} forced all to {}", GET_NAME(ch), to_force), NRM, std::max(kLvlGod, GET_INVIS_LEV(ch)), SYSLOG, true);
		imm_log("%s forced all to %s", GET_NAME(ch), to_force.c_str());

		for (i = descriptor_list; i; i = next_desc) {
			next_desc = i->next;

			const auto vict = i->character;
			if  (i->state != EConState::kPlaying
				|| !vict
				|| (!vict->IsNpc() && GetRealLevel(vict) >= GetRealLevel(ch)
					&& !ch->IsFlagged(EPrf::kCoderinfo))) {
				continue;
			}

			act(to_vict, true, ch, nullptr, vict.get(), kToVict);
			force_command(vict.get());
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
