/**
 \file alter_curse.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterCurse alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterCurse(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (!obj->has_flag(EObjFlag::kNodrop)) {
		obj->set_extra_flag(EObjFlag::kNodrop);
		if (obj->get_type() == EObjType::kWeapon) {
			if (GET_OBJ_VAL(obj, 2) > 0) {
				obj->dec_val(2);
			}
		} else if (ObjSystem::is_armor_type(obj)) {
			if (GET_OBJ_VAL(obj, 0) > 0) {
				obj->dec_val(0);
			}
			if (GET_OBJ_VAL(obj, 1) > 0) {
				obj->dec_val(1);
			}
		}
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
