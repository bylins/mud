/**
 \file alter_acid.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: AlterAcid alter-object handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/equipment.h"
#include "utils/random.h"

namespace handlers {

EStageResult AlterAcid(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	ObjData *obj = ctx.ovict;
	const ESpell spell_id = ctx.spell_id();
	CharData *recipient = victim ? victim : ch;
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kAcidCorrodeObj).c_str(),
		false, recipient, obj, nullptr, kToChar);
	DamageObj(obj, number(GetRealLevel(ch) * 2, GetRealLevel(ch) * 4), 100);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
