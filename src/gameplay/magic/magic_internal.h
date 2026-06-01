/*
 * magic_internal.h -- declarations shared between magic.cpp and magic_utils.cpp for the
 * stage functions that aren't part of the public spell-cast surface. Don't include from
 * outside the magic module; use CallMagic / CastSpell in magic.h instead.
 *
 * These functions are the per-stage dispatchers reached from CallMagic via the kMag*
 * flag table. CastToTargets is the main entry (it fans out to the target list and runs
 * CastOnTarget per target) with
 * the blocking / required / reflection / caster_blocking gates layered on top. Calling
 * any of them directly skips those gates, which is why they don't sit in magic.h.
 */
#ifndef MAGIC_INTERNAL_H_
#define MAGIC_INTERNAL_H_

#include "magic.h"          // for EStageResult / ECastResult / CastContext / forward decls

// issue.area-cast: target scope for a cast. kSingle -> ctx.cvict only; kFoes -> every foe in
// the caster's room (area/mass); kFriends -> the caster's group, caster placed last.
enum class ECastTargets { kSingle, kFoes, kFriends };

// Per-target pipeline: runs the stages in order with the target blocking/required/reflection
// gates on top (the caster_conditions gate lives in CallMagic). One target = ctx.cvict.
ECastResult CastOnTarget(CastContext &ctx);

// Main cast entry: builds the target list (per scope) and runs the pipeline on each, applying
// the area <distribution> falloff. Unifies the former CallMagicToArea + CallMagicToGroup.
ECastResult CastToTargets(CastContext &ctx, ECastTargets scope);

// Forced area-fanout of a spell over the caster's room (room-affect ticks); see magic.cpp.
ECastResult CastAreaInRoom(CharData *ch, ESpell spell_id, int level);

// Builds a CastContext (evaluates both rolls). Module-internal; CallMagic is the entry.
CastContext ComputeCastRoll(CharData *caster, ESpell spell_id, int level);

// Spell-level caster gate (issue.spell-unification): true if the caster fails the
// spell's <caster_conditions> -- carries a <blocking> flag/affect/align, or lacks a
// <required> one. Checked once in CallMagic before per-target dispatch.
bool CasterBlocked(CharData *caster, const talents_actions::CasterConditions &cc);

// Per-stage dispatchers reached from CastToSingleTarget via the kMag* flag table.
EStageResult CastUnaffects(CastContext &ctx);
EStageResult CastToPoints(CastContext &ctx);
EStageResult CastToAlterObjs(CastContext &ctx);
EStageResult CastCreation(CastContext &ctx);
EStageResult CastSummon(CastContext &ctx, bool need_fail);
EStageResult CastManual(CastContext &ctx);

// Material-component check + class anti-saving modifier: only used inside the module.
EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id);
int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id);

#endif  // MAGIC_INTERNAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
