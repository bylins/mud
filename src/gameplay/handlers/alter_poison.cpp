/**
 \file alter_poison.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterPoison alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterPoison(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (!GET_OBJ_VAL(obj, 3) && (obj->get_type() == EObjType::kLiquidContainer
			|| obj->get_type() == EObjType::kFountain || obj->get_type() == EObjType::kFood)) {
		obj->set_val(3, 1);
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
