/**
 \file setup_keeper_stats.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: SetupKeeperStats summon post-spawn handler (extracted from magic.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/summon.h"

namespace handlers {

void SetupKeeperStats(CharData *ch, CharData *mob, const ActionContext &ctx) {
	mob->set_level(GetRealLevel(ch));
	const int hp = SummonScaledStat(ctx, 50, 1.0, 164.4);   // + the 10d10 potency dice (old RollDices(10,10))
	mob->set_hit(hp);
	mob->set_max_hit(hp);
	SetSkill(mob, ESkill::kPunch,  SummonScaledStat(ctx, 10, 0.0, 41.1));
	SetSkill(mob, ESkill::kRescue, SummonScaledStat(ctx, 50, 0.0, 27.4));
	mob->set_str(SummonScaledStat(ctx, 3,  0.0, 5.5));
	mob->set_dex(SummonScaledStat(ctx, 10, 0.0, 5.5));
	mob->set_con(SummonScaledStat(ctx, 10, 0.0, 5.5));
	GET_HR(mob) = SummonScaledStat(ctx, 0, 0.0, 12.4);
	GET_AC(mob) = 100 - SummonScaledStat(ctx, 0, 0.0, 72.6);
	mob->SetFlag(EMobFlag::kSummoned);	// true conjuration (banishable)
	mob->SetFlag(EMobFlag::kCompanion);	// any NPC ally
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
