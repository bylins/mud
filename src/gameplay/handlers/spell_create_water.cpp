/**
 \file spell_create_water.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellCreateWater manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/condition.h"
#include "administration/privilege.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/core/utils_char_obj.inl"    // weight_change_object -- inline, нужен #include (не forward-decl), иначе при unity=off падает линковка

namespace handlers {

EStageResult SpellCreateWater(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	ObjData *obj = ctx.ovict;
	int water;
	if (ch == nullptr || (obj == nullptr && victim == nullptr))
		return EStageResult::kSuccess;
	// level = MAX(MIN(level, kLevelImplementator), 1);       - not used

	if (obj
		&& obj->get_type() == EObjType::kLiquidContainer) {
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
			// kCreateWater overrides kItemCreationFailToChar
			// with "Прекратите, ради бога, химичить.".
			SendMsgToChar(MUD::SpellMessages().GetMessage(
					ESpell::kCreateWater, ESpellMsg::kItemCreationFailToChar) + "\r\n", ch);
			return EStageResult::kSuccess;
		} else {
			water = std::max(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0) {
				if (GET_OBJ_VAL(obj, 1) >= 0) {
					name_from_drinkcon(obj);
				}
				obj->set_val(2, LIQ_WATER);
				obj->add_val(1, water);
				// kCreateWater overrides the generic "Вы создали $o3." (kItemCreatedToChar)
				// with "Вы наполнили $o3 водой.".
				const auto &filled_msg = MUD::SpellMessages().GetMessage(
						ESpell::kCreateWater, ESpellMsg::kItemCreatedToChar);
				act(filled_msg.c_str(), false, ch, obj, nullptr, kToChar);
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
			}
		}
	}
	if (victim && !victim->IsNpc() && !privilege::IsImmortal(victim)) {
		GET_COND(victim, condition::kThirst) = 0;
		// kCreateWater overrides kThirstToVict with "Вы полностью утолили жажду."
		// (literal text, no {intensity} expansion -- the manual path bypasses
		// CastToPoints' intensity machinery).
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kCreateWater, ESpellMsg::kThirstToVict) + "\r\n", victim);
		// The redundant "Вы напоили $N3." line to the caster was removed
		// the caster has no way to gauge the target's
		// prior thirst level, so the line conveys no real info.
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
