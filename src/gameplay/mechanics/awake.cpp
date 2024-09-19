/**
\file awake.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Механика awake - флагов блестит, шумит и так далее
\detail Detail description.
*/

#include "gameplay/mechanics/awake.h"

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/illumination.h"

int awake_invis(CharData *ch);

extern bool IsWearingLight(CharData *ch);

int check_awake(CharData *ch, int what) {
	int i, retval = 0, wgt = 0;

	if (!IS_GOD(ch)) {
		if (IS_SET(what, kAcheckAffects)
			&& (AFF_FLAGGED(ch, EAffect::kStairs) || AFF_FLAGGED(ch, EAffect::kSanctuary)))
			SET_BIT(retval, kAcheckAffects);

		if (IS_SET(what, kAcheckLight) &&
			IsDefaultDark(ch->in_room)
			&& (AFF_FLAGGED(ch, EAffect::kSingleLight) || AFF_FLAGGED(ch, EAffect::kHolyLight)))
			SET_BIT(retval, kAcheckLight);

		for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (!GET_EQ(ch, i))
				continue;

			if (IS_SET(what, kAcheckHumming) && GET_EQ(ch, i)->has_flag(EObjFlag::kHum))
				SET_BIT(retval, kAcheckHumming);

			if (IS_SET(what, kAcheckGlowing) && GET_EQ(ch, i)->has_flag(EObjFlag::kGlow))
				SET_BIT(retval, kAcheckGlowing);

			if (IS_SET(what, kAcheckLight)
				&& IsDefaultDark(ch->in_room)
				&& GET_OBJ_TYPE(GET_EQ(ch, i)) == EObjType::kLightSource
				&& GET_OBJ_VAL(GET_EQ(ch, i), 2)) {
				SET_BIT(retval, kAcheckLight);
			}

			if (ObjSystem::is_armor_type(GET_EQ(ch, i))
				&& GET_OBJ_MATER(GET_EQ(ch, i)) <= EObjMaterial::kPreciousMetel) {
				wgt += GET_OBJ_WEIGHT(GET_EQ(ch, i));
			}
		}

		if (IS_SET(what, kAcheckWeight) && wgt > GetRealStr(ch) * 2)
			SET_BIT(retval, kAcheckWeight);
	}
	return (retval);
}

int awake_hide(CharData *ch) {
	if (IS_GOD(ch))
		return (false);
	return check_awake(ch, kAcheckAffects | kAcheckLight | kAcheckHumming
		| kAcheckGlowing | kAcheckWeight);
}

int awake_sneak(CharData *ch) {
	return awake_hide(ch);
}

int awake_invis(CharData *ch) {
	if (IS_GOD(ch))
		return (false);
	return check_awake(ch, kAcheckAffects | kAcheckLight | kAcheckHumming
		| kAcheckGlowing);
}

int awake_camouflage(CharData *ch) {
	return awake_invis(ch);
}

int awaking(CharData *ch, int mode) {
	if (IS_GOD(ch))
		return (false);
	if (IS_SET(mode, kAwHide) && awake_hide(ch))
		return (true);
	if (IS_SET(mode, kAwInvis) && awake_invis(ch))
		return (true);
	if (IS_SET(mode, kAwCamouflage) && awake_camouflage(ch))
		return (true);
	if (IS_SET(mode, kAwSneak) && awake_sneak(ch))
		return (true);
	return (false);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
