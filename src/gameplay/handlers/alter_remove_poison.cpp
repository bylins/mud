/**
 \file alter_remove_poison.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterRemovePoison alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "engine/db/obj_prototypes.h"
#include "utils/logger.h"
#include <cstdio>

namespace handlers {

EStageResult AlterRemovePoison(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (obj->get_rnum() < 0) {
		char message[100];
		sprintf(message, "неизвестный прототип объекта : %s (VNUM=%d)", obj->get_PName(grammar::ECase::kNom).c_str(), obj->get_vnum());
		mudlog(message, BRF, kLvlBuilder, SYSLOG, 1);
		return AlterMsg(ctx, ESpellMsg::kRemovePoisonUnknown);
	}
	if (obj_proto[obj->get_rnum()]->get_val(3) > 1 && GET_OBJ_VAL(obj, 3) == 1) {
		return AlterMsg(ctx, ESpellMsg::kRemovePoisonRotten);
	}
	if ((GET_OBJ_VAL(obj, 3) == 1) && (obj->get_type() == EObjType::kLiquidContainer
			|| obj->get_type() == EObjType::kFountain || obj->get_type() == EObjType::kFood)) {
		obj->set_val(3, 0);
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
