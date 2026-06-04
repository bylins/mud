/**
 \file spell_energydrain.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellEnergydrain manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/magic/spells.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/magic_internal.h"
#include "engine/db/global_objects.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/cmd/do_hire.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/skills/animal_master.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "engine/ui/color.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/minions.h"
#include <cmath>

namespace handlers {

EStageResult SpellEnergydrain(CastContext &ctx) {
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
