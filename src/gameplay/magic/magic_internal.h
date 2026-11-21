/*
 * magic_internal.h -- declarations shared within the magic module for the stage functions
 * that aren't part of the public spell-cast surface. Don't include from outside the magic
 * module; use CallMagic / CastSpell in magic.h instead.
 *
 * CastSpell is the main entry: it fans out to the target list and runs CastOnTarget per
 * target, with the blocking / required / reflection gates layered on top (caster_conditions
 * is checked in CallMagic). Calling a stage directly skips those gates, which is why they
 * don't sit in magic.h.
 */
#ifndef MAGIC_INTERNAL_H_
#define MAGIC_INTERNAL_H_

#include "magic.h"
#include "gameplay/abilities/feats.h"
#include "affect_actions.h"

// Target scope for a cast. kSingle -> ctx.cvict only; kFoes -> every foe in
// the caster's room (area/mass); kFriends -> the caster's group, caster placed last.
enum class ECastTargets { kSingle, kFoes, kFriends };

// Per-(action, target) pipeline: cast gates + the cursor action's data-driven stages on
// ctx.cvict. is_entry (the spell's first action) also runs the whole-cast steps (reflection,
// mtrigger, one-shots, ReactToCast). The caster_conditions gate lives in CallMagic.
ECastResult CastOnTarget(ActionContext &ctx, bool is_entry);

// Main cast entry: walks the spell's <action> list, resolving and running each action over its
// own target list (action[0] = the spell scope).
ECastResult CastSpell(ActionContext &ctx, ECastTargets scope);

// Forced area-fanout of a spell over the caster's room (room-affect ticks); see magic.cpp.
ECastResult CastAreaInRoom(CharData *ch, ESpell spell_id, int level);

// RunRoomEntryTriggers (the on-entry dispatcher) is declared in room_affects.h, since the movement code
// calls it; it is defined in affect_triggers.cpp with the other affect-owned actions.

// Build an ActionContext for a cast: evaluates the potency roll once. Module-internal
// (CallMagic is the public entry).
ActionContext BuildActionContext(CharData *caster, ESpell spell_id, int level, float fixed_potency = -1.0f,
								 float fixed_competence = -1.0f,
								 double fixed_noise_z = std::numeric_limits<double>::quiet_NaN(), int fixed_skill = -1);

// Spell-level caster gate: true if the caster fails the
// spell's <caster_conditions> -- carries a <blocking> flag/affect/align, or lacks a
// <required> one. Checked once in CallMagic before per-target dispatch.
bool CasterBlocked(CharData *caster, const talents_actions::CasterConditions &cc);

// Per-stage dispatchers reached from CastOnTarget via the kMag* flags.
EStageResult CastUnaffects(ActionContext &ctx);
EStageResult CastToPoints(ActionContext &ctx);
EStageResult CastToAlterObjs(ActionContext &ctx);
EStageResult CastManual(ActionContext &ctx);

EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id);

// Affect wards (affect_wards.cpp). RunWholeCastWards: reflection / absorb-all wards, evaluated once
// for the top-level cast. TryScopedAbsorb: a scoped absorb ward, rolled per payload delivery.
bool RunWholeCastWards(ActionContext &ctx, bool is_magic);
bool TryScopedAbsorb(ActionContext &ctx, talents_actions::EWardScope want);

// Spends one trigger charge of the bearer's `affect_type` affect and removes the affect when its
// charges run out (affect_triggers.cpp).
void SpendCharAffectCharge(CharData *bearer, EAffect affect_type);

// Summon stage (action_summon.cpp): the data-driven <summon> action and the corpse-based manual
// handlers registered in kManualHandlers.
EStageResult CastSummonAction(ActionContext &ctx);
EStageResult SpellAnimateSkeleton(ActionContext &ctx);
EStageResult SpellAnimateDead(ActionContext &ctx);
EStageResult SpellResurrection(ActionContext &ctx);

// Object creation stage (action_handlers.cpp): the data-driven <obj_creation> action.
EStageResult CastCreationAction(ActionContext &ctx);

// Talent patches (action_context.cpp): maps a patch's mod op to the manifestation-side enum, shared
// by spell and affect action chains.
talents_actions::EFieldModOp ToFieldModOp(feats::TalentPatch::EModOp op);

// Cast-flow helpers (magic.cpp) that the affect-trigger code (affect_triggers.cpp) reuses to run an
// action chain over its targets.
bool TargetIsBlocked(CharData *victim, const talents_actions::FlagCondition &cond);
bool TargetMeetsRequired(CharData *victim, const talents_actions::FlagCondition &cond);
ECastResult RunActionOverTargets(ActionContext &ctx, const std::vector<CharData *> &targets,
								 ECastTargets scope, bool is_entry);
ECastTargets ActionTargetScope(talents_actions::EActionTarget sel, ECastTargets prev_scope);
std::vector<CharData *> ResolveActionTargets(ActionContext &ctx,
		talents_actions::EActionTarget sel, const std::vector<CharData *> &prev);
ECastResult RunActionOnRoom(ActionContext &ctx);

// Room-targeted casting (action_room_affect.cpp).
namespace room_affects {
ECastResult CallMagicToRoom(CharData *ch, RoomData *room, ActionContext roll);
// Cast a kMagRoom spell onto the exit in `dir`.
ECastResult CallMagicToExit(CharData *ch, int dir, ActionContext roll);
// Impose the spell's room affect from the current action; callable from the per-action loop.
ECastResult CastRoomAffect(ActionContext &ctx);
}  // namespace room_affects

#endif  // MAGIC_INTERNAL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
