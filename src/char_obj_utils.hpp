#ifndef __CHAR_OBJ_UTILS_HPP__
#define __CHAR_OBJ_UTILS_HPP__

#include "structs.h"
#include "utils.h"
#include "char.hpp"
#include "obj.hpp"

inline bool INVIS_OK_OBJ(const CHAR_DATA* sub, const OBJ_DATA* obj)
{
	return !obj->get_extraflag(EExtraFlags::ITEM_INVISIBLE)
		|| AFF_FLAGGED(sub, AFF_DETECT_INVIS);
}

inline bool MORT_CAN_SEE_OBJ(const CHAR_DATA* sub, const OBJ_DATA* obj)
{
	return INVIS_OK_OBJ(sub, obj)
		&& !AFF_FLAGGED(sub, AFF_BLIND)
		&& (IS_LIGHT(IN_ROOM(sub))
			|| OBJ_FLAGGED(obj, EExtraFlags::ITEM_GLOW)
			|| (IS_CORPSE(obj)
				&& AFF_FLAGGED(sub, AFF_INFRAVISION))
			|| can_use_feat(sub, DARK_READING_FEAT));
}

inline bool CAN_SEE_OBJ(const CHAR_DATA* sub, const OBJ_DATA* obj)
{
	return (obj->worn_by == sub
		|| obj->carried_by == sub
		|| (obj->in_obj
			&& (obj->in_obj->worn_by == sub
				|| obj->in_obj->carried_by == sub))
		|| MORT_CAN_SEE_OBJ(sub, obj)
		|| (!IS_NPC(sub)
			&& PRF_FLAGGED((sub), PRF_HOLYLIGHT)));
}

const char* OBJN(const OBJ_DATA* obj, const CHAR_DATA* vict, const size_t pad)
{
	return CAN_SEE_OBJ(vict, obj)
		? (obj->PNames[pad]
			? obj->PNames[pad]
			: obj->short_description)
		: GET_PAD_OBJ(pad);
}

const char* OBJS(const OBJ_DATA* obj, const CHAR_DATA* vict)
{
	return CAN_SEE_OBJ(vict, obj) ? obj->short_description : "что-то";
}

#endif // __CHAR_OBJ_UTILS_HPP__