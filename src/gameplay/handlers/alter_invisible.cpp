/**
 \file alter_invisible.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterInvisible alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterInvisible(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (!obj->has_flag(EObjFlag::kNoinvis) && !obj->has_flag(EObjFlag::kInvisible)) {
		obj->set_extra_flag(EObjFlag::kInvisible);
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
