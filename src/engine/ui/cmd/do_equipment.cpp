/**
\file equipment.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "utils/grammar/declensions.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/sight.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/affects/affect_messages.h"

void DoEquipment(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int i, found = 0;
	skip_spaces(&argument);

	SendMsgToChar("На вас надето:\r\n", ch);
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			if (sight::CanSeeObj(ch, GET_EQ(ch, i))) {
				SendMsgToChar(where[i], ch);
				sight::show_obj_to_char(GET_EQ(ch, i), ch, 1, true, 1);
				if (GET_EQ(ch, i)->has_suppressed_affects()) {
					std::string note = "        &G(волшебство подавлено: ";
					bool first = true;
					for (const auto &pr : GET_EQ(ch, i)->suppressed_affects()) {
						if (!first) { note += ", "; }
						first = false;
						char hbuf[96];
					snprintf(hbuf, sizeof(hbuf), "%s (%d %s)",
							affects::AffectMsg(pr.first, affects::EAffectMsgType::kShortDesc).c_str(), pr.second,
							grammar::GetDeclensionInNumber(pr.second, grammar::EWhat::kHour));
					note += hbuf;
					}
					note += ")&n\r\n";
					SendMsgToChar(note, ch);
				}
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
					if (!((GET_EQ(ch, EEquipPos::kBoths)->get_type() == EObjType::kWeapon)
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
		if (IsFemale(ch)) {
			SendMsgToChar("Костюм Евы вам очень идет :)\r\n", ch);
		} else {
			SendMsgToChar(" Вы голы, аки сокол.\r\n", ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
