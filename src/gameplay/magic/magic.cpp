/* ************************************************************************
*   File: magic.cpp                                     Part of Bylins    *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "magic.h"

#include <fmt/format.h>

#include "gameplay/affects/obj_affects.h"
#include "gameplay/affects/obj_affect_messages.h"
#include "administration/privilege.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/condition.h"
#include "gameplay/fight/fight_stuff.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/resist.h"
#include "magic_internal.h"
#include "spell_trace.h"
#include "gameplay/mechanics/saving.h"
#include "gameplay/mechanics/antisaving.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/summon.h"
#include "gameplay/mechanics/animate_dead.h"
#include "gameplay/mechanics/mob_races.h"
#include "gameplay/handlers/bone_commander.h"

#include <algorithm>
#include <functional>
#include <map>
#include "magic_rooms.h"
#include "room_affect_messages.h"

#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/core/target_resolver.h"

#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/affects/equipment_affects.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/obj_sets.h"
#include "gameplay/mechanics/poison.h"
#include "engine/db/world_characters.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/fight/pk.h"
#include "engine/core/char_equip_flags.h"
#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "magic_utils.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_player.h"
#include "utils/random.h"
#include "engine/db/global_objects.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/weather.h"
#include "utils/utils_time.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/core/remort.h"
#include "gameplay/abilities/abilities_rollsystem.h"

extern int interpolate(int min_value, int pulse);

void ReactToCast(CharData *victim, CharData *caster, ESpell spell_id);

bool IsRoomForbidden(RoomData *room) {
	for (const auto &af: room->affected) {
		if (af->affect_type == room_spells::ERoomAffect::kForbidden && (number(1, 100) <= af->modifier)) {
			return true;
		}
	}
	return false;
}

bool IsAbleToSay(CharData *ch) {
	if (!ch->IsNpc()) {
		return true;
	}
	return MUD::MobRaces()[GET_RACE(ch)].IsVocal();
}

int CheckMobList(CharData *ch) {
	for (const auto &vict: character_list) {
		if (vict.get() == ch) {
			return (true);
		}
	}

	return (false);
}

// A reasonably-bright victim who can't see its invisible caster spends one of its own
// prepared spells trying to remedy that (detect invis -> sense life -> light, first
// available wins). The intellect check is "above 25, or a soft number(10,25) roll".
void MaybeAutoCastDetection(CharData *victim, CharData *caster) {
	if (sight::CanSee(victim, caster)) {
		return;
	}
	if (GetRealInt(victim) <= 25 && GetRealInt(victim) <= number(10, 25)) {
		return;
	}
	if (!AFF_FLAGGED(victim, EAffect::kDetectInvisible)
		&& GET_SPELL_MEM(victim, ESpell::kDetectInvis) > 0) {
		CastSpell(victim, victim, nullptr, nullptr, ESpell::kDetectInvis, ESpell::kDetectInvis);
	} else if (!AFF_FLAGGED(victim, EAffect::kDetectLife)
		&& GET_SPELL_MEM(victim, ESpell::kSenseLife) > 0) {
		CastSpell(victim, victim, nullptr, nullptr, ESpell::kSenseLife, ESpell::kSenseLife);
	} else if (!AFF_FLAGGED(victim, EAffect::kInfravision)
		&& GET_SPELL_MEM(victim, ESpell::kLight) > 0) {
		CastSpell(victim, victim, nullptr, nullptr, ESpell::kLight, ESpell::kLight);
	}
}

void ReactToCast(CharData *victim, CharData *caster, ESpell spell_id) {
	if (caster == victim)
		return;

	// An NPC reacts only to a cast that is aggressive against it -- spells from allies (a PC's pet
	// on its master, mob-to-mob) fall through silently.
	if (!CheckMobList(victim) || !MUD::Spell(spell_id).IsViolentAgainst(caster, victim))
		return;

	if (AFF_FLAGGED(victim, EAffect::kCharmed) ||
		AFF_FLAGGED(victim, EAffect::kSleep) ||
		AFF_FLAGGED(victim, EAffect::kBlind) ||
		AFF_FLAGGED(victim, EAffect::kStopFight) ||
		AFF_FLAGGED(victim, EAffect::kMagicStopFight) || AFF_FLAGGED(victim, EAffect::kHold)
		|| mount::IsHorse(victim))
		return;

	if (caster->IsNpc() && caster->get_rnum() == GetMobRnum(kDgCasterProxy))
		return;

	if (sight::CanSee(victim, caster) && MayAttack(victim) && victim->in_room == caster->in_room) {
		if (victim->IsNpc())
			mob_ai::attack_best(victim, caster, false);
		else
			hit(victim, caster, ESkill::kUndefined, fight::kMainHand);
	} else if (sight::CanSee(victim, caster) && !caster->IsNpc() && victim->IsNpc()
		&& victim->IsFlagged(EMobFlag::kMemory)) {
		mob_ai::mobRemember(victim, caster);
	}

	if (caster->purged()) {
		return;
	}
	MaybeAutoCastDetection(victim, caster);
}

// <blocking>: true if the target carries ANY of the listed flags/affects
// (NPC mob flags matter only for NPCs; affect flags via AFF_FLAGGED for any target).
bool TargetIsBlocked(CharData *victim, const talents_actions::FlagCondition &cond) {
	if (victim->IsNpc()) {
		for (const auto flag : cond.mob_flags) {
			if (victim->IsFlagged(flag)) {
				return true;
			}
		}
	}
	for (const auto aff : cond.affect_flags) {
		if (AFF_FLAGGED(victim, aff)) {
			return true;
		}
	}
	if (cond.align == EAlign::kGood && alignment::IsGood(victim)) {
		return true;
	}
	if (cond.align == EAlign::kEvil && alignment::IsEvil(victim)) {
		return true;
	}
	if (cond.align == EAlign::kNeutral && alignment::IsNeutral(victim)) {
		return true;
	}
	return false;
}

// <required>: true only if the target has ALL the listed flags/affects
// (a required mob flag implies the target is an NPC carrying it).
bool TargetMeetsRequired(CharData *victim, const talents_actions::FlagCondition &cond) {
	for (const auto flag : cond.mob_flags) {
		if (!victim->IsNpc() || !victim->IsFlagged(flag)) {
			return false;
		}
	}
	for (const auto aff : cond.affect_flags) {
		if (!AFF_FLAGGED(victim, aff)) {
			return false;
		}
	}
	if (cond.align == EAlign::kGood && !alignment::IsGood(victim)) {
		return false;
	}
	if (cond.align == EAlign::kEvil && !alignment::IsEvil(victim)) {
		return false;
	}
	if (cond.align == EAlign::kNeutral && !alignment::IsNeutral(victim)) {
		return false;
	}
	return true;
}

// Spell-level caster gate. Reuses the victim-side helpers on the
// CASTER: a <caster_conditions><blocking> flag/affect/align blocks the cast, and every
// <caster_conditions><required> axis must be satisfied. Empty conditions never block.
bool CasterBlocked(CharData *caster, const talents_actions::CasterConditions &cc) {
	return TargetIsBlocked(caster, cc.blocking) || !TargetMeetsRequired(caster, cc.required);
}

// True if `victim` matches any of the reflection's affect flags or its alignment. Flag presence +
// prob only: mob/object affects carry no potency, so reflection can't be potency-gated.
static bool VictimMatchesReflection(CharData *victim, const talents_actions::Reflection &refl) {
	for (const auto aff : refl.affect_flags) {
		if (AFF_FLAGGED(victim, aff)) return true;
	}
	if (refl.align == EAlign::kGood && alignment::IsGood(victim)) return true;
	if (refl.align == EAlign::kEvil && alignment::IsEvil(victim)) return true;
	if (refl.align == EAlign::kNeutral && alignment::IsNeutral(victim)) return true;
	return false;
}

// If the spell's <reflection> matches `cvict` and the prob roll succeeds, emit the three
// reflection messages and return `caster` -- the spell will now hit the caster instead.
// Otherwise return `cvict` unchanged. The redirection happens once for the whole cast (damage +
// affects + ...), not per stage. Self-casts never bounce.
static CharData *MaybeReflectToCaster(CharData *caster, CharData *cvict, ESpell spell_id) {
	if (!cvict || cvict == caster) {
		return cvict;
	}
	const auto &refl = MUD::Spell(spell_id).actions.GetReflection();
	if (refl.empty() || !VictimMatchesReflection(cvict, refl)) {
		return cvict;
	}
	if (number(1, 100) > refl.prob) {
		return cvict;
	}
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kReflectedToChar).c_str(),
		false, caster, nullptr, cvict, kToChar);
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kReflectedToVict).c_str(),
		false, caster, nullptr, cvict, kToVict);
	act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kReflectedToRoom).c_str(),
		false, caster, nullptr, cvict, kToNotVict | kToArenaListen);
	return caster;
}

// Cast each of the action's <side_spell> spells as a full nested pipeline on the current target.
// Each side cast gets its OWN rolls and a fresh context, so its accumulators and reactions are
// isolated. Loop guard: a spell already in ctx.casting (this chain) is refused.
EStageResult CastSideSpell(ActionContext &ctx) {
	const auto &sides = ctx.action_or_default().GetSideSpells();
	if (sides.empty()) {
		return EStageResult::kSuccess;
	}
	EStageResult result = EStageResult::kSuccess;
	for (const ESpell side : sides) {
		if (ctx.casting.count(side) > 0) {
			err_log("CastSideSpell: cast loop -- %s already in the chain (spell %s); aborted.",
					NAME_BY_ITEM<ESpell>(side).c_str(), NAME_BY_ITEM<ESpell>(ctx.spell_id()).c_str());
			result = EStageResult::kFail;
			continue;
		}
		ActionContext sub = BuildActionContext(ctx.caster(), side, ctx.level);
		sub.casting = ctx.casting;   // inherit the ancestry ...
		sub.casting.insert(side);    // ... plus this side spell (so its own side casts see the chain)
		sub.SetNested();             // a side-spell delivery must not re-run whole-cast wards
		sub.cvict = ctx.cvict;       // cast on the current target (this is a per-target stage)
		sub.area_coeff = ctx.area_coeff;  // inherit the outer per-target falloff (mass spells)
		sub.SetEvent(ctx.Event());   // a side spell's handlers see the event too
		sub.SetAffectDamageMsg(ctx.AffectDamageMsgChar(), ctx.AffectDamageMsgVict(),
							   ctx.AffectDamageMsgRoom());   // ... and the affect's damage flavor
		if (CastSpell(sub, ECastTargets::kSingle) == ECastResult::kNotCast) {
			result = EStageResult::kFail;
		}
	}
	return result;
}

// Per-(action, target) pipeline (per-action targets). Runs the cast gates and
// the CURRENT action's data-driven stages on ctx.cvict. `is_entry` (the spell's first action)
// additionally runs the whole-cast steps -- reflection, god guard, mtrigger, the one-shot
// stages (alter-objs / summon / creation / manual) and ReactToCast -- which belong to the
// cast as a whole, not to a per-action target list.
ECastResult CastOnTarget(ActionContext &ctx, bool is_entry) {
	CharData *caster = ctx.caster();
	CharData *cvict = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	const auto &action = ctx.action_or_default();
	ctx.is_entry_action = is_entry;  // stages read this via ctx.CompetenceBase()
	// Dead-target guard: a later action may aim at someone an earlier action
	// already killed; never run stages on a corpse -- skip it, the cast continues for the rest.
	if (cvict && cvict->purged()) {
		return ECastResult::kSuccess;
	}
	// kTarMinionsOnly: castable only on one of the caster's own NPC followers (master == caster).
	// Checked per target so it covers group/mass casts too. A single-target cast on the wrong
	// target is refused with a message; group/mass casts just skip non-followers silently.
	if (MUD::Spell(spell_id).AllowTarget(kTarMinionsOnly)
			&& !(cvict && cvict->IsNpc() && cvict->get_master() == caster)) {
		if (!MUD::Spell(spell_id).IsFlagged(kMagGroups | kMagMasses | kMagAreas)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCantCastNotMinion) + "\r\n", caster);
		}
		return ECastResult::kNotCast;
	}
	// Action-level <blocking>/<required> gates, read from THIS action (the cursor): the cast is
	// refused on a target carrying a blocking flag/affect or lacking a required one. Per target,
	// so it covers group/mass casts; for those a refusal stays silent (no per-target spam).
	if (cvict && (TargetIsBlocked(cvict, action.GetBlocking())
			|| !TargetMeetsRequired(cvict, action.GetRequired()))) {
		if (!MUD::Spell(spell_id).IsFlagged(kMagGroups | kMagMasses | kMagAreas)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
		}
		return ECastResult::kNotCast;
	}
	if (is_entry) {
		cvict = MaybeReflectToCaster(caster, cvict, spell_id);
		ctx.cvict = cvict;
		// Whole-cast defender wards (reflect / scope="all" absorb / magical shield) run once, for the
		// TOP-LEVEL cast only: a side-spell sub-cast (ctx.Nested()) is part of the same incoming spell,
		// so re-running them would fire the ward twice. Scoped absorbs are per-manifest.
		if (!ctx.Nested()) {
			RunWholeCastWards(ctx, /*is_magic=*/true);
		}
		if (ctx.WardStop()) {
			return ECastResult::kNotCast;
		}
		cvict = ctx.cvict;
		if (cvict && (caster != cvict))
			// The level-difference half of this guard is commented out: after
			// proper balancing it should be moot -- a low-level mage can't land a strong buff,
			// can't penetrate a debuff's saving throw, and damage now scales with (low) skill.
			// Kept for quick reactivation if some unforeseen case needs it:
			//     || (((GetRealLevel(cvict) / 2) > (GetRealLevel(caster) + (GetRealRemort(caster) / 2))) && !caster->IsNpc())
			if (privilege::IsGod(cvict) /* level-diff condition disabled, see above */) {
				SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", caster);
				return ECastResult::kNotCast;
			}
		if (!cast_mtrigger(cvict, caster, spell_id)) {
			return ECastResult::kNotCast;
		}
		if (MUD::Spell(spell_id).IsFlagged(kMagWarcry) && cvict && cvict->IsFlagged(EMobFlag::kUndead))
			return ECastResult::kSuccess;
	}
	// This action's data-driven stages, in the fixed order Damage -> Unaffect -> Affect -> Points.
	// The entry action triggers on the spell's kMag* flags (so flag-only, code-driven stages such
	// as kCallLighting still fire); later actions run only the stages they carry as data. A stage
	// that breaks, or a target killed by the damage, stops only THIS action's remaining stages on
	// THIS target -- never the action chain (a later heal on the group still runs) and never the
	// action's other targets.
	const bool run_damage    = is_entry ? MUD::Spell(spell_id).IsFlagged(kMagDamage)
										: action.Contains(talents_actions::EAction::kDamage);
	const bool run_unaffects = is_entry ? MUD::Spell(spell_id).IsFlagged(kMagUnaffects)
										: action.Contains(talents_actions::EAction::kUnaffect);
	const bool run_affects   = is_entry ? MUD::Spell(spell_id).IsFlagged(kMagAffects)
										: action.Contains(talents_actions::EAction::kAffect);
	const bool run_points    = is_entry ? MUD::Spell(spell_id).IsFlagged(kMagPoints)
										: action.Contains(talents_actions::EAction::kPoints);
	// the manual stage runs whenever the action names a handler (entry or not).
	const bool run_manual    = is_entry ? MUD::Spell(spell_id).IsFlagged(kMagManual)
										: action.Contains(talents_actions::EAction::kManual);
	// runs whenever the action carries a <side_spell> (entry or not, no flag).
	const bool run_side      = action.Contains(talents_actions::EAction::kSideSpell);
	const bool run_alter     = action.Contains(talents_actions::EAction::kAlterObj);
	bool target_died = false;
	bool stop_stages = false;
	if (run_damage && CastDamage(ctx) == EStageResult::kBreak) {
		// CastDamage breaks only when the target died: affect/points on a corpse are pointless,
		// so skip this action's remaining stages -- but the cast continues for everyone else.
		target_died = true;
		stop_stages = true;
	}
	if (run_unaffects && !stop_stages && CastUnaffects(ctx) != EStageResult::kSuccess) {
		stop_stages = true;   // a dispel that broke stops THIS action's remaining stages, not the chain.
	}
	if (run_affects && !stop_stages && CastAffect(ctx) != EStageResult::kSuccess) {
		stop_stages = true;
	}
	if (run_points && !stop_stages && CastToPoints(ctx) != EStageResult::kSuccess) {
		stop_stages = true;
	}
	if (run_side && !stop_stages) {
		CastSideSpell(ctx);  // nested cast(s) on the current target; fire-and-forget
	}
	if (run_manual && !stop_stages && CastManual(ctx) != EStageResult::kSuccess) {
		stop_stages = true;
	}
	// Per target (after manual), so an area corrode (kAcidArrow) hits each target's item. Gated on
	// !target_died only, NOT stop_stages: remove-curse still cleans the object even if the
	// char-side dispel did nothing.
	if (run_alter && !target_died) {
		CastToAlterObjs(ctx);
	}
	// Record this target for the deferred end-of-cast reaction. Reaching here
	// means the cast landed on a LIVE `cvict` (not refused, not killed), so the target should
	// retaliate -- but only after ALL the spell's actions have run (fired in CastSpell).
	if (cvict && !target_died
			&& std::find(ctx.reactions.begin(), ctx.reactions.end(), cvict) == ctx.reactions.end()) {
		ctx.reactions.push_back(cvict);
	}
	// kTargetDied is a status (callers like warcry / mixture check it); it does NOT break the chain.
	return target_died ? ECastResult::kTargetDied : ECastResult::kSuccess;
}

void TrySendCastMessages(CharData *ch, CharData *victim, RoomData *room, ESpell spell_id) {
	if (room && world[ch->in_room] == room && IsAbleToSay(ch)) {
		const auto &sheaf = MUD::SpellMessages()[spell_id];
		if (sheaf.HasMessage(ESpellMsg::kAreaToChar)) {
			act(sheaf.GetMessage(ESpellMsg::kAreaToChar).c_str(), false, ch, nullptr, victim, kToChar);
		}
		if (sheaf.HasMessage(ESpellMsg::kAreaToRoom)) {
			act(sheaf.GetMessage(ESpellMsg::kAreaToRoom).c_str(), false, ch, nullptr, victim, kToRoom | kToArenaListen);
		}
	}
};

// build the ordered target list for an area/group cast. kFoes -> every
// foe in the caster's room (priority target first); kFriends -> the caster's group with the
// caster moved LAST (so a group recall does not whisk the caster off before the others).
static std::vector<CharData *> BuildCastRoster(CharData *caster, CharData *victim,
											   ECastTargets scope) {
	std::vector<CharData *> out;
	if (scope == ECastTargets::kFoes) {
		target_resolver::FoesRosterType roster{caster, victim,
											   [](CharData *, CharData *target) {
												   return !mount::IsHorse(target);
											   }};
		for (auto *target : roster) {
			out.push_back(target);
		}
	} else if (scope == ECastTargets::kFriends) {
		target_resolver::FriendsRosterType roster{caster, caster};
		roster.flip();
		for (auto *target : roster) {
			out.push_back(target);
		}
	}
	return out;
}

// Run the cursor's current action over its target list, applying the <area> distribution
// coefficient and the matching cast_success scaling per target. A single-target cast uses no
// roster, distribution or cast_success scaling.
ECastResult RunActionOverTargets(ActionContext &ctx, const std::vector<CharData *> &targets,
										ECastTargets scope, bool is_entry) {
	CharData *caster = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (scope == ECastTargets::kSingle) {
		ctx.area_coeff = 1.0;
		ctx.cvict = targets.empty() ? nullptr : targets.front();
		return CastOnTarget(ctx, is_entry);
	}
	// A multi-target spell without <area> (e.g. a kMagGroups heal) hits the whole roster at full
	// strength. GetArea() throws when there is no <area>, so check Contains() first.
	const auto &action = ctx.action_or_default();
	const talents_actions::Area *area =
			action.Contains(talents_actions::EAction::kArea) ? &action.GetArea() : nullptr;
	// N = number of targets hit. No <area>, or max <= 0, means no limit (the whole roster).
	const int n = (area == nullptr || area->max_targets <= 0)
			? static_cast<int>(targets.size())
			: std::min(area->CalcTargetsQuantity(GetSkill(caster, MUD::Spell(spell_id).GetSuccessRoll().GetBaseSkill()),
													  ctx.potency().stat_coeff),
					   static_cast<int>(targets.size()));
	// The decay may be scaled by a talent patch (kMultipleCast); NPC casters ignore it (coeff 1.0 below).
	const double decay_eff = (area == nullptr) ? 0.0 : area->decay;
	const int kCasterCastSuccess = GET_CAST_SUCCESS(caster);
	const auto &sheaf = MUD::SpellMessages()[spell_id];
	const bool has_vict_msg = sheaf.HasMessage(ESpellMsg::kAreaToVict);
	const std::string vict_msg = has_vict_msg ? sheaf.GetMessage(ESpellMsg::kAreaToVict) : std::string{};
	int j = 0;
	for (CharData *target : targets) {
		if (j >= n) {
			break;
		}
		if (target->purged()) {
			continue;  // skip a target an earlier action already killed (don't consume a slot)
		}
		++j;  // 1-based position; target #1 = primary victim (foes) / first ally (group).
		const double coeff = (area == nullptr || caster->IsNpc()) ? 1.0 : area->DistributionCoeff(j, n, decay_eff);
		ctx.area_coeff = coeff;
		GET_CAST_SUCCESS(caster) = static_cast<int>(kCasterCastSuccess * coeff);
		if (has_vict_msg && target->desc) {
			act(vict_msg.c_str(), false, caster, nullptr, target, kToVict);
		}
		ctx.cvict = target;
		ctx.ovict = nullptr;
		CastOnTarget(ctx, is_entry);
		if (caster->purged()) {
			return ECastResult::kSuccess;
		}
		if (caster->IsFlagged(EPrf::kTester)) {
			SendMsgToChar(caster,
						  "&GМакс. целей: %d, Каст: %d, Коэффициент: %.2f.&n\r\n",
						  n,
						  GET_CAST_SUCCESS(caster),
						  coeff);
		}
	}
	GET_CAST_SUCCESS(caster) = kCasterCastSuccess;
	return ECastResult::kSuccess;
}

// Cast scope implied by a per-action target selector; kTarSame inherits the previous action's scope.
ECastTargets ActionTargetScope(talents_actions::EActionTarget sel, ECastTargets prev_scope) {
	switch (sel) {
		case talents_actions::EActionTarget::kTarFoes:  return ECastTargets::kFoes;
		case talents_actions::EActionTarget::kTarGroup: return ECastTargets::kFriends;
		case talents_actions::EActionTarget::kTarMinions: return ECastTargets::kFriends;
		case talents_actions::EActionTarget::kTarSame:  return prev_scope;
		default:                                        return ECastTargets::kSingle;
	}
}

// resolve a NON-first action's target list from its <action target=...> selector.
std::vector<CharData *> ResolveActionTargets(ActionContext &ctx,
		talents_actions::EActionTarget sel, const std::vector<CharData *> &prev) {
	CharData *caster = ctx.caster();
	switch (sel) {
		case talents_actions::EActionTarget::kTarSame:
			return prev;
		case talents_actions::EActionTarget::kTarFightSelf:
			return {caster};
		case talents_actions::EActionTarget::kTarFightVict: {
			CharData *enemy = caster->GetEnemy();
			return enemy ? std::vector<CharData *>{enemy} : std::vector<CharData *>{};
		}
		case talents_actions::EActionTarget::kTarGroup:
			return BuildCastRoster(caster, caster, ECastTargets::kFriends);
		case talents_actions::EActionTarget::kTarFoes:
			return BuildCastRoster(caster, nullptr, ECastTargets::kFoes);
		case talents_actions::EActionTarget::kTarMinions: {
			// The caster's charmed NPC followers in the room (the game's "minion"). An undead-only
			// restriction etc. is layered on via the action's <target_conditions> (e.g. required kCorpse).
			std::vector<CharData *> out;
			for (auto *f : caster->followers) {
				if (f && f->IsNpc() && AFF_FLAGGED(f, EAffect::kCharmed) && f->in_room == caster->in_room) {
					out.push_back(f);
				}
			}
			return out;
		}
		case talents_actions::EActionTarget::kTarRoomThis:
			// The room itself is not a CharData target -- it is applied to ctx.rvict by the
			// room-impose path, not through the char roster. No char targets here.
			return {};
		case talents_actions::EActionTarget::kTarActor:
			// The char that fired an event trigger (e.g. the room enterer), carried on ctx.cvict.
			return ctx.cvict ? std::vector<CharData *>{ctx.cvict} : std::vector<CharData *>{};
		case talents_actions::EActionTarget::kTarRandomFoe: {
			target_resolver::FoesRosterType roster{caster, nullptr,
					[](CharData *, CharData *t) { return !mount::IsHorse(t); }};
			CharData *one = roster.getRandomItem();
			return one ? std::vector<CharData *>{one} : std::vector<CharData *>{};
		}
		case talents_actions::EActionTarget::kTarRandomAlly: {
			target_resolver::FriendsRosterType roster{caster, caster};
			CharData *one = roster.getRandomItem();
			return one ? std::vector<CharData *>{one} : std::vector<CharData *>{};
		}
		case talents_actions::EActionTarget::kTarMaster: {
			// The caster's charm master (kSoulLink heals the master).
			CharData *mst = caster->has_master() ? caster->get_master() : nullptr;
			return mst ? std::vector<CharData *>{mst} : std::vector<CharData *>{};
		}
	}
	return {};
}

// Run a kTarRoomThis action on the room (ctx.rvict, defaulting to the caster's room): the
// room-capable stages -- dispel (<unaffect>) then impose (<affects> via CastRoomAffect). Lets a
// later action apply/scale a room affect off a prior action's result (ctx.CompetenceBase()).
ECastResult RunActionOnRoom(ActionContext &ctx) {
	CharData *caster = ctx.caster();
	if (ctx.rvict == nullptr) {
		if (caster->in_room == kNowhere) {
			return ECastResult::kNotCast;
		}
		ctx.rvict = world[caster->in_room];
	}
	ctx.cvict = nullptr;
	ctx.area_coeff = 1.0;
	if (CastUnaffects(ctx) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}
	return room_spells::CastRoomAffect(ctx);
}

// Main cast entry: runs each of the spell's <action>s over its own target list. Action[0] (the entry)
// targets the spell's scope; later actions resolve theirs from <action target=...>. Entry-only gates
// (reflection / mtrigger / one-shots) fire on action[0] -- see CastOnTarget's is_entry.
// A flag-only spell (no <action>) still gets one entry iteration so its in-code stages fire.
ECastResult CastSpell(ActionContext &ctx, ECastTargets scope) {
	CharData *caster = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (caster == nullptr || caster->in_room == kNowhere) {
		return ECastResult::kNotCast;
	}
	std::vector<CharData *> entry_targets;
	if (scope == ECastTargets::kSingle) {
		entry_targets.push_back(ctx.cvict);
	} else {
		entry_targets = BuildCastRoster(caster, ctx.cvict, scope);
		TrySendCastMessages(caster, ctx.cvict, world[caster->in_room], spell_id);
	}
	std::vector<CharData *> prev = entry_targets;
	ECastTargets prev_scope = scope;
	bool is_entry = true;
	ECastResult result = ECastResult::kSuccess;
	try {
		auto apply_reset = [&]() {
			// a non-first action with reset zeroes the accumulator it read (its base=), so a
			// later action starts fresh. Meaningless for the entry action.
			if (!is_entry && ctx.action_or_default().GetReset()) {
				switch (ctx.action_or_default().GetBase()) {
					case talents_actions::EActionBase::kDamage:    ctx.damage_count = 0.0; break;
					case talents_actions::EActionBase::kPoints:    ctx.points_count = 0.0; break;
					case talents_actions::EActionBase::kAffects:   ctx.affects_potency = 0.0; break;
					case talents_actions::EActionBase::kDispelled: ctx.dispelled_potency = 0.0; break;
					default: break;
				}
			}
		};
		for (ctx.RewindActions(); ctx.HasPendingActions(); ctx.NextAction()) {
			// A kTarRoomThis action targets the room itself (ctx.rvict), not the char roster --
			// it runs the room dispel + impose stages and can scale off a prior action.
			if (ctx.action_or_default().GetTarget() == talents_actions::EActionTarget::kTarRoomThis) {
				result = RunActionOnRoom(ctx);
				if (caster->purged()) {
					return ECastResult::kSuccess;
				}
				apply_reset();
				prev.clear();
				prev_scope = ECastTargets::kSingle;
				is_entry = false;
				continue;
			}
			// A <summon> spawns once per action, not per target. Running it here lets it chain off a prior
			// action via base=; is_entry_action selects the entry potency C vs the base= accumulator.
			if (ctx.action_or_default().Contains(talents_actions::EAction::kSummon)) {
				ctx.is_entry_action = is_entry;
				CastSummonAction(ctx);
				if (caster->purged()) {
					return ECastResult::kSuccess;
				}
				apply_reset();
				prev.clear();
				prev_scope = ECastTargets::kSingle;
				is_entry = false;
				continue;
			}
			// An <obj_creation> likewise runs once per action, not per target.
			if (ctx.action_or_default().Contains(talents_actions::EAction::kCreation)) {
				ctx.is_entry_action = is_entry;
				CastCreationAction(ctx);
				if (caster->purged()) {
					return ECastResult::kSuccess;
				}
				apply_reset();
				prev.clear();
				prev_scope = ECastTargets::kSingle;
				is_entry = false;
				continue;
			}
			// The entry action targets the spell scope; later actions resolve their own target
			// list from <action target=...> (default kTarSame = reuse the previous list).
			std::vector<CharData *> targets;
			ECastTargets cur_scope;
			if (is_entry) {
				targets = entry_targets;
				cur_scope = scope;
			} else {
				const talents_actions::EActionTarget sel = ctx.action_or_default().GetTarget();
				targets = ResolveActionTargets(ctx, sel, prev);
				cur_scope = ActionTargetScope(sel, prev_scope);
			}
			result = RunActionOverTargets(ctx, targets, cur_scope, is_entry);
			if (caster->purged()) {
				return ECastResult::kSuccess;
			}
			apply_reset();
			prev = std::move(targets);
			prev_scope = cur_scope;
			is_entry = false;
		}
	} catch (std::runtime_error &e) {
		err_log("%s", e.what());
	}
	// The whole cast is one event: fire the target/environment reactions now, after every action
	// has completed. Skip if the caster is gone; skip targets that died during
	// the cast; a reaction that kills the caster stops the rest.
	if (!caster->purged()) {
		for (CharData *target : ctx.reactions) {
			if (target->purged()) {
				continue;
			}
			ReactToCast(target, caster, spell_id);
			if (caster->purged()) {
				break;
			}
		}
	}
	return result;
}

// Casts `spell_id` as an area attack on every foe in the caster's room, regardless of the spell's own
// targeting flags. Used by room-affect ticks (deadly fog / thunderstorm).
ECastResult CastAreaInRoom(CharData *ch, ESpell spell_id, int level) {
	auto ctx = BuildActionContext(ch, spell_id, level);
	return CastSpell(ctx, ECastTargets::kFoes);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
