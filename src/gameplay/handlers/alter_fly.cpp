/**
 \file alter_fly.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterFly alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterFly(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	obj->add_timed_spell(ESpell::kFly, -1);
	obj->set_extra_flag(EObjFlag::kFlying);
	// issue #3618: таймер -1 -- это бессрочно, флаг лежит в самой вещи и переживает перезагрузку.
	obj->set_extra_flag(EObjFlag::kTransformed);
	return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
