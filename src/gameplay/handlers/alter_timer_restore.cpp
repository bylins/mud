/**
 \file alter_timer_restore.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterTimerRestore alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "engine/db/obj_prototypes.h"
#include "utils/logger.h"

namespace handlers {

EStageResult AlterTimerRestore(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	ObjData *obj = ctx.ovict;
	if (obj->get_rnum() != kNothing) {
		obj->set_current_durability(obj->get_maximum_durability());
		obj->set_timer(obj_proto.at(obj->get_rnum())->get_timer());
		log("%s used magic repair", GET_NAME(ch));
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kSuccess;  // rnum==kNothing: silent no-op (matches old early return)
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
