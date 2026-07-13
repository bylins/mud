/**
\file examine.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "utils/grammar/declensions.h"
#include "engine/entities/obj_data.h"
#include "gameplay/mechanics/sight.h"
#include "engine/db/global_objects.h"
#include "engine/core/target_resolver.h"
#include "gameplay/affects/affect_messages.h"

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

	sight::skip_hide_on_look(ch);

	{
		ObjData *supp_obj = nullptr;
		CharData *supp_ch = nullptr;
		generic_find(arg, where_bits, ch, &supp_ch, &supp_obj);
		const bool did_look = sight::look_at_target(ch, argument, subcmd);
		if (supp_obj && supp_obj->has_suppressed_affects()) {
			// One suppressed affect per indented line -- a single comma-joined line is unreadable with 2+.
			std::string note = "Временно подавлено волшебство:\r\n";
			for (const auto &pr : supp_obj->suppressed_equip_affects()) {
				char hbuf[96];
				snprintf(hbuf, sizeof(hbuf), "    %s (%d %s)\r\n",
						affects::AffectMsg(pr.first, affects::EAffectMsgType::kShortDesc).c_str(), pr.second,
						grammar::GetDeclensionInNumber(pr.second, grammar::EWhat::kHour));
				note += hbuf;
			}
			SendMsgToChar(note, ch);
		}
		if (did_look) {
			return;
		}
	}

	if (isname(arg, "пентаграмма") && IS_SET(where_bits, EFind::kObjRoom)) {
		for (const auto &aff : world[ch->in_room]->affected) {
			if (aff->affect_type == room_spells::ERoomAffect::kNoPortalExit) {
				return;
			}
		}
	}
	generic_find(arg, where_bits, ch, &tmp_char, &tmp_object);
	if (tmp_object) {
		if (tmp_object->get_type() == EObjType::kLiquidContainer
			|| tmp_object->get_type() == EObjType::kFountain
			|| tmp_object->get_type() == EObjType::kContainer) {
			sight::look_in_obj(ch, argument);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
