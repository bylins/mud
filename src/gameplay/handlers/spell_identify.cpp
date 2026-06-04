/**
 \file spell_identify.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellIdentify manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/identify.h"
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

EStageResult SpellIdentify(CastContext &ctx) {
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
