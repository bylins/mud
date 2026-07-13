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

#include "gameplay/magic/magic.h"        // ActionContext, EStageResult
#include "gameplay/magic/magic_rooms.h"  // room_spells::ERoomApply (HandleThunderstormTick sig)

enum class ESpellMsg;               // defined in gameplay/magic/spell_messages.h
class CharData;                     // entities/char_data.h
class ObjData;                      // entities/obj_data.h

namespace handlers {

// --- Alter-object handlers (issue.obj-casting / issue.spellhandlers) ---------------------------
// Each runs on ctx.ovict (already resolved and kNoalter-guarded by CastToAlterObjs), does its OWN
// messaging, and returns kSuccess when it acted (or chose to stay silent) or kFail to ask the
// skeleton for the generic "no effect" line.
EStageResult AlterBless(ActionContext &ctx);
EStageResult AlterCurse(ActionContext &ctx);
EStageResult AlterInvisible(ActionContext &ctx);
EStageResult AlterPoison(ActionContext &ctx);
EStageResult AlterRemoveCurse(ActionContext &ctx);
EStageResult AlterEnchantWeapon(ActionContext &ctx);
EStageResult AlterRemovePoison(ActionContext &ctx);
EStageResult AlterFly(ActionContext &ctx);
EStageResult AlterAcid(ActionContext &ctx);
EStageResult AlterRepair(ActionContext &ctx);
EStageResult AlterTimerRestore(ActionContext &ctx);
EStageResult AlterRestoration(ActionContext &ctx);
EStageResult AlterLight(ActionContext &ctx);
EStageResult AlterDarkness(ActionContext &ctx);

// --- Object-creation handlers (issue.obj-casting) ----------------------------------------------
// Optional post-load customizer for <obj_creation>: shapes the freshly-loaded base object before
// narration/placement. Currently plumbing stubs (the stat/type customization is a TODO).
void CreateWeapon(CharData *ch, ObjData *obj, const ActionContext &ctx);
void CreateArmor(CharData *ch, ObjData *obj, const ActionContext &ctx);

// --- Summon handlers (issue.spellhandlers) ------------------------------------------------------
// kSummonTutelar manual cast: rolls the guardian-angel summon, builds and places the mob.
EStageResult SummonTutelar(ActionContext &ctx);

// Post-spawn customizers run by CastSummonAction's kSummonHandlers registry (the spell-specific
// 20%); the shared summon-pipeline helpers they use live in gameplay/mechanics/summon.h.
void SetupKeeperStats(CharData *ch, CharData *mob, const ActionContext &ctx);
void SetupFirekeeperStats(CharData *ch, CharData *mob, const ActionContext &ctx, int charm_duration);
void CloneCascade(CharData *ch, CharData *mob, const ActionContext &ctx, int duration);

// --- Room-affect tick handler (issue.spellhandlers) ---------------------------------------------
// issue.affect-migration: kThunderstorm per-tick cascade -- now a manual_cast handler in the shared
// kManualHandlers registry (was the room_affects tick_handler). Phase = the affect's duration, passed
// via ActionContext (SetTickDuration/GetTickDuration).
EStageResult HandleThunderstormTick(ActionContext &ctx);

// --- Manual spell-cast handlers (issue.spellhandlers, extracted from spells.cpp) ------------
EStageResult SpellCreateWater(ActionContext &ctx);
EStageResult SpellRecall(ActionContext &ctx);
EStageResult SpellTeleport(ActionContext &ctx);
EStageResult SpellRelocate(ActionContext &ctx);
EStageResult SpellPortal(ActionContext &ctx);
EStageResult SpellSummon(ActionContext &ctx);
EStageResult SpellLocateObject(ActionContext &ctx);
EStageResult SpellCharm(ActionContext &ctx);
EStageResult SpellIdentify(ActionContext &ctx);
EStageResult SpellFullIdentify(ActionContext &ctx);
EStageResult SpellControlWeather(ActionContext &ctx);
EStageResult SpellFear(ActionContext &ctx);
EStageResult SpellEnergydrain(ActionContext &ctx);
EStageResult SpellHolystrike(ActionContext &ctx);
EStageResult SpellMentalShadow(ActionContext &ctx);

// Shared messaging helper for the alter-obj handlers: act() the cast spell's `key` message on
// ctx.ovict and return kSuccess. Used by multiple handlers, so by the issue's rule it is shared
// code -- kept in magic.cpp by design (trivial pipeline messaging glue; a candidate for a future
// move to src/gameplay/mechanics/ should it grow).
EStageResult AlterMsg(ActionContext &ctx, ESpellMsg key);

}  // namespace handlers

#endif  // BYLINS_SRC_GAMEPLAY_HANDLERS_SPELL_HANDLERS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
