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

// Per-(action, target) pipeline: cast gates + the cursor action's data-driven stages on
// ctx.cvict. is_entry (the spell's first action) also runs the whole-cast steps (reflection,
// mtrigger, one-shots, ReactToCast). The caster_conditions gate lives in CallMagic.
ECastResult CastOnTarget(CastContext &ctx, bool is_entry);

// Main cast entry: walks the spell's <action> list, resolving and running each action over its
// own target list (action[0] = the spell scope). Unifies the former CallMagicToArea/Group and
// the single-target path. Per-action target resolving (issue.area-cast).
ECastResult CastSpell(CastContext &ctx, ECastTargets scope);

// Forced area-fanout of a spell over the caster's room (room-affect ticks); see magic.cpp.
ECastResult CastAreaInRoom(CharData *ch, ESpell spell_id, int level);
// Run a single, cycled action (action[phase % N]) of a kService tick spell on the room.
ECastResult CastRoomTickAction(CharData *ch, RoomData *room, ESpell tick_spell, int phase);

// Build a CastContext for a cast: evaluates the success + potency rolls once. Module-internal
// (CallMagic is the public entry; CastAreaInRoom uses it for the room-affect ticks).
CastContext BuildCastContext(CharData *caster, ESpell spell_id, int level, float fixed_potency = -1.0f,
		double fixed_noise_z = std::numeric_limits<double>::quiet_NaN(), int fixed_skill = -1);

// Spell-level caster gate (issue.spell-unification): true if the caster fails the
// spell's <caster_conditions> -- carries a <blocking> flag/affect/align, or lacks a
// <required> one. Checked once in CallMagic before per-target dispatch.
bool CasterBlocked(CharData *caster, const talents_actions::CasterConditions &cc);

// Per-stage dispatchers reached from CastToSingleTarget via the kMag* flag table.
EStageResult CastUnaffects(CastContext &ctx);
EStageResult CastToPoints(CastContext &ctx);
EStageResult CastToAlterObjs(CastContext &ctx);
EStageResult CastManual(CastContext &ctx);

// Material-component check + class anti-saving modifier: only used inside the module.
EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id);
int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id);

#endif  // MAGIC_INTERNAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
