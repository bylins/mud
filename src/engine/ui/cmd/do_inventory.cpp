/**
\file inventory.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"

void DoInventory(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("Вы несете:\r\n", ch);
	list_obj_to_char(ch->carrying, ch, 1, 2);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
