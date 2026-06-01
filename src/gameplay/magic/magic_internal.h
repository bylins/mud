/*
 * magic_internal.h -- declarations shared between magic.cpp and magic_utils.cpp for the
 * stage functions that aren't part of the public spell-cast surface. Don't include from
 * outside the magic module; use CallMagic / CastSpell in magic.h instead.
 *
 * These functions are the per-stage dispatchers reached from CallMagic via the kMag*
 * flag table. CastToSingleTarget is the per-target entry that runs them in order with
 * the blocking / required / reflection / caster_blocking gates layered on top. Calling
 * any of them directly skips those gates, which is why they don't sit in magic.h.
 */
#ifndef MAGIC_INTERNAL_H_
#define MAGIC_INTERNAL_H_

#include "magic.h"          // for EStageResult / ECastResult / CastContext / forward decls

// Per-target entry: runs the stages in order with the blocking/required/reflection/
// caster_blocking gates on top. Returns the whole-cast outcome.
ECastResult CastToSingleTarget(CastContext &ctx);

// Area / group dispatchers: only CallMagic fans out to these.
ECastResult CallMagicToGroup(CharData *ch, CastContext roll);
ECastResult CallMagicToArea(CharData *ch, CharData *victim, RoomData *room, CastContext roll);

// Builds a CastContext (evaluates both rolls). Module-internal; CallMagic is the entry.
CastContext ComputeCastRoll(CharData *caster, ESpell spell_id, int level);

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
