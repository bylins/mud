/**
 \file spell_handlers.h - a part of the Bylins engine.
 \brief issue.spellhandlers: declarations of the named spell handlers referenced by
        handler="..." in spells.xml.
 \details Each handler lives in its own .cpp under src/gameplay/handlers/, so it can later be
          rewritten individually (e.g. in a scripting language) without touching the pipeline.
          This single header is included both by the handler .cpp files and by the spell-pipeline
          files that hold the name->function dispatch registries (currently magic.cpp).
*/

#ifndef BYLINS_SRC_GAMEPLAY_HANDLERS_SPELL_HANDLERS_H_
#define BYLINS_SRC_GAMEPLAY_HANDLERS_SPELL_HANDLERS_H_

#include "gameplay/magic/magic.h"        // CastContext, EStageResult
#include "gameplay/magic/magic_rooms.h"  // room_spells::ERoomApply (HandleThunderstormTick sig)

enum class ESpellMsg;               // defined in gameplay/magic/spell_messages.h
class CharData;                     // entities/char_data.h
class ObjData;                      // entities/obj_data.h

namespace handlers {

// --- Alter-object handlers (issue.obj-casting / issue.spellhandlers) ---------------------------
// Each runs on ctx.ovict (already resolved and kNoalter-guarded by CastToAlterObjs), does its OWN
// messaging, and returns kSuccess when it acted (or chose to stay silent) or kFail to ask the
// skeleton for the generic "no effect" line.
EStageResult AlterBless(CastContext &ctx);
EStageResult AlterCurse(CastContext &ctx);
EStageResult AlterInvisible(CastContext &ctx);
EStageResult AlterPoison(CastContext &ctx);
EStageResult AlterRemoveCurse(CastContext &ctx);
EStageResult AlterEnchantWeapon(CastContext &ctx);
EStageResult AlterRemovePoison(CastContext &ctx);
EStageResult AlterFly(CastContext &ctx);
EStageResult AlterAcid(CastContext &ctx);
EStageResult AlterRepair(CastContext &ctx);
EStageResult AlterTimerRestore(CastContext &ctx);
EStageResult AlterRestoration(CastContext &ctx);
EStageResult AlterLight(CastContext &ctx);
EStageResult AlterDarkness(CastContext &ctx);

// --- Object-creation handlers (issue.obj-casting) ----------------------------------------------
// Optional post-load customizer for <obj_creation>: shapes the freshly-loaded base object before
// narration/placement. Currently plumbing stubs (the stat/type customization is a TODO).
void CreateWeapon(CharData *ch, ObjData *obj, const CastContext &ctx);
void CreateArmor(CharData *ch, ObjData *obj, const CastContext &ctx);

// --- Summon handlers (issue.spellhandlers) ------------------------------------------------------
// kSummonTutelar manual cast: rolls the guardian-angel summon, builds and places the mob.
EStageResult SummonTutelar(CastContext &ctx);

// Post-spawn customizers run by CastSummonAction's kSummonHandlers registry (the spell-specific
// 20%); the shared summon-pipeline helpers they use live in gameplay/mechanics/summon.h.
void SetupKeeperStats(CharData *ch, CharData *mob, const CastContext &ctx);
void SetupFirekeeperStats(CharData *ch, CharData *mob, const CastContext &ctx, int charm_duration);
void CloneCascade(CharData *ch, CharData *mob, const CastContext &ctx, int duration);

// --- Room-affect tick handler (issue.spellhandlers) ---------------------------------------------
// kThunderstorm per-tick cascade, dispatched by magic_rooms.cpp's kRoomTickHandlers. (Its
// Affect<ERoomApply>::shared_ptr signature is why this header includes magic_rooms.h.)
void HandleThunderstormTick(CharData *ch, const Affect<room_spells::ERoomApply>::shared_ptr &aff);

// Shared messaging helper for the alter-obj handlers: act() the cast spell's `key` message on
// ctx.ovict and return kSuccess. Used by multiple handlers, so by the issue's rule it is shared
// code -- kept in magic.cpp by design (trivial pipeline messaging glue; a candidate for a future
// move to src/gameplay/mechanics/ should it grow).
EStageResult AlterMsg(CastContext &ctx, ESpellMsg key);

}  // namespace handlers

#endif  // BYLINS_SRC_GAMEPLAY_HANDLERS_SPELL_HANDLERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
