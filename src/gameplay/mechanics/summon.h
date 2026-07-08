/**
 \file summon.h - a part of the Bylins engine.
 \brief issue.spellhandlers: shared summon-pipeline helpers, lifted out of magic.cpp so the
        per-spell summon handlers (in src/gameplay/handlers/) and the CastSummonAction skeleton
        can both call them.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_SUMMON_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_SUMMON_H_

#include "gameplay/magic/magic.h"   // ActionContext

class CharData;
enum class ESpell;

// Scale a summoned-minion stat off the cast competence C via the standard option-2 modifier
// formula (the same ComputeApplyModifier the affect modifiers use).
int SummonScaledStat(const ActionContext &ctx, double min, double weight, double beta, int cap = 0);

// Charm a freshly-read summoned mob to `ch`, place it in the caster's room, return the charm
// duration (callers reuse it for follow-on affects).
int FinalizeSummonedMob(CharData *ch, CharData *mob, ESpell spell_id, bool keeper);

// Guard against summoning protected targets (sanctuary / no-resurrection / horse / ...): extracts
// the mob and returns true when the summon must be aborted.
bool IsSummonTargetProtected(CharData *ch, CharData *mob, ESpell spell_id);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_SUMMON_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
