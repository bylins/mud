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

// Spend one trigger charge on a character affect after its trigger fired. Decrements every
// instance of the affect type (one per apply, kept in sync); when any hits 0 the whole affect is
// removed. Unlimited charges are never spent. Caller must NOT be mid-iteration over `bearer->affected`.
void SpendCharAffectCharge(CharData *bearer, EAffect affect_type) {
	if (!bearer) {
		return;
	}
	bool depleted = false;
	for (const auto &aff : bearer->affected) {
		if (aff && aff->affect_type == affect_type && aff->charges != kUnlimitedAffectCharges && --aff->charges <= 0) {
			depleted = true;
		}
	}
	if (depleted) {
		// Charges exhausted counts as expiry: fire kExpired before removal, as the timer path does.
		RunCharAffectTrigger(bearer, affect_type, talents_actions::EActionTrigger::kExpired);
		if (bearer->purged()) {
			return;
		}
		RemoveAffectFromChar(bearer, affect_type);
	}
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
static bool TargetIsBlocked(CharData *victim, const talents_actions::FlagCondition &cond) {
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
static bool TargetMeetsRequired(CharData *victim, const talents_actions::FlagCondition &cond) {
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
static ECastResult RunActionOverTargets(ActionContext &ctx, const std::vector<CharData *> &targets,
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
static ECastTargets ActionTargetScope(talents_actions::EActionTarget sel, ECastTargets prev_scope) {
	switch (sel) {
		case talents_actions::EActionTarget::kTarFoes:  return ECastTargets::kFoes;
		case talents_actions::EActionTarget::kTarGroup: return ECastTargets::kFriends;
		case talents_actions::EActionTarget::kTarMinions: return ECastTargets::kFriends;
		case talents_actions::EActionTarget::kTarSame:  return prev_scope;
		default:                                        return ECastTargets::kSingle;
	}
}

// resolve a NON-first action's target list from its <action target=...> selector.
static std::vector<CharData *> ResolveActionTargets(ActionContext &ctx,
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
static ECastResult RunActionOnRoom(ActionContext &ctx) {
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

// Recursion guard shared by every trigger-launched action chain (kKill / kExpired / kDispell / kPreHit /
// kPostHit / room entry+tick) -- they all run through RunRoomCycledAction. A triggered action that deals
// damage can kill -> ProcessDeath -> another trigger -> back here. Hitting the cap almost always means a
// content loop, so the chain is skipped and logged.
namespace {
constexpr int kMaxTriggerActionDepth = 5;
int g_trigger_action_depth = 0;
struct TriggerDepthGuard {
	bool ok;
	TriggerDepthGuard() : ok(g_trigger_action_depth < kMaxTriggerActionDepth) {
		if (ok) { ++g_trigger_action_depth; }
	}
	~TriggerDepthGuard() { if (ok) { --g_trigger_action_depth; } }
	TriggerDepthGuard(const TriggerDepthGuard &) = delete;
	TriggerDepthGuard &operator=(const TriggerDepthGuard &) = delete;
};
}  // namespace

// Runs one cycled action (action[phase % N]) of `list` on the room; the action's own target marker decides
// whom it hits. Shared by kService tick spells and affect-owned actions.
static ECastResult RunRoomCycledAction(ActionContext &ctx, RoomData *room,
									   const std::vector<talents_actions::Action> &list, int phase) {
	TriggerDepthGuard depth_guard;
	if (!depth_guard.ok) {
		log("SYSERR: trigger-action recursion depth cap (%d) hit -- skipping a triggered action chain.",
			kMaxTriggerActionDepth);
		return ECastResult::kNotCast;
	}
	ctx.rvict = room;
	if (list.empty()) {
		return ECastResult::kNotCast;
	}
	ctx.RewindActions();
	const size_t idx = static_cast<size_t>(phase < 0 ? 0 : phase) % list.size();
	for (size_t i = 0; i < idx; ++i) {
		ctx.NextAction();
	}
	// A manual_cast action runs its hand-coded handler (the kManualHandlers registry), which reads its
	// context (e.g. the tick duration) off ctx.
	if (ctx.action_or_default().Contains(talents_actions::EAction::kManual)) {
		CastManual(ctx);
		return ECastResult::kSuccess;
	}
	const talents_actions::EActionTarget sel = ctx.action_or_default().GetTarget();
	if (sel == talents_actions::EActionTarget::kTarRoomThis) {
		return RunActionOnRoom(ctx);
	}
	const ECastTargets scope = ActionTargetScope(sel, ECastTargets::kFoes);
	std::vector<CharData *> targets = ResolveActionTargets(ctx, sel, {});
	return RunActionOverTargets(ctx, targets, scope, false);
}

// Runs a bearer's ADDITIVE affect talent-patches (op=append/insert) for one affect type as extra actions
// alongside the affect's own. `accept` picks the payload actions matching the runner's trigger. Gated on
// CanUseFeat(ResolvePatchRelative(bearer, relative), feat). Returns true if the bearer was purged.
template <typename TriggerPred>
static bool RunAdditiveAffectPatches(ActionContext &ctx, CharData *ch, RoomData *room,
									 EAffect type, TriggerPred accept) {
	std::vector<talents_actions::Action> extra;
	for (const auto &ref : feats::AffectTalentPatches(type)) {
		if (ref.patch->op != feats::EPatchOp::kAppend && ref.patch->op != feats::EPatchOp::kInsert) {
			continue;
		}
		CharData *holder = feats::ResolvePatchRelative(ch, ref.patch->relative);
		if (!holder || !CanUseFeat(holder, ref.feat)) {
			continue;
		}
		for (const auto &pa : ref.patch->payload.list()) {
			if (accept(pa)) {
				extra.push_back(pa);
			}
		}
	}
	if (extra.empty()) {
		return false;
	}
	ctx.UseExternalActions(&extra);
	for (size_t i = 0; i < extra.size(); ++i) {
		RunRoomCycledAction(ctx, room, extra, i);
		if (ch->purged()) {
			return true;
		}
	}
	return false;
}

// Applies an affect's MUTATING talent-patches (modify / remove / replace / add_effect / replace_all) in
// place to `list`, the per-firing COPY of its matching actions (never the shared config). `accept` filters
// the addressed actions and the installed payload to this trigger. Same feat gate as the additive path.
template <typename TriggerPred>
static void ApplyMutatingAffectPatches(std::vector<talents_actions::Action> &list, CharData *ch,
									   EAffect type, TriggerPred accept) {
	using talents_actions::Action;
	for (const auto &ref : feats::AffectTalentPatches(type)) {
		const auto op = ref.patch->op;
		if (op == feats::EPatchOp::kAppend || op == feats::EPatchOp::kInsert) {
			continue;
		}
		CharData *holder = feats::ResolvePatchRelative(ch, ref.patch->relative);
		if (!holder || !CanUseFeat(holder, ref.feat)) {
			continue;
		}
		const feats::TalentPatch &p = *ref.patch;
		auto payload_for = [&]() {
			std::vector<Action> v;
			for (const auto &pa : p.payload.list()) {
				if (accept(pa)) {
					v.push_back(pa);
				}
			}
			return v;
		};
		auto find_id = [&](const std::string &id) -> long {
			for (size_t k = 0; k < list.size(); ++k) {
				if (list[k].GetId() == id) {
					return static_cast<long>(k);
				}
			}
			return -1;
		};
		switch (op) {
			case feats::EPatchOp::kReplaceAll: {
				list = payload_for();
				break;
			}
			case feats::EPatchOp::kModify: {
				long i = -1;
				if (!p.action_id.empty()) {
					i = find_id(p.action_id);
				} else {
					for (size_t k = 0; k < list.size(); ++k) {
						if (list[k].Contains(p.mod_kind)) { i = static_cast<long>(k); break; }
					}
				}
				if (i < 0) {
					break;
				}
				talents_actions::IAction *man = list[i].CloneManifestation(p.mod_kind);
				if (man) {
					man->ApplyFieldMod(p.mod_field, ToFieldModOp(p.mod_op), p.mod_value);
				}
				break;
			}
			case feats::EPatchOp::kRemove: {
				if (p.action_id.empty()) {
					break;
				}
				long i = find_id(p.action_id);
				if (i < 0) {
					break;
				}
				if (p.effect_id.empty()) {
					list.erase(list.begin() + i);
				} else {
					list[i].EraseManifestationsById(p.effect_id);
				}
				break;
			}
			case feats::EPatchOp::kReplace: {
				if (p.action_id.empty()) {
					break;
				}
				long i = find_id(p.action_id);
				if (i < 0) {
					break;
				}
				if (p.effect_id.empty()) {
					std::vector<Action> pm = payload_for();
					list.erase(list.begin() + i);
					list.insert(list.begin() + i, pm.begin(), pm.end());
				} else {
					list[i].EraseManifestationsById(p.effect_id);
					for (auto &b : payload_for()) {
						list[i].MergeManifestationsFrom(b);
					}
				}
				break;
			}
			case feats::EPatchOp::kAddEffect: {
				if (p.action_id.empty()) {
					break;
				}
				long i = find_id(p.action_id);
				if (i < 0) {
					break;
				}
				for (auto &b : payload_for()) {
					list[i].MergeManifestationsFrom(b);
				}
				break;
			}
			default:
				break;
		}
	}
}

// Runs an affect's own actions on the room each tick. Context (level/potency) comes from the imposing
// spell (ctx_spell); the action list is injected via UseExternalActions. `tick_duration` (if non-null)
// carries the affect's duration in and out, so a manual_cast handler can read or change it.
ECastResult CastRoomTickActionFromActions(CharData *ch, RoomData *room, ESpell ctx_spell,
										  const std::vector<talents_actions::Action> &actions, int phase,
										  int *tick_duration, float fixed_potency,
										  const std::string &aff_dmg_char, const std::string &aff_dmg_vict,
										  const std::string &aff_dmg_room, long damage_author) {
	if (ch == nullptr) {
		return ECastResult::kNotCast;
	}
	ActionContext ctx = BuildActionContext(ch, ctx_spell, GetRealLevel(ch), fixed_potency);
	ctx.UseExternalActions(&actions);
	// Credited author of the tick's damage (e.g. the poisoner).
	ctx.SetDamageAuthorUid(damage_author);
	// Affect-owned damage flavor for <damage> actions (empty -> the generic combat line).
	ctx.SetAffectDamageMsg(aff_dmg_char, aff_dmg_vict, aff_dmg_room);
	if (tick_duration) {
		ctx.SetTickDuration(*tick_duration);
	}
	const ECastResult result = RunRoomCycledAction(ctx, room, actions, phase);
	if (tick_duration) {
		*tick_duration = ctx.GetTickDuration();
	}
	return result;
}

// Runs one character affect's pulse/battle-pulse <actions> on its bearer: kPulse every tick,
// kBattlePulse only while fighting. The bearer is the caster. The affect's duration is threaded through
// so a manual_cast action can end it early. Returns true if any action fired.
bool RunCharAffectTick(CharData *ch, Affect<EApply>::shared_ptr aff) {
	if (!ch || !aff || ch->in_room == kNowhere) {
		return false;
	}
	const bool combat = (ch->GetEnemy() != nullptr);
	std::vector<talents_actions::Action> pulse;
	for (const auto &action : affects::AffectActions(aff->affect_type).list()) {
		const auto &t = action.GetTrigger();
		if (t.test(talents_actions::EActionTrigger::kPulse)
				|| (combat && t.test(talents_actions::EActionTrigger::kBattlePulse))) {
			pulse.push_back(action);
		}
	}
	// Let perks mutate the per-tick actions (on the copy; the affect config stays intact).
	if (!feats::AffectTalentPatches(aff->affect_type).empty()) {
		ApplyMutatingAffectPatches(pulse, ch, aff->affect_type,
				[combat](const talents_actions::Action &a) {
					const auto &tt = a.GetTrigger();
					return tt.test(talents_actions::EActionTrigger::kPulse)
						|| (combat && tt.test(talents_actions::EActionTrigger::kBattlePulse));
				});
	}
	if (pulse.empty()) {
		return false;
	}
	const int phase = aff->apply_time > 0 ? aff->apply_time - 1 : 0;
	int dur = aff->duration;
	// Автора тика ищем по всем узлам этого типа, см. SelectAffectAuthorUid.
	const long author_uid = SelectAffectAuthorUid(ch->affected, aff);
	// Spell-free tick on the affect's stored potency. The bearer is the caster, so a <damage> action must
	// target kTarFightSelf explicitly (a lone action's kTarSame resolves to an empty list). The affect's
	// kDamageTo* flavor is passed, otherwise a DoT deals damage silently.
	CastRoomTickActionFromActions(ch, world[ch->in_room], ESpell::kUndefined, pulse, phase, &dur, aff->potency,
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToChar),
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToVict),
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToRoom),
			author_uid);
	aff->duration = dur;
	// The affect's additive talent-patches matching the pulse triggers (fresh ctx, tick's potency/author).
	if (!ch->purged() && !feats::AffectTalentPatches(aff->affect_type).empty()) {
		ActionContext pctx = BuildActionContext(ch, ESpell::kUndefined, GetRealLevel(ch), aff->potency);
		pctx.SetDamageAuthorUid(author_uid);
		RunAdditiveAffectPatches(pctx, ch, world[ch->in_room], aff->affect_type,
				[combat](const talents_actions::Action &a) {
					const auto &tt = a.GetTrigger();
					return tt.test(talents_actions::EActionTrigger::kPulse)
						|| (combat && tt.test(talents_actions::EActionTrigger::kBattlePulse));
				});
	}
	// A DoT with <charges max=N> lasts exactly N ticks, in or out of combat, independent of the duration.
	// When depleted, mark it expired (duration 0) for the caller's normal removal instead of erasing from
	// ch->affected mid-iteration. Unlimited charges (the default): duration-bound only.
	if (!ch->purged() && aff->charges != kUnlimitedAffectCharges && --aff->charges <= 0) {
		aff->duration = 0;
	}
	return true;
}

// Runs ONE affect type's <actions> matching `trig`, as caster `ch`, with event.actor = `actor` (a
// per-affect lifecycle event; RunCharEventTriggers handles per-hit events across all affects). The caller
// picks `ch`: for kExpired the bearer (actor = null); for kDispell the DISPELLER, so a kTarFightSelf
// retaliation is self-damage, not PvP against the bearer. Recursion is bounded in RunRoomCycledAction.
bool RunCharAffectTrigger(CharData *ch, EAffect affect_type, talents_actions::EActionTrigger trig,
						  CharData *actor, int event_amount, int event_category) {
	if (!ch || ch->in_room == kNowhere) {
		return false;
	}
	std::vector<talents_actions::Action> fired;
	for (const auto &a : affects::AffectActions(affect_type).list()) {
		if (!a.GetTrigger().test(trig)) {
			continue;
		}
		// kPoints category filter: an action pinned to a <trigger category=> fires only for that category.
		// Uncategorized actions and non-kPoints triggers (event_category == -1) always pass.
		if (event_category >= 0 && a.GetTriggerPointsCategory() >= 0
				&& a.GetTriggerPointsCategory() != event_category) {
			continue;
		}
		fired.push_back(a);
	}
	// Let perks mutate the lifecycle actions (on the copy).
	if (!feats::AffectTalentPatches(affect_type).empty()) {
		ApplyMutatingAffectPatches(fired, ch, affect_type,
				[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
	}
	if (fired.empty()) {
		return false;
	}
	const int level = ch->IsNpc() ? GetRealLevel(ch) : 1;
	ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level);
	ctx.UseExternalActions(&fired);
	// kTarActor resolves off ctx.cvict, so point it at the actor (the dispeller for kDispell, the healer
	// for kPoints); otherwise the retaliation would hit the bearer. Actor-less triggers leave it null.
	ctx.cvict = actor;
	EventContext ev;
	ev.trigger = trig;
	ev.actor = actor;
	ev.amount = event_amount;
	ev.points_category = event_category;
	ctx.SetEvent(ev);
	// The affect's own damage flavor (kDamageTo*) for any <damage> in the chain; empty -> generic line.
	ctx.SetAffectDamageMsg(affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToChar),
						   affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToVict),
						   affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToRoom));
	RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
	// The affect's additive talent-patches for this trigger.
	RunAdditiveAffectPatches(ctx, ch, world[ch->in_room], affect_type,
			[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
	return true;
}

// Fires the DYING char's kDeath actions (from die(), before raw_kill) on the char itself, with
// event.actor = the killer. Returns true if any resolved a block (<trigger return="0"/> or a handler
// override): die() then prevents the death. The preventing affect must heal the char itself -- an
// unhealed block just delays death to the next tick (by design).
bool RunCharDeathTriggers(CharData *ch, CharData *killer) {
	if (!ch || ch->in_room == kNowhere || ch->affected.empty()) {
		return false;
	}
	const int level = ch->IsNpc() ? GetRealLevel(ch) : 1;
	bool prevented = false;
	std::vector<EAffect> seen;
	std::vector<EAffect> spend_after;   // types that actually PREVENTED death -> spend a charge (post-loop)
	for (const auto &aff : ch->affected) {
		if (!aff || std::find(seen.begin(), seen.end(), aff->affect_type) != seen.end()) {
			continue;
		}
		std::vector<talents_actions::Action> fired;
		for (const auto &a : affects::AffectActions(aff->affect_type).list()) {
			if (a.GetTrigger().test(talents_actions::EActionTrigger::kDeath)) {
				fired.push_back(a);
			}
		}
		if (fired.empty()) {
			continue;
		}
		seen.push_back(aff->affect_type);
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level);
		ctx.UseExternalActions(&fired);
		ctx.cvict = ch;   // self: a <points><heal> / kTarFightSelf lands on the dying char
		EventContext ev;
		ev.trigger = talents_actions::EActionTrigger::kDeath;
		ev.actor = killer;
		ctx.SetEvent(ev);
		ctx.SetAffectDamageMsg(affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToChar),
							   affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToVict),
							   affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToRoom));
		RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		if (ch->purged()) {
			return prevented;   // a kDeath action purged the char -- can't prevent what's already gone
		}
		// Optional kDeath flavor, shown after the action (so it follows the heal). $n = the bearer.
		const std::string &to_char = affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDeathToChar);
		if (!to_char.empty()) {
			act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar);
		}
		const std::string &to_room = affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDeathToRoom);
		if (!to_room.empty()) {
			act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		// Resolve the "prevent death" verdict: a manual_cast handler's ctx override wins, else the
		// action's static <trigger return="0"/> tag. 0 == block (mirrors kEntryTriggerBlock).
		std::optional<int> ret = ctx.GetTriggerReturn();
		if (!ret) {
			for (const auto &a : fired) {
				if (a.GetTriggerReturn()) { ret = a.GetTriggerReturn(); break; }
			}
		}
		if (ret && *ret == 0) {
			prevented = true;
				// A charge is spent only when death is actually prevented. Deferred until after the loop so removing
				// a depleted affect can't invalidate the iteration over ch->affected.
			spend_after.push_back(aff->affect_type);
		}
	}
	for (const EAffect t : spend_after) {
		SpendCharAffectCharge(ch, t);
	}
	return prevented;
}

// Runs every action on the bearer's affects whose trigger matches event.trigger (e.g. kPreHit/kPostHit),
// with `event` on the context for manual_cast handlers. NPCs proc at their real level, PCs at level 1.
// Snapshots the affect TYPES first because a fired side-spell may mutate ch->affected.
bool RunCharEventTriggers(CharData *ch, const EventContext &event) {
	if (!ch || ch->in_room == kNowhere || ch->affected.empty() || !event.valid()) {
		return false;
	}
	const talents_actions::EActionTrigger trig = event.trigger;
	std::vector<EAffect> types;   // unique affect types carrying a matching action (few affects -> linear dedup)
	for (const auto &aff : ch->affected) {
		if (!aff || std::find(types.begin(), types.end(), aff->affect_type) != types.end()) {
			continue;
		}
		for (const auto &a : affects::AffectActions(aff->affect_type).list()) {
			if (a.GetTrigger().test(trig)) {
				types.push_back(aff->affect_type);
				break;
			}
		}
	}
	if (types.empty()) {
		return false;
	}
	const int level = ch->IsNpc() ? GetRealLevel(ch) : 1;
	for (const EAffect t : types) {
		std::vector<talents_actions::Action> fired;
		for (const auto &a : affects::AffectActions(t).list()) {
			if (a.GetTrigger().test(trig)) {
				fired.push_back(a);
			}
		}
		// Let perks mutate the per-hit actions (on the copy).
		if (!feats::AffectTalentPatches(t).empty()) {
			ApplyMutatingAffectPatches(fired, ch, t,
					[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
		}
		// Competence-based event actions scale with the firing affect's stored potency (the on-kill kHaste is
		// as strong as the vampirism behind it). base="tag" actions read the event amount instead.
		float aff_potency = 0.0f;
		for (const auto &a : ch->affected) {
			if (a && a->affect_type == t) { aff_potency = a->potency; break; }
		}
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level, -1.0f, aff_potency);
		ctx.UseExternalActions(&fired);
		ctx.SetEvent(event);              // handlers (and propagated side-spell sub-ctxs) read this
		if (!fired.empty()) {   // a remove/replace_all patch may have emptied the affect's own actions
			RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		}
		if (ch->purged()) {
			return true;
		}
		// The affect's additive talent-patches (e.g. kSoulLink's master heal).
		if (RunAdditiveAffectPatches(ctx, ch, world[ch->in_room], t,
				[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); })) {
			return true;
		}
		// kPreHit/kPostHit/kKill keep the affect, so charges bound how often it fires. Safe: we iterate the
		// `types` snapshot, not ch->affected.
		SpendCharAffectCharge(ch, t);
	}
	return true;
}

// Obj-affect analog of RunCharEventTriggers: runs a weapon's obj-affect actions on kWeaponHit, with
// cvict = victim (for kTarActor/kTarFightVict) and ovict = the weapon.
bool RunObjAffectWeaponHit(ObjData *weapon, CharData *ch, CharData *victim, int dam) {
	if (!weapon || !ch || !victim || dam <= 0 || ch->in_room == kNowhere) {
		return false;
	}
	if (weapon->get_type() != EObjType::kWeapon || weapon->get_obj_affects().empty()) {
		return false;
	}
	using talents_actions::EActionTrigger;
	std::vector<obj_affects::EObjAffect> types;
	for (const auto &aff : weapon->get_obj_affects()) {
		if (!aff || std::find(types.begin(), types.end(), aff->affect_type) != types.end()) {
			continue;
		}
		for (const auto &a : obj_affects::ObjAffectActions(aff->affect_type).list()) {
			if (a.GetTrigger().test(EActionTrigger::kWeaponHit)) {
				types.push_back(aff->affect_type);
				break;
			}
		}
	}
	if (types.empty()) {
		return false;
	}
	EventContext event;
	event.trigger = EActionTrigger::kWeaponHit;
	event.amount = dam;
	event.weapon = weapon;
	event.actor = victim;
	const int level = GetRealLevel(ch);
	bool any = false;
	for (const obj_affects::EObjAffect t : types) {
		std::vector<talents_actions::Action> fired;
		for (const auto &a : obj_affects::ObjAffectActions(t).list()) {
			if (a.GetTrigger().test(EActionTrigger::kWeaponHit)) {
				fired.push_back(a);
			}
		}
		if (fired.empty()) {
			continue;
		}
		float aff_potency = 0.0f;
		for (const auto &a : weapon->get_obj_affects()) {
			if (a && a->affect_type == t) { aff_potency = a->potency; break; }
		}
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level, -1.0f, aff_potency);
		ctx.cvict = victim;   // <action target="kTarActor"> resolves off cvict (see RunCharAffectTrigger)
		ctx.ovict = weapon;
		ctx.UseExternalActions(&fired);
		ctx.SetEvent(event);
		RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		any = true;
		if (ch->purged() || victim->purged()) {
			return true;
		}
		obj_affects::SpendCharge(weapon, t);   // no-op unless the affect has limited charges
	}
	return any;
}

// Runs one obj-affect trigger action with the actor as the (self-inflicting) caster: emits the trigger
// flavor, runs the action and returns the <trigger return=> verdict (0 = block the actor's action).
static int CastObjTriggerAction(CharData *caster, ObjData *obj, CharData *actor,
		obj_affects::EObjAffect affect_type, const talents_actions::Action &action, float potency) {
	if (actor) {
		const std::string &to_char = obj_affects::ObjAffectMsgRaw(affect_type,
				obj_affects::EObjAffectMsgType::kTriggerToChar);
		if (!to_char.empty()) { act(to_char.c_str(), false, actor, obj, nullptr, kToChar); }
		const std::string &to_room = obj_affects::ObjAffectMsgRaw(affect_type,
				obj_affects::EObjAffectMsgType::kTriggerToRoom);
		if (!to_room.empty()) { act(to_room.c_str(), true, actor, obj, actor, kToRoom | kToArenaListen); }
	}
	const std::vector<talents_actions::Action> single{action};
	ActionContext ctx = BuildActionContext(caster, ESpell::kUndefined, GetRealLevel(caster), potency);
	ctx.cvict = actor;   // <action target="kTarActor"> -> the actor (the trap's victim)
	ctx.ovict = obj;
	// Give a triggered <damage> the affect's own flavor (kDamageTo*) instead of the generic melee-hit line
	// ("X painfully hit you") -- a trap deals impersonal damage, it is not a character attacking.
	ctx.SetAffectDamageMsg(
			obj_affects::ObjAffectMsgRaw(affect_type, obj_affects::EObjAffectMsgType::kDamageToChar),
			obj_affects::ObjAffectMsgRaw(affect_type, obj_affects::EObjAffectMsgType::kDamageToVict),
			obj_affects::ObjAffectMsgRaw(affect_type, obj_affects::EObjAffectMsgType::kDamageToRoom));
	ctx.UseExternalActions(&single);
	RunRoomCycledAction(ctx, world[caster->in_room], single, 0);
	if (const auto h = ctx.GetTriggerReturn()) { return *h; }
	if (const auto t = action.GetTriggerReturn()) { return *t; }
	return 1;   // allow
}

// Fires an object's obj-affect actions matching `trig`. The actor is the (self-inflicting) caster, so a
// trap harms whoever triggers it. Returns false to refuse the actor's action (a <trigger return="0">
// action, e.g. kDartTrap blocking pick/open). Spends trigger charges.
bool RunObjAffectTrigger(ObjData *obj, CharData *actor, talents_actions::EActionTrigger trig) {
	if (!obj || obj->get_obj_affects().empty()) {
		return true;
	}
	// Snapshot so an action that consumes a charge (removing the affect) cannot invalidate iteration.
	struct Pending { obj_affects::EObjAffect type; long caster_id; float potency; };
	std::vector<Pending> pending;
	for (const auto &aff : obj->get_obj_affects()) {
		if (aff) { pending.push_back({aff->affect_type, aff->caster_id, aff->potency}); }
	}
	bool allowed = true;
	for (const auto &p : pending) {
		for (const auto &action : obj_affects::ObjAffectActions(p.type).list()) {
			if (!action.GetTrigger().test(trig)) {
				continue;
			}
			// The affect is SELF-INFLICTING: the actor who triggered it is the caster AND the victim, so
			// the effect (trap damage, poison) never engages combat with whoever SET the affect. With
			// fixed-strength actions (<amount beta="0">) self-sourcing does not weaken it. For an actorless
			// trigger (kDecay) fall back to the setter so the action can still fire. caster_id = provenance.
			CharData *caster = actor ? actor : find_char(p.caster_id);
			if (!caster || caster->in_room == kNowhere) {
				continue;
			}
			const int ret = CastObjTriggerAction(caster, obj, actor, p.type, action, p.potency);
			obj_affects::SpendCharge(obj, p.type);   // spend one trigger charge (removes the affect at 0)
			if (ret == 0) {
				allowed = false;   // refuse the actor's action, but keep firing the rest
			}
			if (actor && actor->purged()) {
				return allowed;   // the actor died from the trap
			}
		}
	}
	return allowed;
}

// On-entry trigger return values: 0 blocks the action that fired the trigger, anything else allows.
// "No value set" means allow, so non-blocking affects need no return attr.
namespace {
constexpr int kEntryTriggerAllow = 1;
constexpr int kEntryTriggerBlock = 0;
}

// Spends one trigger charge (no-op for unlimited charges). At 0 the affect is removed from its host
// list (the room's or the exit's) with its kAffExpired line sent to the actor's room.
static void ConsumeAffectCharge(const Affect<room_spells::ERoomApply>::shared_ptr &aff, RoomData *room,
								room_spells::RoomAffects &host, CharData *actor) {
	if (!aff || aff->charges == kUnlimitedAffectCharges) {
		return;
	}
	if (--aff->charges <= 0) {
		// Running out of charges counts as expiry: fire kExpired (in the caster's context) before removal.
		room_spells::RunRoomAffectTrigger(room, find_char(aff->caster_id), aff->affect_type,
										  talents_actions::EActionTrigger::kExpired, aff->potency);
		const std::string &msg =
				room_spells::RoomAffectMsgRaw(aff->affect_type, room_spells::ERoomAffectMsgType::kAffExpiredToRoom);
		if (!msg.empty() && actor && actor->in_room != kNowhere) {
			SendMsgToRoom(msg.c_str(), actor->in_room, 0);
		}
		host.remove(aff);
	}
}

// Run ONE event-triggered (kEnter/kEnterPC) action on the actor. `caster` (the affect's owner) sources
// the cast; `actor` is the target -- carried on ctx.cvict and resolved by EActionTarget::kTarActor.
// Returns the trigger's return value: a manual_cast handler's override (set on ctx) wins, else the
// action's <trigger return=> tag, else "allow". Returns allow without acting if the actor fails the
// action's <target_conditions> (so a filtered-out actor neither gets the effect nor blocks).
static int CastRoomEntryAction(CharData *caster, RoomData *room, CharData *actor,
							   room_spells::ERoomAffect affect_type,
							   const talents_actions::Action &action, float potency, int seal_strength,
							   talents_actions::EActionTrigger ev) {
	if (TargetIsBlocked(actor, action.GetBlocking())
			|| !TargetMeetsRequired(actor, action.GetRequired())) {
		return kEntryTriggerAllow;
	}
	// prob gate: a miss leaves the event untouched (no effect, no block). The chance is the affect's seal
	// strength (Affect::modifier, e.g. kForbidden) if it has a seal_strength formula, else <trigger prob=N>,
	// else always.
	const std::optional<int> prob = room_spells::RoomAffectHasSeal(affect_type)
			? std::optional<int>(seal_strength)
			: action.GetTriggerProb();
	if (prob && number(1, 100) > *prob) {
		return kEntryTriggerAllow;
	}
	// The affect's flavor, shown BEFORE the effect and keyed by which trigger fired, so walking through /
	// opening / picking / unlocking read distinctly. Silent if not authored.
	using room_spells::ERoomAffectMsgType;
	ERoomAffectMsgType char_key = ERoomAffectMsgType::kTriggerOnEntryToChar;
	ERoomAffectMsgType room_key = ERoomAffectMsgType::kTriggerOnEntryToRoom;
	switch (ev) {
		case talents_actions::EActionTrigger::kOpen:
			char_key = ERoomAffectMsgType::kTriggerOnOpenToChar;
			room_key = ERoomAffectMsgType::kTriggerOnOpenToRoom;
			break;
		case talents_actions::EActionTrigger::kPick:
			char_key = ERoomAffectMsgType::kTriggerOnPickToChar;
			room_key = ERoomAffectMsgType::kTriggerOnPickToRoom;
			break;
		case talents_actions::EActionTrigger::kUnlock:
			char_key = ERoomAffectMsgType::kTriggerOnUnlockToChar;
			room_key = ERoomAffectMsgType::kTriggerOnUnlockToRoom;
			break;
		case talents_actions::EActionTrigger::kClose:
			char_key = ERoomAffectMsgType::kTriggerOnCloseToChar;
			room_key = ERoomAffectMsgType::kTriggerOnCloseToRoom;
			break;
		case talents_actions::EActionTrigger::kLock:
			char_key = ERoomAffectMsgType::kTriggerOnLockToChar;
			room_key = ERoomAffectMsgType::kTriggerOnLockToRoom;
			break;
		default:  // kEnter family (move-through) and pulse
			break;
	}
	const std::string &to_char = room_spells::RoomAffectMsgRaw(affect_type, char_key);
	if (!to_char.empty()) { act(to_char.c_str(), false, actor, nullptr, nullptr, kToChar); }
	const std::string &to_room = room_spells::RoomAffectMsgRaw(affect_type, room_key);
	if (!to_room.empty()) { act(to_room.c_str(), true, actor, nullptr, actor, kToRoom | kToArenaListen); }
	const std::vector<talents_actions::Action> single{action};
	ActionContext ctx = BuildActionContext(caster, ESpell::kUndefined, GetRealLevel(caster), potency);
	ctx.cvict = actor;
	ctx.UseExternalActions(&single);
	const ECastResult eff = RunRoomCycledAction(ctx, room, single, 0);
	// A fired trigger with no visible effect (refused cast, or a violent side_spell suppressed by a peaceful
	// room) would look like a silent no-op, so emit the affect's kTriggerNoEffect flash.
	if (action.Contains(talents_actions::EAction::kSideSpell)) {
		bool no_effect = (eff == ECastResult::kNotCast);
		if (!no_effect && ROOM_FLAGGED(actor->in_room, ERoomFlag::kPeaceful)) {
			for (const auto s : action.GetSideSpells()) {
				if (MUD::Spell(s).IsViolent()) { no_effect = true; break; }
			}
		}
		if (no_effect) {
			const std::string &flash = room_spells::RoomAffectMsgRaw(affect_type, ERoomAffectMsgType::kTriggerNoEffect);
			if (!flash.empty()) { SendMsgToRoom(flash.c_str(), actor->in_room, 0); }
		}
	}
	if (const auto h = ctx.GetTriggerReturn()) { return *h; }       // handler override wins
	if (const auto t = action.GetTriggerReturn()) { return *t; }    // else the <trigger return=> tag
	return kEntryTriggerAllow;                                       // else allow
}

bool room_spells::RunRoomEntryTriggers(CharData *actor, RoomData *room,
									   room_spells::EEntryTriggerPhase phase) {
	using room_spells::EEntryTriggerPhase;
	if (!actor || !room || privilege::IsImmortal(actor)) {
		return true;   // affect triggers never fire on (or block) immortals
	}
	const bool actor_is_pc = !actor->IsNpc();
	// Snapshot {affect, caster, potency, strength} so running an action (which could alter
	// room->affected) cannot invalidate the iteration. `strength` is the affect's seal strength
	// (Affect::modifier) -- the block chance for seal-strength affects (kForbidden).
	struct Pending { room_spells::ERoomAffect type; long caster_id; float potency; int strength;
					 Affect<room_spells::ERoomApply>::shared_ptr aff; };
	std::vector<Pending> pending;
	for (const auto &aff : room->affected) {
		if (aff) { pending.push_back({aff->affect_type, aff->caster_id, aff->potency, aff->modifier, aff}); }
	}
	bool allowed = true;
	for (const auto &p : pending) {
		for (const auto &action : room_spells::RoomAffectActions(p.type).list()) {
			const auto &trig = action.GetTrigger();
			const bool fires = trig.test(talents_actions::EActionTrigger::kEnter)
					|| (actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterPC))
					|| (!actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterNPC));
			if (!fires) { continue; }
			// An action "can block" iff it yields a non-default return: a <trigger return=> tag or a
			// manual_cast handler that may set one. Such actions run in the BEFORE-placement kBlockCheck
			// pass (where return=0 refuses the move); pure-effect actions (the common case) run in the
			// AFTER-placement effect passes so their effect/messages keep destination-room context.
			const bool can_block = action.GetTriggerReturn().has_value() || !action.GetManualHandler().empty();
			if (phase == EEntryTriggerPhase::kBlockCheck && !can_block) { continue; }
			if (phase == EEntryTriggerPhase::kEffectsNonBlocking && can_block) { continue; }
			// Действию без следствий в проходе за следствиями делать нечего (у печати -- один вердикт
			// <trigger return="0"/>). Принудительный вход (скакун за седоком, телепорт, бегство) вердикта не
			// смотрит, и иначе печать на каждом шаге сообщала бы уже вошедшему, что его не пускают. Вердикт
			// спрашивают в kBlockCheck, пока вход ещё можно отменить.
			if (phase != EEntryTriggerPhase::kBlockCheck && !action.HasEffects()) { continue; }
			CharData *caster = find_char(p.caster_id);
			if (!caster) { continue; }   // no owner to source the cast this entry
			const int ret = CastRoomEntryAction(caster, room, actor, p.type, action, p.potency, p.strength,
					talents_actions::EActionTrigger::kEnter);   // room entry -> the "entry" message pair
			ConsumeAffectCharge(p.aff, room, room->affected, actor);   // spend a trigger charge (no-op if unlimited)
			// Only the block-check pass enforces the verdict; the effect passes ignore it.
			if (phase == EEntryTriggerPhase::kBlockCheck && ret == kEntryTriggerBlock) {
				allowed = false;   // block, but keep running the rest (so all blocking actions still fire)
			}
		}
	}
	return allowed;
}

// Fires door affects when someone acts on the door in direction `dir` from `room`. Like
// RunRoomEntryTriggers, but the host is the exit's affect list and there is a single dispatch point
// (the door command), so no phase split. Returns false to refuse the door action.
bool room_spells::RunDoorTriggers(CharData *actor, RoomData *room, int dir, talents_actions::EActionTrigger ev) {
	if (!actor || !room || dir < 0 || dir >= EDirection::kMaxDirNum || privilege::IsImmortal(actor)) {
		return true;   // affect triggers never fire on (or block) immortals
	}
	const auto near_exit = room->dir_option[dir];
	if (!near_exit) { return true; }
	// Snapshot {affect, caster, potency, strength} so running an action (which could alter the exit's
	// affect list) cannot invalidate the iteration.
	struct Pending { room_spells::ERoomAffect type; long caster_id; float potency; int strength;
					 Affect<room_spells::ERoomApply>::shared_ptr aff; RoomData::exit_data_ptr host; };
	std::vector<Pending> pending;
	auto collect = [&pending](const RoomData::exit_data_ptr &ex) {
		if (!ex) { return; }
		for (const auto &aff : ex->affected) {
			if (aff) { pending.push_back({aff->affect_type, aff->caster_id, aff->potency, aff->modifier, aff, ex}); }
		}
	};
	collect(near_exit);
	// A door affect cast from the far room lives on the reverse exit, so collect that side too (one
	// validated hop, like do_doorcmd's `back`) -- the trap reacts whichever side opens the door.
	if (near_exit->to_room() != kNowhere) {
		static const int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth,
									   EDirection::kEast, EDirection::kDown, EDirection::kUp};
		const auto rev_exit = world[near_exit->to_room()]->dir_option[rev_dir[dir]];
		if (rev_exit && rev_exit->to_room() == actor->in_room) {
			collect(rev_exit);
		}
	}
	const bool actor_is_pc = !actor->IsNpc();
	bool allowed = true;
	for (const auto &p : pending) {
		for (const auto &action : room_spells::RoomAffectActions(p.type).list()) {
			const auto &trig = action.GetTrigger();
			// kEnter here means the enter family (passage move-through), expanded by actor kind like
			// RunRoomEntryTriggers; door verbs are matched verbatim.
			bool fires = trig.test(ev);
			if (ev == talents_actions::EActionTrigger::kEnter) {
				fires = fires || (actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterPC))
						|| (!actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterNPC));
			}
			if (!fires) { continue; }
			CharData *caster = find_char(p.caster_id);
			if (!caster) { continue; }   // no owner to source the cast
			const int ret = CastRoomEntryAction(caster, room, actor, p.type, action, p.potency, p.strength, ev);
			ConsumeAffectCharge(p.aff, room, p.host->affected, actor);   // spend a trigger charge (no-op if unlimited)
			if (ret == kEntryTriggerBlock) {
				allowed = false;   // refuse the door action, but keep firing the rest
			}
		}
	}
	return allowed;
}

// Casts `spell_id` as an area attack on every foe in the caster's room, regardless of the spell's own
// targeting flags. Used by room-affect ticks (deadly fog / thunderstorm).
ECastResult CastAreaInRoom(CharData *ch, ESpell spell_id, int level) {
	auto ctx = BuildActionContext(ch, spell_id, level);
	return CastSpell(ctx, ECastTargets::kFoes);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
