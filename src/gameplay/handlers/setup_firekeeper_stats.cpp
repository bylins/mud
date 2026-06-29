/**
 \file setup_firekeeper_stats.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SetupFirekeeperStats summon post-spawn handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/summon.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/minions.h"

namespace handlers {

void SetupFirekeeperStats(CharData *ch, CharData *mob, const ActionContext &ctx, int charm_duration) {
	Affect<EApply> af;
	af.duration = charm_duration;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = kAfCharmBond;
	af.affect_type = (get_effective_cha(ch) >= 30) ? EAffect::kFireShield : EAffect::kFireAura;
	affect_to_char(mob, af);
	mob->SetFlag(EMobFlag::kSummoned);	// true conjuration (banishable)
	mob->SetFlag(EMobFlag::kCompanion);	// any NPC ally

	// Cha-driven stats, calibrated onto C = base_cha-20 (kSummonFirekeeper potency: kCha
	// threshold=20 weight=100) via the same SummonScaledStat/option-2 formula; each cap
	// reproduces the old clamp(effective_cha-20, 0, 30) ceiling at cha 50. No new formula.
	GET_DR(mob) = SummonScaledStat(ctx, 10, 0.0, 1.5, 55);
	GET_NDD(mob) = 1;
	GET_SDD(mob) = SummonScaledStat(ctx, 1, 0.0, 0.2, 7);
	mob->mob_specials.extra_attack = 0;

	// old: 300 + number(modifier*12, modifier*16) -- the random spread collapses to the average.
	const int m = SummonScaledStat(ctx, 300, 0.0, 14.0, 720);
	mob->set_hit(m);
	mob->set_max_hit(m);
	SetSkill(mob, ESkill::kAwake, SummonScaledStat(ctx, 50, 0.0, 2.0, 110));
	mob->SetFlag(EPrf::kAwake);
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
