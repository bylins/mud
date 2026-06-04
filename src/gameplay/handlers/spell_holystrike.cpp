/**
 \file spell_holystrike.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellHolystrike manual-cast handler (extracted from spells.cpp).
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

EStageResult SpellHolystrike(CastContext &ctx) {
	CharData *ch = ctx.caster();
	const char *msg1 = "Земля под вами засветилась и всех поглотил плотный туман.";
	const char *msg2 = "Вдруг туман стал уходить обратно в землю, забирая с собой тела поверженных.";

	act(msg1, false, ch, nullptr, nullptr, kToChar);
	act(msg1, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

	const auto people_copy = world[ch->in_room]->people;
	for (const auto tch : people_copy) {
		if (tch->IsNpc()) {
			if (!tch->IsFlagged(EMobFlag::kCorpse)
				&& GET_RACE(tch) != ENpcRace::kZombie
				&& GET_RACE(tch) != ENpcRace::kBoggart) {
				continue;
			}
		} else {
			//Чуток нелогично, но раз зомби гоняет -- сам немного мертвяк. :)
			//Тут сам спелл бредовый... Но пока на скорую руку.
			if (!CanUseFeat(tch, EFeat::kZombieDrover)) {
				continue;
			}
		}

		// per-target order is Damage -> Unaffect -> Affect (matches
		// CallMagic's stage order). kHolystrike's <unaffect> dispels kEviless on the target and
		// returns kBreak, which skips the hold imposition for that just-dispelled minion. Damage
		// always lands first, so the high-damage replacement for the old instant_death still
		// bites kEviless minions on the way through.
		CastContext hs_ctx(ch, ESpell::kHolystrike, GetRealLevel(ch), {}, {});
		hs_ctx.cvict = tch;
		CastDamage(hs_ctx);
		if (CastUnaffects(hs_ctx) == EStageResult::kBreak) {
			continue;
		}
		CastAffect(hs_ctx);
	}

	act(msg2, false, ch, nullptr, nullptr, kToChar);
	act(msg2, false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

	ObjData *o = nullptr;
	do {
		for (auto o : world[ch->in_room]->contents) {
			if (!IS_CORPSE(o)) {
				continue;
			}

			ExtractObjFromWorld(o);

			break;
		}
	} while (o);
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
