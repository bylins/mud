/**
 \file alter_restoration.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterRestoration alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "engine/db/obj_prototypes.h"

namespace handlers {

EStageResult AlterRestoration(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (obj->has_flag(EObjFlag::kMagic) && (obj->get_rnum() != kNothing)) {
		if (obj_proto.at(obj->get_rnum())->has_flag(EObjFlag::kMagic)) {
			return EStageResult::kSuccess;
		}
		obj->unset_enchant();
	} else {
		return EStageResult::kSuccess;
	}
	return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
