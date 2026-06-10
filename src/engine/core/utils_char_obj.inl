#ifndef CHAR_OBJ_UTILS_HPP_
#define CHAR_OBJ_UTILS_HPP_

#include "handler.h"
#include "gameplay/mechanics/minions.h"
#include "engine/structs/structs.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "gameplay/mechanics/named_stuff.h"
#include "gameplay/mechanics/illumination.h"
#include "utils/utils.h"
#include "gameplay/mechanics/sight.h"
#include "utils/grammar/cases.h"
#include "gameplay/core/base_stats.h"
#include "engine/entities/zone.h"

extern int invalid_anti_class(CharData *ch, const ObjData *obj);

inline bool CAN_CARRY_OBJ(const CharData *ch, const ObjData *obj) {
	// для анлимного лута мобами из трупов
	if (ch->IsNpc() && !IsCharmice(ch)) {
		return true;
	}

	if (ch->GetCarryingWeight() + obj->get_weight() <= CAN_CARRY_W(ch)
		&& ch->GetCarryingQuantity() + 1 <= CAN_CARRY_N(ch)) {
		return true;
	}

	return false;
}

inline bool INVIS_OK_OBJ(const CharData *sub, const ObjData *obj) {
	return !obj->has_flag(EObjFlag::kInvisible)
		|| AFF_FLAGGED(sub, EAffect::kDetectInvisible);
}

inline const char *OBJN(const ObjData *obj, const CharData *vict, const grammar::ECase name_case) {
	return sight::CanSeeObj(vict, obj)
		   ? (!obj->get_PName(name_case).empty()
			  ? obj->get_PName(name_case).c_str()
			  : obj->get_short_description().c_str())
		   : grammar::SomethingInCase(name_case);
}

inline const char *OBJS(const ObjData *obj, const CharData *vict) {
	return sight::CanSeeObj(vict, obj) ? obj->get_short_description().c_str() : "что-то";
}

inline bool CAN_GET_OBJ(const CharData *ch, const ObjData *obj) {
	return (CAN_WEAR(obj, EWearFlag::kTake)
		&& CAN_CARRY_OBJ(ch, obj)
		&& sight::CanSeeObj(ch, obj))
		&& !(ch->IsNpc()
			&& obj->has_flag(EObjFlag::kBloody));
}

inline bool CanTakeObj(CharData *ch, ObjData *obj) {
	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)
		&& obj->get_type() != EObjType::kMoney) {
		act("$p: Вы не могете нести столько вещей.", false, ch, obj, nullptr, kToChar);
		return false;
	} else if ((ch->GetCarryingWeight() + obj->get_weight()) > CAN_CARRY_W(ch)
		&& obj->get_type() != EObjType::kMoney) {
		act("$p: Вы не в состоянии нести еще и $S.", false, ch, obj, nullptr, kToChar);
		return false;
	} else if (!(CAN_WEAR(obj, EWearFlag::kTake))) {
		act("$p: Вы не можете взять $S.", false, ch, obj, nullptr, kToChar);
		return false;
	} else if (invalid_anti_class(ch, obj)) {
		act("$p: Эта вещь не предназначена для вас!", false, ch, obj, nullptr, kToChar);
		return false;
	} else if (NamedStuff::check_named(ch, obj, false)) {
		if (!NamedStuff::wear_msg(ch, obj))
			act("$p: Эта вещь не предназначена для вас!", false, ch, obj, nullptr, kToChar);
		return false;
	}
	return true;
}

inline void weight_change_object(ObjData *obj, int weight) {
	ObjData *tmp_obj;
	CharData *tmp_ch;

	if (obj->get_in_room() != kNowhere) {
		obj->set_weight(std::max(1, obj->get_weight() + weight));
	} else if ((tmp_ch = obj->get_carried_by())) {
		RemoveObjFromChar(obj);
		obj->set_weight(std::max(1, obj->get_weight() + weight));
		PlaceObjToInventory(obj, tmp_ch);
	} else if ((tmp_obj = obj->get_in_obj())) {
		RemoveObjFromObj(obj);
		obj->set_weight(std::max(1, obj->get_weight() + weight));
		PlaceObjIntoObj(obj, tmp_obj);
	} else {
		log("SYSERR: Unknown attempt to subtract weight from an object.");
	}
}

inline bool InTestZone(CharData *ch) {
	Rnum zrn = world[ch->in_room]->zone_rn;
	if ((ch->player_specials->saved.olc_zone == zone_table[zrn].vnum) || zone_table[zrn].under_construction)
		return true;
	return false;
}

#endif // __CHAR_OBJ_UTILS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :