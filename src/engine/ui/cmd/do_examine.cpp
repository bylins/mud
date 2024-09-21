/**
\file examine.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/global_objects.h"
#include "engine/core/handler.h"

void do_examine(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *tmp_char;
	ObjData *tmp_object;
	char where[kMaxInputLength];
	int where_bits = EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip | EFind::kCharInRoom | EFind::kObjExtraDesc;

	if (ch->GetPosition() < EPosition::kSleep) {
		SendMsgToChar("Виделся часто сон беспокойный...\r\n", ch);
		return;
	} else if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы ослеплены!\r\n", ch);
		return;
	}

	two_arguments(argument, arg, where);

	if (!*arg) {
		SendMsgToChar("Что вы желаете осмотреть?\r\n", ch);
		return;
	}

	if (isname(where, "земля комната room ground"))
		where_bits = EFind::kObjRoom | EFind::kCharInRoom;
	else if (isname(where, "инвентарь inventory"))
		where_bits = EFind::kObjInventory;
	else if (isname(where, "экипировка equipment"))
		where_bits = EFind::kObjEquip;

	skip_hide_on_look(ch);

	if (look_at_target(ch, argument, subcmd))
		return;

	if (isname(arg, "пентаграмма") && IS_SET(where_bits, EFind::kObjRoom)) {
		for (const auto &aff : world[ch->in_room]->affected) {
			if (aff->type == ESpell::kPortalTimer && aff->bitvector == room_spells::ERoomAffect::kNoPortalExit) {
				return;
			}
		}
	}

	if (isname(arg, "камень") && MUD::Runestones().ViewRunestone(ch, where_bits)) {
		return;
	}

	generic_find(arg, where_bits, ch, &tmp_char, &tmp_object);
	if (tmp_object) {
		if (GET_OBJ_TYPE(tmp_object) == EObjType::kLiquidContainer
			|| GET_OBJ_TYPE(tmp_object) == EObjType::kFountain
			|| GET_OBJ_TYPE(tmp_object) == EObjType::kContainer) {
			look_in_obj(ch, argument);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
