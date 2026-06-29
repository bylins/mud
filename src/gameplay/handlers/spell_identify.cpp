/**
 \file spell_identify.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellIdentify manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/identify.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/dungeons.h"

namespace handlers {

EStageResult SpellIdentify(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	CharData *victim = ctx.cvict;
	ObjData *obj = ctx.ovict;
	if (obj)
		MortShowObjValues(obj, ch, 100);
	else if (victim) {
		if (GET_GOD_FLAG(ch, EGf::kAllowTesterMode) && (world[ch->in_room]->vnum / 100 >= dungeons::kZoneStartDungeons)) {
			do_stat_character(ch, victim);
			return EStageResult::kSuccess;
		}
		if (victim != ch) {
			// kIdentify overrides kWrongTarget with the identify-specific text
			SendMsgToChar(MUD::SpellMessages().GetMessage(
					ESpell::kIdentify, ESpellMsg::kWrongTarget) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
		if (GetRealLevel(victim) < 3) {
			// low-level self-identify rejection on kIdentify's sheaf.
			SendMsgToChar(MUD::SpellMessages().GetMessage(
					ESpell::kIdentify, ESpellMsg::kCustomMsgOne) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
		MortShowCharValues(victim, ch, 100);
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
