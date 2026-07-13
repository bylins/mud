/**
 \file alter_repair.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterRepair alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterRepair(ActionContext &ctx) {
	ctx.ovict->set_current_durability(ctx.ovict->get_maximum_durability());
	return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
