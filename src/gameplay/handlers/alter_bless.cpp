/**
 \file alter_bless.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterBless alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterBless(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	ObjData *obj = ctx.ovict;
	// Благословение прибавляет прочность и кубы, поэтому имеет смысл только для оружия и брони.
	// Раньше благословить можно было что угодно, вплоть до хлеба.
	if (obj->get_type() != EObjType::kWeapon && !ObjSystem::is_armor_type(obj)) {
		return EStageResult::kFail;
	}
	if (!obj->has_flag(EObjFlag::kBless) && (obj->get_weight() <= 5 * GetRealLevel(ch))) {
		obj->set_extra_flag(EObjFlag::kBless);
		if (obj->has_flag(EObjFlag::kNodrop)) {
			obj->unset_extraflag(EObjFlag::kNodrop);
			if (obj->get_type() == EObjType::kWeapon) {
				obj->inc_val(2);
			}
		}
		obj->add_maximum(std::max(obj->get_maximum_durability() >> 2, 1));
		obj->set_current_durability(obj->get_maximum_durability());
		obj->add_timed_spell(ESpell::kBless, -1);
		// issue #3618: вещь навсегда разошлась с прототипом (флаг, кубы, максимальная прочность) --
		// помечаем, иначе правка прототипа в olc затрет наложенное.
		obj->set_extra_flag(EObjFlag::kTransformed);
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return EStageResult::kFail;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
