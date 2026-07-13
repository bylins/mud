/**
 \file spell_fear.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellFear manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/magic_internal.h"
#include "engine/ui/cmd/do_flee.h"

namespace handlers {

EStageResult SpellFear(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	int modi = 0;
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, ESpell::kFear);
		if (!pk_agro_action(ch, victim))
			return EStageResult::kSuccess;
	}
	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
		modi += (GetRealLevel(ch) - 10);
	if (ch->IsFlagged(EPrf::kAwake))
		modi = modi - 50;
	if (AFF_FLAGGED(victim, EAffect::kBless))
		modi -= 25;

	if (!victim->IsFlagged(EMobFlag::kNoFear) && !CalcGeneralSaving(ch, victim, ESaving::kWill, modi))
		GoFlee(victim);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
