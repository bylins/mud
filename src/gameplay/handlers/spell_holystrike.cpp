/**
 \file spell_holystrike.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SpellHolystrike manual-cast handler (extracted from spells.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/magic_internal.h"
#include "engine/core/obj_handler.h"

namespace handlers {

EStageResult SpellHolystrike(ActionContext &ctx) {
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
		ActionContext hs_ctx(ch, ESpell::kHolystrike, GetRealLevel(ch), {});
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
