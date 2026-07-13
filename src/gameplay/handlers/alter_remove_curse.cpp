/**
 \file alter_remove_curse.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterRemoveCurse alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterRemoveCurse(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (obj->has_flag(EObjFlag::kNodrop)) {
		obj->unset_extraflag(EObjFlag::kNodrop);
		if (obj->get_type() == EObjType::kWeapon) {
			obj->inc_val(2);
		}
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
