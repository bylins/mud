/**
\file equipment.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/utils_char_obj.inl"

void DoEquipment(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int i, found = 0;
	skip_spaces(&argument);

	SendMsgToChar("На вас надето:\r\n", ch);
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
				SendMsgToChar(where[i], ch);
				show_obj_to_char(GET_EQ(ch, i), ch, 1, true, 1);
				found = true;
			} else {
				SendMsgToChar(where[i], ch);
				SendMsgToChar("что-то.\r\n", ch);
				found = true;
			}
		} else {
			if (utils::IsAbbr(argument, "все") || utils::IsAbbr(argument, "all")) {
				if (GET_EQ(ch, EEquipPos::kBoths))
					if ((i == EEquipPos::kWield) || (i == EEquipPos::kHold))
						continue;
				if ((i == EEquipPos::kQuiver) && (GET_EQ(ch, EEquipPos::kBoths))) {
					if (!(((GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kBoths))) == EObjType::kWeapon)
						&& (static_cast<ESkill>(GET_EQ(ch, EEquipPos::kBoths)->get_spec_param()) == ESkill::kBows)))
						continue;
				} else if (i == EEquipPos::kQuiver)
					continue;
				if (GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kHold))
					if (i == EEquipPos::kBoths)
						continue;
				if (GET_EQ(ch, EEquipPos::kShield)) {
					if ((i == EEquipPos::kHold) || (i == EEquipPos::kBoths))
						continue;
				}
				SendMsgToChar(where[i], ch);
				sprintf(buf, "%s[ Ничего ]%s\r\n", kColorBoldBlk, kColorNrm);
				SendMsgToChar(buf, ch);
				found = true;
			}
		}
	}
	if (!found) {
		if (IS_FEMALE(ch)) {
			SendMsgToChar("Костюм Евы вам очень идет :)\r\n", ch);
		} else {
			SendMsgToChar(" Вы голы, аки сокол.\r\n", ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
