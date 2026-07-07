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
#include "engine/db/world_objects.h"
#include "gameplay/mechanics/liquid.h"
#include "utils/logger.h"
#include <cstdio>

namespace handlers {

EStageResult AlterRemovePoison(CastContext &ctx) {
	ObjData *obj = ctx.ovict;
	if (obj->get_rnum() < 0) {
		char message[100];
		sprintf(message, "неизвестный прототип объекта : %s (VNUM=%d)", obj->get_PName(grammar::ECase::kNom).c_str(), obj->get_vnum());
		mudlog(message, BRF, kLvlBuilder, SYSLOG, 1);
		return AlterMsg(ctx, ESpellMsg::kRemovePoisonUnknown);
	}
	// issue.potion-hotfix: remove-poison on a drink/food clears its poison LEVEL (kLiquidPoison) and
	// RESTARTS the contents freshness timer, so a spoiled potion can be salvaged and drunk again. It
	// will spoil once more after the timer runs out -- each cycle leaves it weaker (spoil_potion halves
	// the power), until it is inert. (The old "rotten/uncurable" case, keyed off the overloaded val[3],
	// is gone: spoilage poison is always curable now, by design.)
	if (obj->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison) > 0
			&& (obj->get_type() == EObjType::kLiquidContainer
				|| obj->get_type() == EObjType::kFountain
				|| obj->get_type() == EObjType::kFood)) {
		obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidPoison, -1);   // -1 removes the key (no poison)
		obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, drinkcon::kSpoiledResetTimerMinutes);
		world_objects.decay_manager().register_perishable(obj);         // resume the spoilage countdown
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
