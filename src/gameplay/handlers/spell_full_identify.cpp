/**
 \file spell_full_identify.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellFullIdentify manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/identify.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult SpellFullIdentify(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	ObjData *obj = ctx.ovict;
	if (obj)
		MortShowObjValues(obj, ch, 400);
	else if (victim) {
		// kFullIdentify overrides kWrongTarget with the identify-specific text
		SendMsgToChar(MUD::SpellMessages().GetMessage(
				ESpell::kFullIdentify, ESpellMsg::kWrongTarget) + "\r\n", ch);
			return EStageResult::kSuccess;
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
