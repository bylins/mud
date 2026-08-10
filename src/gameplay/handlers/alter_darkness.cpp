/**
 \file alter_darkness.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterDarkness alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/affects/obj_affects.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterDarkness(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (obj_affects::Has(obj, obj_affects::EObjAffect::kLight)) {
		obj_affects::Remove(obj, obj_affects::EObjAffect::kLight, true);
		return EStageResult::kSuccess;  // silent (matches old early return)
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
