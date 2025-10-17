/**
\file do_gen_door.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/doors.h"
#include "utils/utils.h"
#include "engine/core/handler.h"

void do_gen_door(CharData *ch, char *argument, int, int subcmd) {
	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Очнитесь, вы же слепы!\r\n", ch);
		return;
	}

	if (subcmd == kScmdPick && !ch->GetSkill(ESkill::kPickLock)) {
		SendMsgToChar("Это умение вам недоступно.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	if (!*argument) {
		sprintf(buf, "%s что?\r\n", a_cmd_door[subcmd]);
		SendMsgToChar(utils::CAP(buf), ch);
		return;
	}
	char type[kMaxInputLength], dir[kMaxInputLength];
	two_arguments(argument, type, dir);
	int where_bits = EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip;
	if (isname(dir, "земля комната room ground"))
		where_bits = EFind::kObjRoom;
	else if (isname(dir, "инвентарь inventory"))
		where_bits = EFind::kObjInventory;
	else if (isname(dir, "экипировка equipment"))
		where_bits = EFind::kObjEquip;

	go_gen_door(ch, type, dir, where_bits, subcmd);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
