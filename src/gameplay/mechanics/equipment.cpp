/**
\file equipment.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "equipment.h"

#include "engine/entities/obj_data.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "utils/random.h"
#include "engine/core/handler.h"
#include "gameplay/core/constants.h"

void DamageEquipment(CharData *ch, int pos, int dam, int chance) {
	// calculate drop_chance if
	if (pos == kNowhere) {
		pos = number(0, 100);
		if (pos < 3)
			pos = EEquipPos::kFingerR + number(0, 1);
		else if (pos < 6)
			pos = EEquipPos::kNeck + number(0, 1);
		else if (pos < 20)
			pos = EEquipPos::kBody;
		else if (pos < 30)
			pos = EEquipPos::kHead;
		else if (pos < 45)
			pos = EEquipPos::kLegs;
		else if (pos < 50)
			pos = EEquipPos::kFeet;
		else if (pos < 58)
			pos = EEquipPos::kHands;
		else if (pos < 66)
			pos = EEquipPos::kArms;
		else if (pos < 76)
			pos = EEquipPos::kShield;
		else if (pos < 86)
			pos = EEquipPos::kShoulders;
		else if (pos < 90)
			pos = EEquipPos::kWaist;
		else if (pos < 94)
			pos = EEquipPos::kWristR + number(0, 1);
		else
			pos = EEquipPos::kHold;
	}

	if (pos <= 0 || pos > EEquipPos::kBoths || !GET_EQ(ch, pos) || dam < 0 || AFF_FLAGGED(ch, EAffect::kGodsShield)) {
		return;
	}
	DamageObj(GET_EQ(ch, pos), dam, chance);
}

// * Alterate equipment
// По-хорошему, нужна отдельная механика прочности предметов и эту функцию нужно перенести туда
void DamageObj(ObjData *obj, int dam, int chance) {
	if (!obj)
		return;
	dam = number(0, dam * (material_value[obj->get_material()] + 30) /
		std::max(1, obj->get_maximum_durability() *
			(obj->has_flag(EObjFlag::kNodrop) ? 5 :
			 obj->has_flag(EObjFlag::kBless) ? 15 : 10)
			* (static_cast<ESkill>(obj->get_spec_param()) == ESkill::kBows ? 3 : 1)));

	if (dam > 0 && chance >= number(1, 100)) {
		if (dam > 1 && obj->get_worn_by() && GET_EQ(obj->get_worn_by(), EEquipPos::kShield) == obj) {
			dam /= 2;
		}

		obj->sub_current(dam);
		if (obj->get_current_durability() <= 0) {
			if (obj->get_worn_by()) {
				snprintf(buf, kMaxStringLength, "$o%s рассыпал$U, не выдержав повреждений.",
						 char_get_custom_label(obj, obj->get_worn_by()).c_str());
				act(buf, false, obj->get_worn_by(), obj, nullptr, kToChar);
			} else if (obj->get_carried_by()) {
				snprintf(buf, kMaxStringLength, "$o%s рассыпал$U, не выдержав повреждений.",
						 char_get_custom_label(obj, obj->get_carried_by()).c_str());
				act(buf, false, obj->get_carried_by(), obj, nullptr, kToChar);
			}
			ExtractObjFromWorld(obj);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
