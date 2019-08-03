#ifndef __CHAR_OBJ_UTILS_HPP__
#define __CHAR_OBJ_UTILS_HPP__

#include "structs.h"
#include "char.hpp"
#include "obj.hpp"
#include "utils.h"

inline bool INVIS_OK_OBJ(const CHAR_DATA* sub, const OBJ_DATA* obj)
{
	return !obj->get_extra_flag(EExtraFlag::ITEM_INVISIBLE)
		|| AFF_FLAGGED(sub, EAffectFlag::AFF_DETECT_INVIS);
}

inline bool MORT_CAN_SEE_OBJ(const CHAR_DATA* sub, const OBJ_DATA* obj)
{
	return INVIS_OK_OBJ(sub, obj)
		&& !AFF_FLAGGED(sub, EAffectFlag::AFF_BLIND)
		&& (IS_LIGHT(obj->get_in_room())
			|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_GLOW)
			|| (IS_CORPSE(obj)
				&& AFF_FLAGGED(sub, EAffectFlag::AFF_INFRAVISION))
			|| can_use_feat(sub, DARK_READING_FEAT));
}

inline bool CAN_SEE_OBJ(const CHAR_DATA* sub, const OBJ_DATA* obj)
{
	return (obj->get_worn_by() == sub
		|| obj->get_carried_by() == sub
		|| (obj->get_in_obj()
			&& (obj->get_in_obj()->get_worn_by() == sub
				|| obj->get_in_obj()->get_carried_by() == sub))
		|| MORT_CAN_SEE_OBJ(sub, obj)
		|| (!IS_NPC(sub)
			&& PRF_FLAGGED((sub), PRF_HOLYLIGHT)));
}

inline const char* OBJN(const OBJ_DATA* obj, const CHAR_DATA* vict, const size_t pad)
{
	return CAN_SEE_OBJ(vict, obj)
		? (!obj->get_PName(pad).empty()
			? obj->get_PName(pad).c_str()
			: obj->get_short_description().c_str())
		: GET_PAD_OBJ(pad);
}

inline const char* OBJS(const OBJ_DATA* obj, const CHAR_DATA* vict)
{
	return CAN_SEE_OBJ(vict, obj) ? obj->get_short_description().c_str() : "что-то";
}

inline bool CAN_GET_OBJ(const CHAR_DATA* ch, const OBJ_DATA* obj)
{
	return (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_TAKE)
		&& CAN_CARRY_OBJ(ch, obj)
		&& CAN_SEE_OBJ(ch, obj))
		&& !(IS_NPC(ch)
			&& obj->get_extra_flag(EExtraFlag::ITEM_BLOODY));
}

#endif // __CHAR_OBJ_UTILS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :