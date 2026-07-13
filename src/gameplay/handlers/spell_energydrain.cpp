/**
 \file spell_energydrain.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellEnergydrain manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/magic_internal.h"

namespace handlers {

EStageResult SpellEnergydrain(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	// истощить энергию - круг 28 уровень 9 (1)
	// для всех
	int modi = 0;
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, ESpell::kEnergyDrain);
		if (!pk_agro_action(ch, victim))
			return EStageResult::kSuccess;
	}
	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
		modi += (GetRealLevel(ch) - 10);
	if (ch->IsFlagged(EPrf::kAwake))
		modi = modi - 50;

	if (ch == victim || !CalcGeneralSaving(ch, victim, ESaving::kWill, modi)) {
		for (auto spell_id = ESpell::kFirst ; spell_id <= ESpell::kLast; ++spell_id) {
			GET_SPELL_MEM(victim, spell_id) = 0;
		}
		victim->caster_level = 0;
		SendMsgToChar("Внезапно вы осознали, что у вас напрочь отшибло память.\r\n", victim);
	} else
		SendMsgToChar(MUD::SpellMessages().GetMessage(ESpell::kEnergyDrain, ESpellMsg::kNoeffect) + "\r\n", ch);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
