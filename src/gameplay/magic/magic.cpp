/* ************************************************************************
*   File: magic.cpp                                     Part of Bylins    *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "magic.h"
#include "administration/privilege.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/condition.h"
#include "gameplay/fight/fight_stuff.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/resist.h"
#include "magic_internal.h"
#include "spell_trace.h"
#include "gameplay/mechanics/saving.h"
#include "gameplay/affects/affect_contants.h"  // NAME_BY_ITEM<EApply>
#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/mechanics/summon.h"

#include <functional>
#include <map>
#include "magic_rooms.h"  // room-affect helpers reused by CastUnaffects' room branch
#include "room_affect_messages.h"  // RoomAffectMsgRaw for room dispel narration

#include "gameplay/core/game_limits.h"  // gain_condition
#include "gameplay/mechanics/liquid.h"   // kMaxCondition
#include "engine/core/target_resolver.h"

#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/mechanics/poison.h"   // issue.damage-over-time: CalcPoisonDamage for <damage source="poison">
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
#include "gameplay/abilities/abilities_rollsystem.h"   // issue.instant-death: OppositeLuckTest

extern int interpolate(int min_value, int pulse);

byte GetSavingThrows(ECharClass class_id, ESaving type, int level);    // class.cpp
void ReactToCast(CharData *victim, CharData *caster, ESpell spell_id);

bool IsRoomForbidden(RoomData *room) {
	for (const auto &af: room->affected) {
		// issue.affect-migration: identify the ward by its room-affect identity, not the casting spell.
		if (af->affect_type == room_spells::ERoomAffect::kForbidden && (number(1, 100) <= af->modifier)) {
			return true;
		}
	}
	return false;
}

// * Saving throws are now in class.cpp as of bpl13.

/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */

int bonus_antisaving[] ={
			-9,-8,-7,-6,-5,-4,-3,-2,-1, 0,
			1, 2, 3 ,4, 5, 6, 7, 8, 9, 10,
			11,12,13,14,15,16,17,18,19,20,
			21,21,22,22,23,23,24,24,25,25,
			26,26,27,27,28,28,29,29,30,30,
			31,31,32,32,33,33,34,34,35,35,
			36,36,37,37,38,38,39,39,40,40,
			41,41,41,42,42,42,43,43,43,44,
			44,44,45,45,45,46,46,46,47,47,
			47,48,48,48,49,49,49,50,50,50,
};

int CalcAntiSavings(CharData *ch) {
	int modi = 0;
	if (privilege::IsImmortal(ch))
		modi = 1000;
	else if (GET_GOD_FLAG(ch, EGf::kGodsLike))
		modi = 250;
	else if (GET_GOD_FLAG(ch, EGf::kGodscurse))
		modi = -250;
	else
		modi = ch->add_abils.cast_success;
	modi += bonus_antisaving[GetRealWis(ch) - 1];
	if (!ch->IsNpc()) {
		modi *= condition::GetCondPenalty(ch, condition::kCast);
	}
//  log("[EXT_APPLY] Name==%s modi==%d",GET_NAME(ch), modi);
	return modi;
}

int CalcClassAntiSavingsMod(CharData *ch, ESpell spell_id) {
	auto mod = MUD::Class(ch->GetClass()).spells[spell_id].GetCastMod();
	auto skill = GetSkill(ch, MUD::Spell(spell_id).GetSuccessRoll().GetBaseSkill());
	return static_cast<int>(mod*skill);
}


//killer нужен для того чтоб вывести стату


// issue.npc-races: a mob can speak/incant iff its race carries the <vocal/> trait (players always
// can). Replaces the old per-race switch here and the duplicate IsCasterVerbal in magic_utils.cpp.
bool IsAbleToSay(CharData *ch) {
	if (!ch->IsNpc()) {
		return true;
	}
	return MUD::MobRaces()[GET_RACE(ch)].IsVocal();
}

void ShowAffExpiredMsg(EAffect affect_type, CharData *ch) {
	if (!ch->IsNpc() && ch->IsFlagged(EPlrFlag::kWriting)) {
		return;
	}

	// issue.affect-migration: expiry narration belongs to the AFFECT, keyed by its identity. The char
	// always learns the affect wore off (kDefault generic fallback for affect_type == kUndefined); the
	// room is told only for affects whose expiry is visible to others (sheaf-direct kAffExpiredToRoom,
	// silent if unauthored).
	const std::string &to_char = affects::AffectMsg(affect_type, affects::EAffectMsgType::kAffExpiredToChar);
	const std::string &to_room = affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kAffExpiredToRoom);
	if (!to_char.empty()) {
		act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar | kToSleep);
		SendMsgToChar("\r\n", ch);
	}
	if (!to_room.empty()) {
		act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
	}
}


// SetBattleLag(victim, lag) lives inline next to SetWaitState in char_data.h -- it isn't
// magic-specific. This file keeps only the magic-pipeline-specific skill-scaled overload below.

/*
 * This is a temporary function. It's better to tie the lag value to the success rate of the cast
 * rather than directly to the pure skill. But to calculate the success rate, we need a function
 * for calculating the attack rating and a function for calculating the defense rating.
 * These don't exist and can't be written quickly. Replacing them with placeholders defeats the purpose.
 * The placeholders will still return values different from those ultimately used in the system,
 * and after implementing the rating functions, the spells will have to be rebalanced.
 * Therefore, it is easier to use a temporary function with transparent logic.
 */
void SetBattleLag(CharData *victim, double skill_bonus, unsigned base_lag, double bonus_divisor) {
	if (bonus_divisor > 0) {
		const auto lag = base_lag + static_cast<int>(skill_bonus / bonus_divisor);
		SetBattleLag(victim, lag);
	} else {
		SetBattleLag(victim, base_lag);
	}
}

double CalcMagicElementCoeff(CharData *victim, ESpell spell_id) {
	double element_coeff = 1.0;
	if (victim->IsNpc()) {
		if (NPC_FLAGGED(victim, ENpcFlag::kFireCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kFire) {
				element_coeff /= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kWater) {
				element_coeff *= 2.0;
			}
		}
		if (NPC_FLAGGED(victim, ENpcFlag::kAirCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kEarth) {
				element_coeff *= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kAir) {
				element_coeff /= 2.0;
			}
		}
		if (NPC_FLAGGED(victim, ENpcFlag::kWaterCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kFire) {
				element_coeff *= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kWater) {
				element_coeff /= 2.0;
			}
		}
		if (NPC_FLAGGED(victim, ENpcFlag::kEarthCreature)) {
			if (MUD::Spell(spell_id).GetElement() == EElement::kEarth) {
				element_coeff /= 2.0;
			} else if (MUD::Spell(spell_id).GetElement() == EElement::kAir) {
				element_coeff *= 2.0;
			}
		}
	}
	return element_coeff;
}


// as a local lambda that handles every category (heal / moves / thirst /
// cond) uniformly.

/**
 * Number of *extra* hits a multi-hit damage spell deals beyond its single mandatory hit.
 * The caller adds 1 for the base hit. Three modes:
 *   - no skill / kUndefined: prob% chance of `max`, else 0;
 *   - skill + prob > 0:      prob% chance of `extra` (= min(skill,75)/divisor, capped at max), else 0;
 *   - skill + prob == 0:     random 0..extra (uniform spread of attack count).
 * (issue.perk-action-patching) kMagicArrows is now DATA-DRIVEN: the feat carries a <talent_patch> that
 * replaces kMagicMissile's <damage> with a boosted <hits> block (skill_divisor 12->6, max 2->6),
 * reproducing the old halve-divisor/triple-max at the skill cap. No spell-specific branch here anymore.
 */
static int CalcExtraHits(CharData *ch, ESkill skill_id,
				  int skill_divisor = 25, int max = 1, int prob = 20) {
	if (ch == nullptr || skill_id == ESkill::kUndefined) {
		return (number(1, 100) <= prob) ? max : 0;
	}
	int extra = CalcNoviceSkillBonus(ch, skill_id, skill_divisor);
	if (extra > max) extra = max;
	if (prob == 0) return number(0, extra);
	return ((number(1, 100) <= prob) ? extra : 0);
}

/**
 * Forces a character to assume a certain position (knocked down, asleep, stunned, etc.)
 * Knocks off horse, checks for afflictions.
 */
// Knock the victim down to position `pos` (with the spell's kKnockdown* messages) and/or
// force the fight to stop. Passing EPosition::kUndefined as `pos` changes no position at all
// -- only the force_stopfight branch runs (in the engine, the "fighting" state is itself part
// of the position system, so stopping a fight is a position change in that sense).
static void ForceReposition(CharData *victim, ESpell spell_id, EPosition pos, bool force_stopfight = false) {
	if (privilege::IsImmortal(victim)) {
		return;
	}
	const bool reposition = (pos != EPosition::kUndefined);
	if (reposition && victim->GetPosition() < pos) {
		return;
	}
	if (((reposition && pos < EPosition::kSit) || force_stopfight) && victim->GetEnemy()) {
		stop_fighting(victim, force_stopfight);
	}
	if (force_stopfight) {
		ChangeFighting(victim, force_stopfight);
	}
	if (reposition) {
		mount::DropFromHorse(victim);
		if (pos < victim->GetPosition()) {
			act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToRoom).c_str(),
				false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kKnockdownToChar).c_str(),
				false, victim, nullptr, nullptr, kToChar);
			victim->SetPosition(pos);
		}
	}
}

// issue.mob-flag-affect-materialization: build the real affect node(s) for a materialized intrinsic mob
// buff. The affect and its granting spell share a name (kSanctuary affect <- kSanctuary spell, etc.), so
// we roll THAT spell's <potency_roll> for the mob -- dice + the mob's skill_coeff + stat_coeff -- exactly
// as a real cast would, and derive both the stored potency (so dispel is a contest, not a free strip) and
// each apply's modifier. competence = skill_coeff + stat_coeff, the same value an entry action feeds
// ComputeApplyModifier (ActionContext::CompetenceBase). An affect with <apply> children (kCloudly ->
// blink+AC, kBlink -> blink) materializes WITH its stat applies -- one node per apply; an applies-less
// buff (sanctuary/shields/prism/glass) gets a single bare flag-carrier node (location kNone). Every node
// is permanent (duration -1) and uncredited (caster_id 0). No same-named spell -> zero roll (flat mins,
// potency 0).
std::vector<Affect<EApply>> BuildMaterializedAffect(const CharData *mob, EAffect affect_type) {
	RollResult roll;   // stays zero if no same-named spell -> potency 0, competence 0 (flat apply mins)
	try {
		const ESpell spell_id = ITEM_BY_NAME<ESpell>(NAME_BY_ITEM<EAffect>(affect_type));
		const auto &pr = MUD::Spell(spell_id).GetPotencyRoll();
		roll.skill_coeff = pr.CalcSkillCoeff(mob);
		roll.stat_coeff = pr.CalcBaseStatCoeff(mob);
	} catch (const std::out_of_range &) {
		// no same-named spell: keep the zero roll (bare/min applies, potency 0)
	}
	const float cast_potency = CalcCastPotency(roll);
	const double competence = roll.skill_coeff + roll.stat_coeff;
	std::vector<Affect<EApply>> nodes;
	auto make = [&](EApply loc, int mod) {
		Affect<EApply> af;
		af.affect_type = affect_type;
		af.duration = -1;             // permanent: never ticks or expires
		af.location = loc;
		af.modifier = mod;
		af.caster_id = 0;
		af.potency = cast_potency;
		nodes.push_back(af);
	};
	const auto &applies = affects::AffectApplies(affect_type);
	if (applies.empty()) {
		make(EApply::kNone, 0);       // flag-only buff (actions-driven): bare carrier, no stat apply
	} else {
		for (const auto &ap : applies) {
			make(ap.location, ComputeApplyModifier(ap, competence, roll));
		}
	}
	return nodes;
}

static int CalcTotalSpellDmg(CharData *ch, CharData *victim, ESpell spell_id,
							const talents_actions::Action &action, double competence,
							double noise_z = std::numeric_limits<double>::quiet_NaN()) {
	const auto &potency_roll = MUD::Spell(spell_id).GetPotencyRoll();
	const bool has_dmg = action.Contains(talents_actions::EAction::kDamage);
	// prob: a <damage prob=> spell may simply not fire (default 100).
	// A miss returns 0 -- which, like a full magic-resist, is still processed downstream (no aggro
	// change was requested), so behaviour matches today's zero-damage handling. The prob<100 guard
	// short-circuits the RNG when the result is fixed at "always fires".
	if (has_dmg) {
		const int dmg_prob = action.GetDmg().GetProb();
		if (dmg_prob < 100 && number(1, 100) > dmg_prob) {
			return 0;
		}
	}
	int total_dmg{0};
	if (number(1, 100) > std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, GET_MR(victim))) {
		const float skill_coeff = potency_roll.CalcSkillCoeff(ch);
		const float stat_coeff = potency_roll.CalcBaseStatCoeff(ch);
		const float bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
		const float elem_coeff = CalcMagicElementCoeff(victim, spell_id);

		float dmg;
		if (has_dmg) {
			// Additive model, mirroring heal: the <amount> weights scale
			// the roll's dice and competencies (skill+stat); an absent <amount> defaults to min 0
			// and both weights 1.0.
			const auto &dmg_act = action.GetDmg();
			// Option-2 subquadratic: skill/stat scales the dice
			// multiplicatively (alpha) plus a flat additive term (beta). alpha=0 -> old
			// Formula A. C = skill_coeff + stat_coeff.
			const float C = static_cast<float>(competence);  // base override
			// issue.potency-noise: one formula -- min + beta*C*(1 + weight*d). For a DoT tick C is the
				// affect's stored potency (fed as fixed_potency -> skill_coeff); noise_z = realized d.
				dmg = static_cast<float>(CalcNoisyAmount(dmg_act.GetAmountMin(),
						dmg_act.GetAmountBeta() * C, dmg_act.GetAmountWeight(), /*cap*/ 0, noise_z));
		} else {
			// issue.random-noise-rework: the legacy `dice * (1 + skill + stat)` model is abandoned --
			// it was dice-dominated (skill barely mattered) and, with the rebalanced competence, both
			// inflated and inconsistent with every <damage> spell. A kMagDamage spell with no <damage>
			// manifestation now deals NO direct damage; it stays an aggressive act (CastDamage already
			// ran pk_agro_action, and a 0-damage hit still sets the victim fighting via Damage::Process),
			// which is exactly what a taunt like kWarcryOfChallenge wants. Real damage comes from a
			// <damage> action or a <side_spell> (e.g. kDeadlyFogTick's kAcidArrow/kPoison/...).
			dmg = 0.0f;
		}

		total_dmg = static_cast<int>(dmg * elem_coeff);
		total_dmg += static_cast<int>(total_dmg * bonus_mod);
		int complex_mod = total_dmg;
		total_dmg = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, total_dmg);
		complex_mod = total_dmg - complex_mod;
		spell_trace::Line(ch, victim,
				"&CMag.dmg (%s -> %s). Skill: %2.2f, Stat: %2.2f, Amount: %2.2f, Bonus: %1.2f, Cmplx: %d, Elem.coeff: %1.2f, Total: %d &n\r\n",
				GET_NAME(ch), GET_NAME(victim), skill_coeff, stat_coeff, dmg, 1 + bonus_mod, complex_mod, elem_coeff, total_dmg);
	}

	return total_dmg;
}

// The remaining code-only defense (magic mirror and sonic barrier are gone: the mirror is now a
// data-driven kWardAttack affect, the barrier was dropped so warcries fizzle on gods like any spell).
// TryBlockByMagicalShield returns true (and emits the standard ToChar/ToNotVict/ToVict message trio)
// when the shield swallows the cast; the caller (RunWholeCastWards) then stops the whole cast.
namespace {

// Build and process one Damage object for a multi-hit damage spell. `count` is the
// remaining-hit counter (1 == last hit) so the no-flee flag is cleared on the final hit only.
// Returns the result of Damage::Process(), reused by the loop's `rand` to decide whether to
// continue (a non-negative value means the victim is still alive).
int LandOneDamageHit(CharData *ch, CharData *victim, ESpell spell_id, int total_dmg,
					 EPosition ch_start_pos, EPosition victim_start_pos, int count,
					 const std::string &aff_msg_char, const std::string &aff_msg_vict,
					 const std::string &aff_msg_room, long author_uid) {
	Damage dmg(SpellDmg(spell_id), total_dmg, fight::kMagicDmg);
	// issue.dot-death-exp: carry the self-damage author (a DoT/affect tick's caster_id) so a kill
	// dealt by the bearer's OWN tick credits the responsible character, exactly like the poison
	// branch above. 0 = no author (a normal offensive hit sets nothing; ch != victim credits ch).
	dmg.author_uid = author_uid;
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;
	// issue.character-affect-triggers: affect-owned damage flavor (empty for ordinary spell damage).
	dmg.aff_msg_char_ = aff_msg_char;
	dmg.aff_msg_vict_ = aff_msg_vict;
	dmg.aff_msg_room_ = aff_msg_room;

	if (CanUseFeat(ch, EFeat::kPowerMagic) && victim->IsNpc()) {
		dmg.flags.set(fight::kIgnoreAbsorbe);
	}
	// отражение магии в кастующего
	if (ch == victim) {
		dmg.flags.set(fight::kMagicReflect);
	}
	if (count <= 1) {
		dmg.flags.reset(fight::kNoFleeDmg);
	} else {
		dmg.flags.set(fight::kNoFleeDmg);
	}
	return dmg.Process(ch, victim);
}

// issue.attack-ward: TryReflectByMagicGlass removed -- the Magic Mirror (kMagicGlass) is now a
// data-driven kWardAttack/reflect affect (its messages live in the affect msg cfg, see RunWholeCastWards).
// issue.attack-ward: TryReflectBySonicBarrier removed too. It was a bespoke "gods reflect warcries"
// workaround that existed only because a violent warcry slipped past the general immortal guard. A
// warcry against a god now simply fails like any other spell -- the IsGod block in CastOnTarget's
// is_entry gate fizzles it with the standard "no effect", no reflection.

// The Vityaz magical-shield block: a skill+feat+worn-shield absorption. The chance is
// (kShieldBlock / 20 + shield_weight / 2) percent. Mass/area/warcry casts bypass the shield.
bool TryBlockByMagicalShield(CharData *ch, CharData *victim, ESpell spell_id) {
	// issue #3554: у каста по объекту/комнате нет char-victim (ctx.cvict == nullptr).
	// Ниже сразу GetSkill(victim,...) -> разыменование nullptr -> краш. Щита-защитника
	// без цели-персонажа нет, поэтому просто выходим (как делает WardEligible).
	if (!victim) return false;
	if (ch == victim) return false;
	if (!MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagWarcry)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagMasses)) return false;
	if (MUD::Spell(spell_id).IsFlagged(kMagAreas)) return false;
	if (GetSkill(victim, ESkill::kShieldBlock) <= 100) return false;
	if (!GET_EQ(victim, EEquipPos::kShield)) return false;
	if (!CanUseFeat(victim, EFeat::kMagicalShield)) return false;
	const int chance = GetSkill(victim, ESkill::kShieldBlock) / 20
		+ GET_EQ(victim, EEquipPos::kShield)->get_weight() / 2;
	if (number(1, 100) >= chance) return false;
	act("Ваши чары повисли на щите $N1, и затем развеялись.", false, ch, nullptr, victim, kToChar);
	act("Щит $N1 поглотил злые чары $n1.", false, ch, nullptr, victim, kToNotVict);
	act("Ваш щит уберег вас от злых чар $n1.", false, ch, nullptr, victim, kToVict);
	return true;
}

// issue.attack-ward: the target's current value of the resist apply an <absorption chance=> names.
// Only kMagicResist is used today (Shadow Cloak); easily extended to the other resist applies.
int WardResistStat(CharData *victim, EApply ap) {
	switch (ap) {
		case EApply::kMagicResist:  return GET_MR(victim);
		case EApply::kPhysicResist: return GET_PR(victim);
		default:                    return 0;
	}
}

// issue.attack-ward: emit a fired ward's flavor (kWardTo{Char,Vict,Room}). Shared by the whole-cast
// gate and the per-manifest absorb so both narrate identically.
void EmitWardMsgs(CharData *caster, CharData *victim, EAffect at) {
	const std::string &mc = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToChar);
	const std::string &mv = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToVict);
	const std::string &mr = affects::AffectMsgRaw(at, affects::EAffectMsgType::kWardToRoom);
	if (!mc.empty()) { act(mc.c_str(), false, caster, nullptr, victim, kToChar); }
	if (!mv.empty()) { act(mv.c_str(), false, caster, nullptr, victim, kToVict); }
	if (!mr.empty()) { act(mr.c_str(), false, caster, nullptr, victim, kToNotVict); }
}

// issue.attack-ward: a defender ward may react only to a violent, non-warcry MAGIC attack by a
// non-god on a distinct same-room victim. `is_magic` is true for every cast today (later sourced
// from the <weave> component so non-magic skills that share this pipeline won't trip wards).
bool WardEligible(CharData *caster, CharData *victim, ESpell spell_id, bool is_magic) {
	return is_magic && caster && victim && caster != victim
		&& caster->in_room == victim->in_room
		&& !privilege::IsGod(caster)
		&& !MUD::Spell(spell_id).IsFlagged(kMagWarcry)
		&& MUD::Spell(spell_id).IsViolentAgainst(caster, victim);
}

// issue.attack-ward (per-manifest wards): WHOLE-CAST defender wards -- reflection (the entire cast
// bounces to the caster) and scope="all" absorption -- plus the code-only magical shield. Evaluated
// ONCE at the is_entry gate, and only for the TOP-LEVEL cast (a side-spell sub-cast is part of the
// same incoming spell, so it must not re-trigger these -- see CastSideSpell / ctx.Nested()). Scoped
// damage/affect absorbs are NOT handled here; they are per-manifest (TryScopedAbsorb) inside the
// stages so each attack rolls independently. First firing ward wins. Returns true if a ward fired.
bool RunWholeCastWards(ActionContext &ctx, bool is_magic) {
	CharData *caster = ctx.caster();
	CharData *victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	if (WardEligible(caster, victim, spell_id, is_magic)) {
		for (const auto &ward : VictimWardAffects(victim)) {
			const EAffect at = ward.first;
			for (const auto &action : affects::AffectActions(at).list()) {
				if (!action.GetTrigger().test(talents_actions::EActionTrigger::kWardAttack)) {
					continue;
				}
				const auto &refl = action.GetReflection();
				const auto &absb = action.GetAbsorption();
				const bool is_reflect = refl.present;
				// Only whole-cast outcomes belong here: a reflect, or an absorption scoped kAll.
				if (!is_reflect
						&& !(absb.present && absb.scope == talents_actions::EWardScope::kAll)) {
					continue;
				}
				int chance;
				if (is_reflect) {
					// potency contest (max>0): clamp(mirror potency - incoming spell potency, min, max);
					// otherwise the fixed prob (spell-side <reflection> / flat-prob reflect).
					chance = (refl.max > 0)
						? std::clamp(static_cast<int>(ward.second - CalcCastPotency(ctx.potency())),
									 refl.min, refl.max)
						: refl.prob;
				} else if (absb.chance != EApply::kNone) {
					// stat-driven: capped GET_<apply>(victim), same clamp as the elemental-resist path.
					const int cap = caster->IsNpc() ? kMaxNpcResist : kMaxPcResist;
					chance = std::min(cap, WardResistStat(victim, absb.chance));
				} else {
					chance = absb.prob;
				}
				if (number(1, 100) > chance) {
					continue;
				}
				EmitWardMsgs(caster, victim, at);
				if (is_reflect) {
					ctx.cvict = caster;   // whole cast bounces back at the attacker
				} else {
					ctx.SetWardStop();    // scope="all": the whole cast is swallowed
				}
				return true;
			}
		}
	}
	// Code-only defense (not migrated): magical shield (absorb-all). A violent warcry against an
	// immortal is no longer reflected here -- it fails via the general IsGod block in CastOnTarget.
	if (TryBlockByMagicalShield(caster, victim, spell_id)) {
		ctx.SetWardStop();
		return true;
	}
	return false;
}

// issue.attack-ward (per-manifest wards): a SCOPED absorption ward (e.g. Shadow Cloak, scope="damage")
// rolls INDEPENDENTLY for each payload delivery. Called from CastDamage (want=kDamage) and CastAffect
// (want=kAffect) right before the stage applies its effect, so: a multi-attack spell gets one roll per
// hit; a spell that carries no matching payload never trips the ward; and a side-spell delivery (a
// nested cast) is warded as the separate attack it is. Returns true if THIS manifest was swallowed
// (the caller skips just this stage). First matching ward wins.
bool TryScopedAbsorb(ActionContext &ctx, talents_actions::EWardScope want) {
	CharData *caster = ctx.caster();
	CharData *victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	if (!WardEligible(caster, victim, spell_id, /*is_magic=*/true)) {
		return false;
	}
	for (const auto &ward : VictimWardAffects(victim)) {
		const EAffect at = ward.first;
		for (const auto &action : affects::AffectActions(at).list()) {
			if (!action.GetTrigger().test(talents_actions::EActionTrigger::kWardAttack)) {
				continue;
			}
			const auto &absb = action.GetAbsorption();
			if (!absb.present || absb.scope != want) {
				continue;
			}
			int chance;
			if (absb.chance != EApply::kNone) {
				const int cap = caster->IsNpc() ? kMaxNpcResist : kMaxPcResist;
				chance = std::min(cap, WardResistStat(victim, absb.chance));
			} else {
				chance = absb.prob;
			}
			if (number(1, 100) > chance) {
				continue;
			}
			EmitWardMsgs(caster, victim, at);
			return true;
		}
	}
	return false;
}

}  // namespace

EStageResult CastDamage(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	int rand = 0, count = 1, modi = 0;

	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr)
		return EStageResult::kSuccess;

	// issue.damage-over-time: a poison-sourced <damage> tick reproduces ProcessPoisonDmg's kPoison branch
	// exactly and returns before the general damage machinery: GET_POISON-based amount dealt as kPoisonDmg,
	// no-flee, self-inflicted, credited to the poisoner (the affect's caster). Poison never had MR/area/
	// saving/wards, so those are intentionally skipped. Affect flavor (empty for kPoisoned) -> generic line.
	if (ctx.action_or_default().Contains(talents_actions::EAction::kDamage)
			&& ctx.action_or_default().GetDmg().GetSource() == talents_actions::EDamageSource::kPoison) {
		// The poison DoT (kPoisoned). Aconite/scopola/belena/datura all contribute to the same GET_POISON
		// total (aconite at a reduced rate), so this single kPoisoned tick deals ALL poison damage once.
		Damage pdmg(SpellDmg(ESpell::kPoison), CalcPoisonDamage(victim), fight::kPoisonDmg);
		pdmg.flags.set(fight::kNoFleeDmg);
		pdmg.author_uid = ctx.DamageAuthorUid();   // отравитель (для засчёта убийства), 0 если автора нет
		pdmg.aff_msg_char_ = ctx.AffectDamageMsgChar();
		pdmg.aff_msg_vict_ = ctx.AffectDamageMsgVict();
		pdmg.aff_msg_room_ = ctx.AffectDamageMsgRoom();
		const int r = pdmg.Process(ch, victim);
		return (r < 0) ? EStageResult::kBreak : EStageResult::kSuccess;
	}

	if (!pk_agro_action(ch, victim))
		return EStageResult::kSuccess;
	log("[MAG DAMAGE] %s damage %s (%d)", GET_NAME(ch), GET_NAME(victim), to_underlying(spell_id));
	// issue.attack-ward (per-manifest wards): a scoped damage-absorb (e.g. Shadow Cloak) rolls HERE,
	// per damage delivery, so each attack of a multi-hit/multi-action spell is warded independently and
	// a non-damaging cast never trips a damage-scoped ward. Whole-cast reflect/absorb ran at the entry gate.
	if (TryScopedAbsorb(ctx, talents_actions::EWardScope::kDamage)) {
		return EStageResult::kSuccess;
	}

	auto ch_start_pos = ch->GetPosition();
	auto victim_start_pos = victim->GetPosition();
	const bool tc = spell_trace::Active(ch, victim);

	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
		if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc())
			modi = modi - 50;
	}
//	if (!ch->IsNpc() && (GetRealLevel(ch) > 10))
//		modi += (GetRealLevel(ch) - 10);

	// issue.instant-death: roll the damage saving throw ONCE here and stash it in the context, so the
	// instant-death gate (target must FAIL) and the save-for-half below share a single roll instead of
	// re-rolling. Only for a modern <damage> action vs another char; kNone saving / self-cast make
	// CalcGeneralSaving return false (no save = failed = full damage, instant-death not blocked).
	if (ch != victim && ctx.action_or_default().Contains(talents_actions::EAction::kDamage)) {
		ctx.last_saving_result = CalcGeneralSaving(ch, victim,
				ctx.action_or_default().GetDmg().GetSaving(), modi);
	}

	// issue.instant-death: a <damage><instant_death prob=.../> spell can kill the target outright.
	// Gate, in order: the target must FAIL its saving throw; the prob roll must fire (default 100 =
	// always); the caster must WIN the opposed luck/morale contest. On success the normal damage is
	// irrelevant -- an NPC takes enough to die (hit + 11, past the -11 death floor); a PC is left at
	// exactly 1 HP (hit - 1). The lethal hit ignores armour/absorb/sanct/prism so the amount lands exactly.
	if (ch != victim && ctx.action_or_default().Contains(talents_actions::EAction::kDamage)) {
		const auto &id_act = ctx.action_or_default().GetDmg();
		if (id_act.HasInstantDeath()
				&& !victim->get_role(static_cast<unsigned>(EMobClass::kBoss))   // never on boss monsters
				&& !ctx.last_saving_result.value_or(false)   // target failed the (already-rolled) save
				&& number(1, 100) <= id_act.GetInstantDeathProb()
				&& abilities_roll::AgainstRivalRoll::OppositeLuckTest(ch, victim)) {
			const int lethal = victim->IsNpc() ? victim->get_hit() + 11
											   : std::max(0, victim->get_hit() - 1);
			if (tc) {
				spell_trace::Line(ch, victim, "&CInstant death %s -> %s: lethal hit %d (%s).&n\r\n",
					MUD::Spell(spell_id).GetCName(), GET_NAME(victim), lethal,
					victim->IsNpc() ? "kill" : "leave at 1 HP");
			}
			Damage id_dmg(SpellDmg(spell_id), lethal, fight::kMagicDmg);
			id_dmg.flags.set(fight::kIgnoreArmor);
			id_dmg.flags.set(fight::kIgnoreAbsorbe);
			id_dmg.flags.set(fight::kIgnoreSanct);
			id_dmg.flags.set(fight::kIgnorePrism);
			id_dmg.flags.set(fight::kNoFleeDmg);
			const int id_rand = id_dmg.Process(ch, victim);
			ctx.result.damage = id_rand;
			if (id_rand < 0) {
				ctx.is_vict_dead = true;
				return EStageResult::kBreak;
			}
			return EStageResult::kSuccess;
		}
	}

	// Multi-hit count: a damage spell with a <hits> child gets its extra-hit
	// number from CalcExtraHits. (kMagicArrows is now a data <talent_patch> boosting kMagicMissile's
	// <hits>; no spell-specific branch remains in CalcExtraHits.)
	// Absent <hits> -> count stays at the file-top default of 1 (single hit), which matches the
	// current behaviour of every non-multi-hit damage spell.
	if (ctx.action_or_default().Contains(talents_actions::EAction::kDamage)) {
		const auto &dmg = ctx.action_or_default().GetDmg();
		if (dmg.HasHits()) {
			const ESkill hits_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
			count = 1 + CalcExtraHits(ch, hits_skill,
									  dmg.GetHitsSkillDivisor(), dmg.GetHitsMax(), dmg.GetHitsProb());
			if (tc) {
				spell_trace::Line(ch, victim, "&C  hits: 1 + CalcExtraHits(skilldiv %d max %d prob %d) = %d.&n\r\n",
					dmg.GetHitsSkillDivisor(), dmg.GetHitsMax(), dmg.GetHitsProb(), count);
			}
		}
	}

	int total_dmg{0};

	try {
		total_dmg = static_cast<int>(CalcTotalSpellDmg(ch, victim, spell_id, ctx.action_or_default(),
													   ctx.CompetenceBase(), ctx.potency().noise_dev) * ctx.area_coeff);
	} catch (std::exception &e) {
		err_log("%s", e.what());
	}
	total_dmg = std::clamp(total_dmg, 0, kMaxHits);
	// issue.instant-death: a successful saving throw halves the damage (the now-live <damage saving=>
	// half-save), reusing the single roll stashed above. A failed/absent save leaves full damage.
	if (ctx.last_saving_result.value_or(false)) {
		total_dmg /= 2;
	}
	if (tc) {
		spell_trace::Line(ch, victim, "&CDamage %s -> %s: total_dmg %d (area %.2f applied), hits %d.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim), total_dmg, ctx.area_coeff, count);
	}

	const int total_hits = count;
	for (; count > 0 && rand >= 0; count--) {
		if (ch->in_room != kNowhere
			&& victim->in_room != kNowhere
			&& victim->in_room == ch->in_room
			&& ch->GetPosition() > EPosition::kStun
			&& victim->GetPosition() > EPosition::kDead) {
			const int hp_before = victim->get_hit();
			rand = LandOneDamageHit(ch, victim, spell_id, total_dmg,
									ch_start_pos, victim_start_pos, count,
									ctx.AffectDamageMsgChar(), ctx.AffectDamageMsgVict(),
									ctx.AffectDamageMsgRoom(), ctx.DamageAuthorUid());
			// accumulate the ACTUAL HP removed this hit (post-resist/save, capped
			// at the target's HP) so a chained action scales off real damage. On death the victim is
			// extracted -> count the HP it had; otherwise the HP it actually lost.
			const int removed = (rand < 0) ? std::max(0, hp_before)
									   : std::max(0, hp_before - victim->get_hit());
			ctx.damage_count += removed;
			// On a lethal hit the victim may be extracted -- pass nullptr so the trace
			// never dereferences it (caster-only line).
			if (tc) {
				spell_trace::Line(ch, (rand < 0) ? nullptr : victim, "&C  hit %d: removed %d HP%s.&n\r\n",
					total_hits - count + 1, removed, (rand < 0) ? " (lethal)" : "");
			}
		}
	}
	// Knockdown/stun outcome: report any position change (only when the victim is
	// alive -- a dead victim may already be extracted).
	if (tc && rand >= 0 && victim->GetPosition() != victim_start_pos) {
		spell_trace::Line(ch, victim, "&C  reposition: pos %d -> %d.&n\r\n",
			to_underlying(victim_start_pos), to_underlying(victim->GetPosition()));
	}
	// rand: >=0 damage dealt, -1 = victim died on this cast. Keep the raw value in
	// result.damage (callers like kSacrifice rely on the -1 sentinel); expose death
	// as a boolean + kBreak so the dispatcher stops the remaining actions.
	ctx.result.damage = rand;
	if (rand < 0) {
		ctx.is_vict_dead = true;
		return EStageResult::kBreak;
	}
	return EStageResult::kSuccess;
}

// Material-item match: an object qualifies as a component for `vnum` if it is
// of type kMagicIngredient and its get_val(1) -- the prototype-vnum field of
// magic ingredients -- equals the listed vnum (match by val(1), not by the
// object's own vnum). A single material requirement can therefore be satisfied
// by multiple concrete ingredient prototypes that carry the right val(1).
static inline bool IsMaterialFor(const ObjData *o, int vnum) {
	return o != nullptr
		&& o->get_type() == EObjType::kMagicIngredient
		&& o->get_val(1) == vnum;
}

// Search caster's equipment for a material ingredient matching `vnum` (see
// IsMaterialFor for the predicate). Returns the first match (slots scanned
// 0..kNumEquipPos-1) or nullptr.
static ObjData *FindMatInEquip(CharData *caster, int vnum) {
	for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		ObjData *o = GET_EQ(caster, i);
		if (IsMaterialFor(o, vnum)) {
			return o;
		}
	}
	return nullptr;
}

// Search caster's inventory list (->carrying) for a material ingredient.
static ObjData *FindMatInInventory(CharData *caster, int vnum) {
	for (ObjData *o = caster->carrying; o; o = o->get_next_content()) {
		if (IsMaterialFor(o, vnum)) {
			return o;
		}
	}
	return nullptr;
}

// Search the caster's room contents for a material ingredient. Returns nullptr
// if the caster is roomless.
static ObjData *FindMatInRoom(CharData *caster, int vnum) {
	if (caster->in_room == kNowhere) {
		return nullptr;
	}
	for (ObjData *o : world[caster->in_room]->contents) {
		if (IsMaterialFor(o, vnum)) {
			return o;
		}
	}
	return nullptr;
}

// Look for a material ingredient by val(1)-vnum across the locations enabled
// in `where`, in the fixed order equipment -> inventory -> room. Returns the
// first match from the highest-priority enabled location, or nullptr if none
// match. The caller is responsible for ensuring `where` carries at least one
// location bit (we treat 0 as a hard failure in ProcessMatComponents below).
static ObjData *FindMatInLocations(CharData *caster, int vnum, Bitvector where) {
	if ((where & EFind::kObjEquip) != 0) {
		if (ObjData *o = FindMatInEquip(caster, vnum)) return o;
	}
	if ((where & EFind::kObjInventory) != 0) {
		if (ObjData *o = FindMatInInventory(caster, vnum)) return o;
	}
	if ((where & EFind::kObjRoom) != 0) {
		if (ObjData *o = FindMatInRoom(caster, vnum)) return o;
	}
	return nullptr;
}

// Spend the configured charge cost of a matched material item. The `cost`
// parameter mirrors Material::cost (see talents_actions.h):
//   cost  > 0 : subtract `cost` from val[2]; emit `use`; destroy + emit
//               `exhausted` when val[2] < 1.
//   cost == 0 : focus/catalyst -- the requirement is checked elsewhere, here
//               we do nothing (no narration, no charge spent).
//   cost == -1: destroy the item in this single cast regardless of val[2];
//               emit `use` then `exhausted`.
// Unlinking dispatches on where the item actually lives (worn / carried /
// room) so destruction works in any of the three component locations.
static void ConsumeMatComponent(CharData *caster, ObjData *obj, int cost,
								const char *use, const char *exhausted) {
	if (cost == 0) {
		// Presence-only requirement: nothing to spend, nothing to narrate.
		return;
	}
	if (use) {
		act(use, false, caster, obj, nullptr, kToChar);
	}
	bool destroy = false;
	if (cost == -1) {
		// Consumed whole in one cast, regardless of remaining charges.
		destroy = true;
	} else {
		// Subtract `cost` from m_vals[2] and destroy when no charges remain.
		obj->set_val(2, GET_OBJ_VAL(obj, 2) - cost);
		if (GET_OBJ_VAL(obj, 2) < 1) {
			destroy = true;
		}
	}
	if (destroy) {
		if (exhausted) {
			act(exhausted, false, caster, obj, nullptr, kToChar);
		}
		// Unlink the item from wherever it lived before extracting it from
		// the world. ExtractObjFromWorld also unlinks defensively, but
		// matching the concrete container first matches the existing
		// inventory-only path (RemoveObjFromChar+ExtractObjFromWorld) and
		// avoids dangling equipment / room references in the meantime.
		if (obj->get_worn_by()) {
			for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
				if (GET_EQ(obj->get_worn_by(), i) == obj) {
					UnequipChar(obj->get_worn_by(), i, CharEquipFlags{});
					break;
				}
			}
		} else if (obj->get_carried_by()) {
			RemoveObjFromChar(obj);
		} else if (obj->get_in_room() != kNowhere) {
			RemoveObjFromRoom(obj);
		}
		// issue.obj-casting: deferred free (unlinked above, freed at heartbeat end) so a target
		// object that doubled as a consumed component is left flagged purged(), not dangling.
		world_objects.AddToExtractedList(obj);
	}
}

// Walk the spell's <components>/<material> entries and verify each requirement
// Returns kBreak if any material's all_of/any_of
// cannot be satisfied; kSuccess otherwise. Matched items are consumed
// (val(2)-- and possibly destroyed). Spells with no <components> block
// return kSuccess immediately -- no requirement, no work.
// Narration (the use / missing / exhausted prose) is looked up sheaf-directly
// in spell_msg.xml on the cast spell -- a missing key stays silent, matching
// the kAffImposed* convention. The kSheaf reference and the message strings
// stay live for the duration of this call (the SpellMessages container is not
// mutated mid-cast), so the c_str() pointers can safely be passed downstream.
EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id) {
	const auto &components = MUD::Spell(spell_id).GetComponents();
	if (components.empty()) {
		return EStageResult::kSuccess;
	}
	const auto &sheaf = MUD::SpellMessages()[spell_id];
	const std::string &use_str       = sheaf.GetMessage(ESpellMsg::kComponentUse);
	const std::string &missing_str   = sheaf.GetMessage(ESpellMsg::kComponentMissing);
	const std::string &exhausted_str = sheaf.GetMessage(ESpellMsg::kComponentExhausted);
	const char *use       = use_str.empty()       ? nullptr : use_str.c_str();
	const char *missing   = missing_str.empty()   ? nullptr : missing_str.c_str();
	const char *exhausted = exhausted_str.empty() ? nullptr : exhausted_str.c_str();

	for (const auto &mat : components.GetMaterials()) {
		// Mask `where` down to the three search locations honoured here. If the
		// XML named only non-search EFind values (or named nothing valid at
		// all), the cast must abort: there's no way to honour the requirement.
		const Bitvector allowed =
				mat.where & (EFind::kObjEquip | EFind::kObjInventory | EFind::kObjRoom);
		if (allowed == 0) {
			log("SYSERR: spell %s: <material where> has no eq/inv/room flag "
				"(where=%lu); cast aborted.",
				NAME_BY_ITEM<ESpell>(spell_id).c_str(),
				static_cast<unsigned long>(mat.where));
			return EStageResult::kBreak;
		}

		// A material with neither any_of nor all_of is meaningless (the parser
		// already logged a warning at load time). Skip it silently so the rest
		// of the components block still gets a chance to run.
		if (mat.any_of.empty() && mat.all_of.empty()) {
			continue;
		}

		// Items to consume once every check has passed. Built up before any
		// dec_val so a partial match doesn't half-spend the requirement.
		std::vector<ObjData *> consume;

		// all_of: every listed vnum must be present.
		for (int vnum : mat.all_of) {
			ObjData *o = FindMatInLocations(caster, vnum, allowed);
			if (!o) {
				if (missing) {
					act(missing, false, victim, nullptr, caster, kToChar);
				}
				return EStageResult::kBreak;
			}
			consume.push_back(o);
		}
		// any_of: at least one listed vnum must be present.
		if (!mat.any_of.empty()) {
			ObjData *found = nullptr;
			for (int vnum : mat.any_of) {
				if ((found = FindMatInLocations(caster, vnum, allowed)) != nullptr) {
					break;
				}
			}
			if (!found) {
				if (missing) {
					act(missing, false, victim, nullptr, caster, kToChar);
				}
				return EStageResult::kBreak;
			}
			consume.push_back(found);
		}

		// Requirement satisfied; spend mat.cost charges from each matched item.
		// The same cost applies to every item in this material -- they all
		// belong to one <material> tag, with one cost attribute.
		for (ObjData *o : consume) {
			ConsumeMatComponent(caster, o, mat.cost, use, exhausted);
		}
	}
	return EStageResult::kSuccess;
}

// Applies one affect produced by a TalentAffect apply to the victim, honoring the
// EAffFlag update flags: kAfAccumulateDuration adds to an existing
// affect's duration, kAfUpdateDuration refreshes it to the longer value, and
// kAfUpdateMod replaces the modifier only when the new magnitude is larger. The
// caller runs affect_total() afterwards.
static void ApplyTalentAffect(CharData *victim, Affect<EApply> &af, int max_stacks, Bitvector extra_battleflag = 0) {
	// issue.affect-migration: stack/update behavior comes from affects.xml by affect_type.
	const Bitvector eff_flags = affects::AffectFlagsByType(af.affect_type);
	const bool accum_dur = IS_SET(eff_flags, to_underlying(EAffFlag::kAfAccumulateDuration));
	const bool update_dur = IS_SET(eff_flags, to_underlying(EAffFlag::kAfUpdateDuration));
	const bool update_mod = IS_SET(eff_flags, to_underlying(EAffFlag::kAfUpdateMod));
	for (auto it = victim->affected.begin(); it != victim->affected.end(); ++it) {
		const auto existing = *it;
		// issue.affect-migration: stacking is keyed on affect_type (the effect identity).
		const bool same_id = existing->affect_type == af.affect_type;
		if (same_id && existing->location == af.location) {
			if (accum_dur) {
				af.duration += existing->duration;
			} else if (update_dur) {
				af.duration = std::max(af.duration, existing->duration);
			}
			if (max_stacks > 1 && existing->stacks < max_stacks) {
				// Add a stack: bump the count and accumulate the modifier.
				// kAfUpdateMod is ignored here; the sum is clamped to int to avoid overflow.
				af.stacks = existing->stacks + 1;
				const int64_t sum = static_cast<int64_t>(existing->modifier) + af.modifier;
				af.modifier = static_cast<int>(std::clamp<int64_t>(sum,
						std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
			} else if (max_stacks > 1) {
				// Already at the cap: keep the accumulated modifier and stack count, only the
				// duration (above) is refreshed.
				af.stacks = existing->stacks;
				af.modifier = existing->modifier;
			} else {
				// Non-stacking affect (max_stacks == 1): legacy kAfUpdateMod behaviour.
				af.stacks = existing->stacks;
				if (update_mod && std::abs(existing->modifier) > std::abs(af.modifier)) {
					af.modifier = existing->modifier;
				}
			}
			RemoveAffect(victim, it);
			break;
		}
	}
	affect_to_char(victim, af, extra_battleflag);
}

// Apply the spell's <affects> talent block to `victim`, returning whether the affect actually
// landed. Three exit paths return false:
//   - the prob roll missed -> silent miss (no message),
//   - a saving throw averted the affect -> "no effect" to the caster,
//   - the affect was already present and the spell doesn't accumulate/update -> "no effect".
// On success, every ordinary apply is imposed; among the random-flagged ones a single uniformly-
// chosen winner (reservoir sampling) is also imposed. The duration is computed once for all
// applies, then each apply's modifier is derived from the cast's potency roll. Every imposed
// affect records the cast's potency and debuff nature so a later dispel can be strength-gated.
static bool TryApplyAffectTalent(CharData *ch, CharData *victim, ESpell spell_id, int modi,
						 const RollResult &potency, float cast_potency,
						 const talents_actions::Action &action, double area_coeff, double competence) {
	const auto &talent = action.GetAffect();
	// prob: percent chance the <affects> block fires at all (default 100, silent miss on fail).
	// Skipping it suppresses the affect, its lag and its reposition (gated by the caller).
	// The prob<100 guard short-circuits the RNG when the spell always fires.
	const int aff_prob = talent.GetProb();
	if (aff_prob < 100 && number(1, 100) > aff_prob) {
		spell_trace::Line(ch, victim, "&CAffect %s on %s missed: prob %d%%.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim), aff_prob);
		return false;
	}
	// The affect-resist (GET_AR) debuff block is handled up front (see top of CastAffect);
	// here only the saving throw can still avert the affect (kNone saving -> CalcGeneralSaving
	// returns false, so no save is taken).
	if (ch != victim && CalcGeneralSaving(ch, victim, talent.GetSaving(), modi)) {
		// issue.spells-hotfix: a reposition spell (knockdown/stun) shows its success via the
		// "knocked down" line; on a saved cast the absence of that line already signals failure, so
		// the redundant "no effect" is suppressed for reposition affects.
		if (!talent.HasReposition()) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
		spell_trace::Line(ch, victim, "&CAffect %s on %s resisted by saving throw.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim));
		return false;
	}
	// issue.affect-migration: the re-apply gate reads update/accumulate behavior from affects.xml
	// (per affect_type); the casting spell no longer carries affect flags.
	Bitvector reapply_flags = 0;
	bool already_affected = false;
	for (const auto &apply : talent.GetApplies()) {
		if (apply.id != EAffect::kUndefined) {
			reapply_flags |= affects::AffectFlagsByType(apply.id);
			already_affected = already_affected || IsAffected(victim, apply.id);
		}
	}
	const bool can_reapply = IS_SET(reapply_flags, to_underlying(EAffFlag::kAfAccumulateDuration))
		|| IS_SET(reapply_flags, to_underlying(EAffFlag::kAfUpdateDuration));
	if (ch != victim && already_affected && !can_reapply) {
		if (ch->in_room == victim->in_room) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
		spell_trace::Line(ch, victim, "&CAffect %s on %s skipped: already affected (no reapply).&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim));
		return false;
	}
	// skill-based duration. The bonus uses the caster's potency-roll base_skill (kUndefined for
	// spells without a <potency_roll> -> flat duration). `victim` decides the unit (PC: hours ->
	// ticks; NPC: raw), preserving today's tick-unit semantics.
	// issue.drunked-migration (Gap A): an affect-action may name its own duration-scaling skill (<duration
	// skill=>); otherwise fall back to the casting spell's base_skill (kUndefined for a triggered action).
	ESkill duration_skill = talent.GetDurationSkill();
	if (duration_skill == ESkill::kUndefined) {
		duration_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
	}
	// issue.vampirism-haste: a battle-decrementing grant (battleflag="kAfBattledec") is measured in
	// combat rounds, so its duration must NOT get the PC hours->ticks conversion.
	const bool raw_rounds = (talent.GetBattleflags() & to_underlying(EAffFlag::kAfBattledec)) != 0;
	// issue.duration-scale: resistances resist VIOLENT effects only -- a beneficial cast's duration is
	// never shortened by the target's resistance. Gate ApplyResist on the per-cast violence verdict.
	int duration = CalcDuration(ch, victim, duration_skill,
					 talent.GetDurationBase(), talent.GetDurationSkillDivisor(),
					 talent.GetDurationMin(), talent.GetDurationMax(), potency.cast_skill, raw_rounds);
	if (MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) {
		duration = ApplyResist(victim, talent.GetResist(), duration);
	}
	duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, duration);
	const bool tc = spell_trace::Active(ch, victim);
	if (tc) {
		spell_trace::Line(ch, victim,
			"&CAffect %s -> %s: potency %.1f (cast %.1f x weight %.2f), duration %d "
			"(base %d /skilldiv %d clamp[%d,%d], skill_id %d).&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim),
			cast_potency * talent.GetPotencyWeight() * static_cast<float>(area_coeff),
			cast_potency, talent.GetPotencyWeight(),
			duration, talent.GetDurationBase(), talent.GetDurationSkillDivisor(),
			talent.GetDurationMin(), talent.GetDurationMax(), to_underlying(duration_skill));
	}
	// issue.affects-improve (P3b): generic over the apply type -- the affect-owned apply
	// (affects::AffectApply) or, for not-yet-migrated affects, the spell's TalentAffect::Apply.
	// The affect id is passed explicitly (AffectApply has no id; the affect IS the id).
	auto apply_one = [&](EAffect aff_id, const auto &apply) {
		Affect<EApply> taf;
		taf.affect_type = aff_id;
		taf.location = apply.location;
		taf.duration = duration;
		// Modifier formula (cap-clamped, factor-applied) lives in magic_utils so
		// CallMagicToRoom computes the room-affect modifier the exact same way.
		const int raw_mod = ComputeApplyModifier(apply, competence, potency);
		taf.modifier = static_cast<int>(raw_mod * area_coeff);
		taf.caster_id = ch->get_uid();
		// issue.character-affect-triggers: trigger charges (<affects><charges max=N>, -1 = unlimited) --
		// mirrors the room-affect impose. Spent when a persisting-affect trigger fires (SpendCharAffectCharge).
		taf.charges = talent.GetChargesMax();
		// Stored potency is the cast potency scaled by the <affects potency_weight=>
		// attribute (default 1.0 = no change). Lets
		// big-modifier spells stay dispellable by recording a deliberately weaker
		// potency than the raw roll would suggest.
		taf.potency = cast_potency * talent.GetPotencyWeight() * static_cast<float>(area_coeff);
		// apply.stack is the max stack count: re-applying up to the cap adds a stack and
		// accumulates the modifier (see ApplyTalentAffect).
		ApplyTalentAffect(victim, taf, apply.stack, talent.GetBattleflags());
		if (tc) {
			spell_trace::Line(ch, victim,
				"&C  apply %s%s: min %.1f beta %.1f C %.2f "
				"cap %d factor %d -> raw %d x area %.2f = %d (stack<=%d).&n\r\n",
				NAME_BY_ITEM<EApply>(apply.location).c_str(),
				apply.random ? " [random pick]" : "",
				apply.min, apply.beta,
				competence, apply.cap, apply.factor,
				raw_mod, area_coeff, taf.modifier, apply.stack);
		}
	};
	// issue.affects-improve (P3b): a spell that random-picks among SEVERAL affects (e.g. kAcidArrow
	// -> one of blind/poison/silence) keeps its own per-apply content + the global random pool;
	// affect-sourcing would impose each affect's (non-random) applies unconditionally. Single-id
	// random (pick one of an affect's own applies, e.g. kFailure) flows through the affect path.
	std::vector<EAffect> rnd_ids;
	for (const auto &a : talent.GetApplies()) {
		if (a.random && a.id != EAffect::kUndefined
				&& std::find(rnd_ids.begin(), rnd_ids.end(), a.id) == rnd_ids.end()) {
			rnd_ids.push_back(a.id);
		}
	}
	if (rnd_ids.size() > 1) {
		std::vector<std::function<void()>> pool;
		for (const auto &ap : talent.GetApplies()) {
			if (ap.id == EAffect::kUndefined) { continue; }
			if (ap.random) { pool.push_back([&apply_one, &ap]() { apply_one(ap.id, ap); }); }
			else { apply_one(ap.id, ap); }
		}
		if (!pool.empty()) {
			pool[static_cast<std::size_t>(number(1, static_cast<int>(pool.size())) - 1)]();
		}
		return true;
	}
	// issue.affects-improve (P3b): impose each DISTINCT affect the spell references using the
	// affect's OWN applies (affects.xml). Affects with no applies yet (kPoisoned, P3d) fall back to
	// the spell's per-apply data. Non-random applies impose immediately; random-flagged ones form
	// ONE global pool (a single uniformly-chosen winner), preserving the prior cross-apply semantics.
	std::vector<EAffect> aff_ids;
	for (const auto &a : talent.GetApplies()) {
		if (a.id != EAffect::kUndefined
				&& std::find(aff_ids.begin(), aff_ids.end(), a.id) == aff_ids.end()) {
			aff_ids.push_back(a.id);
		}
	}
	std::vector<std::function<void()>> random_pool;
	auto consider = [&](EAffect id, const auto &ap) {
		if (ap.random) {
			random_pool.push_back([&apply_one, id, &ap]() { apply_one(id, ap); });
		} else {
			apply_one(id, ap);
		}
	};
	for (const EAffect aff_id : aff_ids) {
		const auto &aff_applies = affects::AffectApplies(aff_id);
		if (!aff_applies.empty()) {
			for (const auto &ap : aff_applies) { consider(aff_id, ap); }
		} else {
			for (const auto &ap : talent.GetApplies()) {
				if (ap.id == aff_id) { consider(aff_id, ap); }
			}
		}
	}
	if (!random_pool.empty()) {
		random_pool[static_cast<std::size_t>(number(1, static_cast<int>(random_pool.size())) - 1)]();
	}
	return true;
}

// On a successfully-landed affect, emit the side effects: battle lag, forced reposition, poison
// owner tag, and the imposition messages. The lag/reposition pair is gated on the spell having an
// <affects> talent (where they live); the poison tag and messages apply to any successful cast.
// issue.affect-migration: shared affect-imposition narration. The affect (not the spell/skill) owns
// these strings; success vs FAIL is chosen by `failed` (skills pass IS_SET(af.battleflag, kAfFailed)).
// Perspectives: to the affected (kToChar; $n=affected, $N=applier), the room (same actors), and -- only
// when applier != affected -- the applier ($n=applier, $N=affected). The wielded weapon is passed as
// $o and the armed/unarmed split is handled by AffectMsgWeapon. Missing keys stay silent.
void EmitAffectImpose(CharData *affected, CharData *other, EAffect affect_type, bool failed) {
	if (affect_type == EAffect::kUndefined || !affected) {
		return;
	}
	using EAMT = affects::EAffectMsgType;
	const bool has_other = (other != nullptr && other != affected);
	const EAMT t_char = failed ? EAMT::kAffImposeFailToChar : EAMT::kAffImposedToChar;
	const EAMT t_vict = failed ? EAMT::kAffImposeFailToVict : EAMT::kAffImposedToVict;
	const EAMT t_room = failed ? EAMT::kAffImposeFailToRoom : EAMT::kAffImposedToRoom;
	ObjData *weapon = GET_EQ(affected, EEquipPos::kWield);
	if (!weapon) { weapon = GET_EQ(affected, EEquipPos::kBoths); }
	const bool armed = (weapon != nullptr);
	// $n = affected, $N = other (target/opponent). ToChar -> affected.
	const std::string &mc = affects::AffectMsgWeapon(affect_type, t_char, armed);
	if (!mc.empty()) { act(mc.c_str(), false, affected, weapon, other, kToChar); }
	// ToVict -> the external target/opponent (only when set and distinct).
	bool vict_shown = false;
	if (has_other) {
		const std::string &mv = affects::AffectMsgWeapon(affect_type, t_vict, armed);
		if (!mv.empty()) { act(mv.c_str(), false, affected, weapon, other, kToVict); vict_shown = true; }
	}
	// ToRoom -> onlookers; exclude `other` only when it already got the to-vict line (no double).
	const std::string &mr = affects::AffectMsgWeapon(affect_type, t_room, armed);
	if (!mr.empty()) {
		const int room_flag = vict_shown ? kToNotVict : kToRoom;
		act(mr.c_str(), true, affected, weapon, other, room_flag | kToArenaListen);
	}
}

static void EmitImpositionEffects(CharData *ch, CharData *victim, ESpell spell_id,
								  const RollResult &potency,
								  const talents_actions::Action &action) {
	if (action.Contains(talents_actions::EAction::kAffect)) {
		const auto &side = action.GetAffect();
		// Battle lag: <affects> with <lag> delays the victim once the affect lands. Constant-lag
		// spells use a non-positive bonus_divisor; skill-scaling ones add
		// potency.low_skill_coeff / bonus_divisor.
		const bool tc = spell_trace::Active(ch, victim);
		if (side.HasLag()) {
			SetBattleLag(victim, potency.low_skill_coeff, side.GetLagBase(), side.GetLagBonusDivisor());
			if (tc) {
				spell_trace::Line(ch, victim, "&C  lag: base %u bonus_divisor %.2f skill_coeff %.2f.&n\r\n",
					side.GetLagBase(), side.GetLagBonusDivisor(), potency.low_skill_coeff);
			}
		}
		// Forced reposition / fight-stop: e.g. kSleep knocks to kSleep, kPeaceful stops the fight
		// (pos kUndefined). Runs after the saving/affect-resist gate, so the position only changes
		// when the debuff actually lands.
		if (side.HasReposition()) {
			ForceReposition(victim, spell_id, side.GetRepositionPos(), side.GetRepositionStopFight());
			if (tc) {
				spell_trace::Line(ch, victim, "&C  reposition: pos %d stop_fight %d.&n\r\n",
					to_underlying(side.GetRepositionPos()), side.GetRepositionStopFight() ? 1 : 0);
			}
		}
	}
	// (issue.chardata-poisoner: автор отравления теперь хранится в caster_id самого аффекта,
	// который выставляется выше при наложении, отдельная запись в CharData больше не нужна.)
	// Affect imposition messages: looked up by the cast spell and emitted sheaf-directly, so a
	// spell with no such message shows nothing.
	// issue.affect-migration: imposition narration belongs to the AFFECT, not the casting spell.
	// Use the affect's sheaf for a flagged affect (sheaf-direct: silent if absent), falling back to
	// the spell's sheaf for unflagged (kUndefined) affects still keyed by spell.
	EAffect imposed_aff = EAffect::kUndefined;
	if (action.Contains(talents_actions::EAction::kAffect)) {
		for (const auto &ap : action.GetAffect().GetApplies()) {
			if (ap.id != EAffect::kUndefined) { imposed_aff = ap.id; break; }
		}
	}
	// issue.affect-migration: a flagged affect routes through the shared affect emitter (success path);
	// an unflagged (kUndefined) affect keeps the transitional spell-message fallback.
	if (imposed_aff != EAffect::kUndefined) {
		EmitAffectImpose(victim, ch, imposed_aff, false);
	} else {
		const auto &imposed = MUD::SpellMessages()[spell_id];
		const std::string &to_vict = imposed.GetMessage(ESpellMsg::kAffImposedToChar);
		const std::string &to_room = imposed.GetMessage(ESpellMsg::kAffImposedToRoom);
		if (!to_vict.empty())
			act(to_vict.c_str(), false, victim, nullptr, ch, kToChar);
		if (!to_room.empty())
			act(to_room.c_str(), true, victim, nullptr, ch, kToRoom | kToArenaListen);
	}
}

EStageResult CastAffect(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	const RollResult &potency = ctx.potency();
	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr) {
		return EStageResult::kSuccess;
	}

	// Calculate PKILL's affects. issue.spell-ally-aggression: a benign cast is never an
	// aggressive act -- gate the PK check on the per-target violence verdict. IsViolentAgainst
	// resolves ambiguous ("A") spells by caster<->target relationship; a static IsViolent()
	// would wrongly treat A as non-violent and skip the gate for genuinely hostile A-casts.
	// Without this, an ally-only buff such as kSanctuary / kPrismaticAura (violent="N") cast
	// on an ally who happens to be in combat tripped pk_agro_action -> clan-castle eviction.
	if (ch != victim && MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) {
		if (MUD::Spell(spell_id).IsFlagged(kNpcAffectPc)) {
			if (!pk_agro_action(ch, victim)) {
				return EStageResult::kSuccess;
			}
		} else if (MUD::Spell(spell_id).IsFlagged(kNpcAffectNpc) && victim->GetEnemy())	{
			if (!pk_agro_action(ch, victim->GetEnemy()))
				return EStageResult::kSuccess;
		}
	}
	// issue.attack-ward (per-manifest wards): a scoped affect-absorb rolls HERE, per affect delivery,
	// so each affect application (incl. a side-spell's) is warded independently and a ward scoped to a
	// different payload does not react. Whole-cast reflect/absorb ran at the entry gate.
	if (TryScopedAbsorb(ctx, talents_actions::EWardScope::kAffect)) {
		return EStageResult::kSuccess;
	}

	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) && ch != victim
		&& MUD::Spell(spell_id).IsViolentAgainst(ch, victim)
		&& number(1, 999) <= GET_AR(victim) * 10) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		spell_trace::Line(ch, victim, "&CAffect %s on %s blocked: AR pre-roll (AR %d).&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim), GET_AR(victim));
		return EStageResult::kSuccess;
	}

	// decrease modi for failing, increese fo success
	int modi = 0;
	if (ch != victim) {
		modi = CalcAntiSavings(ch);
		modi += CalcClassAntiSavingsMod(ch, spell_id);
	}

	if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc()) {
		modi = modi - 50;
	}


	// A violent spell can never touch an immortal target: there is nothing to build or
	// roll, so stop here. This subsumes the per-case privilege::IsImmortal(victim) guards.
	// An A spell resolves against the immortal: a mortal casting
	// kDispellMagic on an immortal outsider is aggressive -> blocked; on a grouped
	// immortal -> non-violent path, no block here.
	if (privilege::IsImmortal(victim) && MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		spell_trace::Line(ch, victim, "&CAffect %s on %s blocked: target is immortal.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim));
		return EStageResult::kSuccess;
	}
	// Affect-resist (GET_AR): a blanket block on any debuff (a violent spell with an effect),
	// a historical mechanic -- checked up front, before any saving throw or affect is built,
	// so it stops the debuff regardless of circumstances.
	if (ch != victim && MUD::Spell(spell_id).IsViolentAgainst(ch, victim) && number(1, 100) <= GET_AR(victim)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		spell_trace::Line(ch, victim, "&CAffect %s on %s blocked: affect-resist (AR %d).&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim), GET_AR(victim));
		return EStageResult::kSuccess;
	}
	// The affect's saving throw is read straight from the talent (GetAffect().GetSaving()) in the
	// talent-affect block below; the <blocking>/<required> immunity checks moved up to
	// CastToSingleTarget (action-level, gating the whole cast).
	const bool has_affect_talent = ctx.action_or_default().Contains(talents_actions::EAction::kAffect);
	// Every affect this cast lands records the cast's potency (strength) so a later dispel can be
	// gated by strength (see CastUnaffects/DispelSucceeds). CalcCastPotency lives in magic_utils so
	// CallMagicToRoom records the same scalar for its room affects. (Buff/debuff classification is
	// the affect's own affects.xml buff flag now, not a per-cast bit -- see affects::AffectBuffKind.)
	const float cast_potency = CalcCastPotency(potency);

	// A spell without an <affects> block has no affect to apply -- `success` stays true so the
	// poison/message side-effects still fire for any non-affect-talent path.
	bool success = true;
	if (has_affect_talent) {
		success = TryApplyAffectTalent(ch, victim, spell_id, modi, potency, cast_potency,
									   ctx.action_or_default(), ctx.area_coeff, ctx.CompetenceBase());
	}

	affect_total(victim);

	if (success) {
		EmitImpositionEffects(ch, victim, spell_id, potency, ctx.action_or_default());
		if (has_affect_talent) {
			// accumulate the applied affect's potency (its stored strength =
			// cast_potency * <affects potency_weight>), matching what a dispel later reads.
			ctx.affects_potency += cast_potency * ctx.action_or_default().GetAffect().GetPotencyWeight();
		}
	}
	return EStageResult::kSuccess;
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

// * These use act(), don't put the \r\n.
// Сообщения призыва/оживления вынесены в lib/cfg/spell_msg.xml:
// kSummonToRoom (по заклинанию), kSummonFail / kSummonNoCorpse и прочие
// guard-сообщения в ветви kDefault. См. MUD::SpellMessages().


// Pick the necro-mob VNUM to spawn for kAnimateDead, given the source corpse's mob level. The
// upper tier (>34) is a 50/50 between damager/breather. The caster's own (level + remort + 4)
// then caps the result: very low-level necromancers can never spawn higher-tier undead.
static MobVnum PickNecroMobForCorpse(CharData *ch, int corpse_mob_level) {
	MobVnum mob_num;
	if (corpse_mob_level <= 5) {
		mob_num = kMobSkeleton;
	} else if (corpse_mob_level <= 10) {
		mob_num = kMobZombie;
	} else if (corpse_mob_level <= 15) {
		mob_num = kMobBonedog;
	} else if (corpse_mob_level <= 20) {
		mob_num = kMobBonedragon;
	} else if (corpse_mob_level <= 25) {
		mob_num = kMobBonespirit;
	} else if (corpse_mob_level <= 34) {
		mob_num = kMobNecrotank;
	} else {
		mob_num = (number(1, 100) > 50) ? kMobNecrobreather : kMobNecrodamager;
	}
	// kMobNecrocaster disabled, cant cast
	const int cap = GetRealLevel(ch) + remort::GetRealRemort(ch) + 4;
	if (cap < 15 && mob_num > kMobZombie) {
		mob_num = kMobZombie;
	} else if (cap < 25 && mob_num > kMobBonedog) {
		mob_num = kMobBonedog;
	} else if (cap < 32 && mob_num > kMobBonedragon) {
		mob_num = kMobBonedragon;
	}
	return mob_num;
}

// Compute the per-spell summon parameters into `p`. Returns false (no further action by caller)
// when the spell short-circuits with its own diagnostic message (e.g. kAnimateDead/kResurrection
// when obj isn't a corpse, kResurrection on a corpse missing its mob VNUM, or an unsummonable
// spell_id reaching the default case).

// Rename a freshly-spawned resurrection mob as "умертвие <его_имя>" across all six grammatical
// cases. Also clears the long-description, sets neutral gender, marks it kResurrected, and
// optionally boosts dam/hit/HR by 20% for the kFuryOfDarkness feat.
static void RenameAsUndead(CharData *ch, CharData *mob) {
	ClearMinionTalents(mob);
	if (mob->IsFlagged(EMobFlag::kNoGroup)) {
		mob->UnsetFlag(EMobFlag::kNoGroup);
	}

	sprintf(buf2, "умертвие %s %s", GET_PAD(mob, 1), GET_NAME(mob));
	mob->SetCharAliases(buf2);
	sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
	mob->set_npc_name(buf2);
	mob->player_data.long_descr = "";
	sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kNom] = std::string(buf2);
	sprintf(buf2, "умертвию %s", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kDat] = std::string(buf2);
	sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kAcc] = std::string(buf2);
	sprintf(buf2, "умертвием %s", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kIns] = std::string(buf2);
	sprintf(buf2, "умертвии %s", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kPre] = std::string(buf2);
	sprintf(buf2, "умертвия %s", GET_PAD(mob, 1));
	mob->player_data.PNames[grammar::ECase::kGen] = std::string(buf2);
	mob->set_sex(EGender::kNeutral);
	mob->SetFlag(EMobFlag::kResurrected);
	mob->SetFlag(EMobFlag::kUndead);	// issue.npc-races: resurrected => undead
	if (CanUseFeat(ch, EFeat::kFuryOfDarkness)) {
		GET_DR(mob) = GET_DR(mob) + GET_DR(mob) * 0.20;
		mob->set_max_hit(mob->get_max_hit() + mob->get_max_hit() * 0.20);
		mob->set_hit(mob->get_max_hit());
		GET_HR(mob) = GET_HR(mob) + GET_HR(mob) * 0.20;
	}
}

// True if the freshly-loaded `mob` can't be summoned by `ch` (sanctuary / mob spec / world flag /
// god's shield / horse). On true, sends the appropriate kResurrect*/kSummonWarhorse message and
// extracts `mob` from the world; caller must `return kSuccess` without further action. Immortals
// bypass every guard except the horse check (which is universal).


// For the top-tier kAnimateDead spawns (kMobNecrodamager..kLastNecroMob): bump max HP by 10% per
// remort, then crank up damnodice until the mob's reformed-charm value matches the caster's
// charm-points budget (or hits the 255 cap, which means damsize is too small for this caster).
static void BoostNecroDamage(CharData *ch, CharData *mob, ESpell spell_id) {
	// add 10% mob health by remort
	mob->set_max_hit(mob->get_max_hit() * (1.0 + remort::GetRealRemort(ch) / 10.0));
	mob->set_hit(mob->get_max_hit());
	int player_charms_value = CalcCharmPoint(ch, spell_id);
	int mob_cahrms_value = GetReformedCharmiceHp(ch, mob, spell_id);
	int damnodice = 1;
	mob->mob_specials.damnodice = damnodice;
	// look for count dice to maximize damage on player_charms_value. max 255.
	while (player_charms_value > mob_cahrms_value && damnodice <= 255) {
		damnodice++;
		mob->mob_specials.damnodice = damnodice;
		mob_cahrms_value = GetReformedCharmiceHp(ch, mob, spell_id);
	}
	damnodice--;

	mob->mob_specials.damnodice = damnodice; // get prew damnodice for match with player_charms_value
	if (damnodice == 255) {
		// if damnodice == 255 mob damage not maximized. damsize too small
		SendMsgToRoom("Темные искры пробежали по земле... И исчезли...", ch->in_room, 0);
	} else {
		// mob damage maximazed.
		SendMsgToRoom("Темные искры пробежали по земле. Кажется сама СМЕРТЬ наполняет это тело силой!",
					  ch->in_room, 0);
	}
}

// Copy caster cosmetics + stats onto the kClone double: PNames in all six cases, every stat, the
// hp/ac/dr/hr/class/build, position, gender, flags.


// kAnimateDead post-spawn: mark undead + per-tier rescue grants for the kLoyalAssist /
// kHauntingSpirit feats; high-wisdom (75+) casters also gift an ice shield. The wis>=65 magic-
// glass grant is left commented out as a "if we ever want it back" hook.
static void EnhanceAnimateDead(CharData *ch, CharData *mob, MobVnum mob_num,
							   ESpell spell_id, int charm_duration) {
	// issue.unstable-hotfixes: animate dead must NOT set kResurrected -- that flag marks a corpse
	// revived by the kResurrection spell (which reuses the original mob's vnum). Animate-dead
	// summons are fresh necro mobs: undead, but not "resurrected".
	mob->SetFlag(EMobFlag::kUndead);
	if (mob_num == kMobSkeleton && CanUseFeat(ch, EFeat::kLoyalAssist)) {
		SetSkill(mob, ESkill::kRescue, 100);
	}
	if (mob_num == kMobBonespirit && CanUseFeat(ch, EFeat::kHauntingSpirit)) {
		SetSkill(mob, ESkill::kRescue, 120);
	}

	// даем всем поднятым, ну наверное не будет чернок 75+ мудры вызывать зомби в щите.
	const float eff_wis = CalcEffectiveWis(ch, spell_id);
	if (eff_wis >= 65) {
		// пока не даем, если надо включите
		//af.affect_type = to_underlying(EAffectFlag::AFF_MAGICGLASS);
		//affect_to_char(mob, af);
	}
	if (eff_wis >= 75) {
		Affect<EApply> af;
		af.duration = charm_duration * (1 + remort::GetRealRemort(ch));
		af.modifier = 0;
		af.location = EApply::kNone;
		af.affect_type = EAffect::kIceShield;
		af.battleflag = 0;
		affect_to_char(mob, af);
	}
}

// kSummonKeeper post-spawn: tie keeper level to caster, then derive a "rating" from
// light-magic + cha and project that onto hit/skills/stats/HR/AC.
// Svent TODO: не забыть перенести это в ability
// Scale a summoned-minion stat off the cast competence C (skill_coeff+stat_coeff, from kSummonKeeper's
// <potency_roll>) via the standard option-2 modifier formula -- the SAME ComputeApplyModifier the
// affect modifiers use; no new formula. dices_weight>0 folds in the spell's potency dice (the HP
// spread, replacing the old RollDices(10,10)); flat stats use beta*C only.


// Keeper stats, calibrated to reproduce the old rating-based curve at a maxed R12 keeper-summoner
// (C ~ 3.1: skill 140, cha 30) and flatten above the novice skill threshold (the accepted
// rebalance -- caps runaway high-skill keepers). C replaces the old (kLightMagic skill + cha)/2
// rating; competence_weight lives in each stat's beta. kRescue is now scaled (was flat 100 only
// via the keeper flag; this is the in-class keeper's own rescue).


// kSummonFirekeeper post-spawn: a fire-aura (or fire-shield at 30+ effective cha) charm affect,
// dr/hp/skills scaled by a 0..30 modifier derived from caster cha. Awakens on spawn.


// Spill the source corpse's contents into the caster's room (decay-checking each item) and
// extract the corpse itself. Used post-spawn whenever SummonParams::handle_corpse is set.
// А надо ли это вообще делать???
static void SpillCorpseContents(CharData *ch, ObjData *obj) {
	for (ObjData *tobj = obj->get_contains(); tobj; ) {
		ObjData *next_obj = tobj->get_next_content();
		RemoveObjFromObj(tobj);
		PlaceObjToRoom(tobj, ch->in_room);
		if (!CheckObjDecay(tobj) && tobj->get_in_room() != kNowhere) {
			act("На земле остал$U лежать $o.", false, ch, tobj, nullptr, kToRoom | kToArenaListen);
		}
		tobj = next_obj;
	}
	ExtractObjFromWorld(obj);
}

// Charm a freshly-read summoned `mob` to `ch` and place it in the caster's room: zero gold/exp/
// carry, the kCharm affect (+ kHelper & kRescue=100 when `keeper`) with a wisdom+moon-phase
// duration, kCorpse, the kSummonToRoom narration, place + add_follower. Returns the charm duration
// (callers reuse it -- the firekeeper aura and the animate-dead ice shield share it).


// kClone post-spawn (issue.summon-pipeline): make the spawned double a copy of the caster, then
// fill the rest of the clone quota -- max(1,(level+4)/5-2) total -- with extra doubles. Each extra
// is charmice-capped and finalized like the first but never re-rolls failure or cascades again
// (replacing the old need_fail=false recursion into CastSummon).


// issue.summon-pipeline: string-named post-spawn handlers (the spell-specific 20%). Signature
// (ch, spawned mob, ctx). Registered here; named by <summon handler="...">. The keeper's
// post-spawn customization is SetupKeeperStats.
static const std::map<std::string, std::function<void(CharData *, CharData *, const ActionContext &, int)>>
		kSummonHandlers = {
	{"SetupKeeperStats",
		[](CharData *ch, CharData *mob, const ActionContext &ctx, int) { handlers::SetupKeeperStats(ch, mob, ctx); }},
	{"SetupFirekeeperStats",
		[](CharData *ch, CharData *mob, const ActionContext &ctx, int dur) { handlers::SetupFirekeeperStats(ch, mob, ctx, dur); }},
	{"CloneCascade", handlers::CloneCascade},
};

// Data-driven summon skeleton (issue.summon-pipeline): the common 80% -- fail roll, charmed
// guard, charmice cap, read mob, charm (+keeper) affect, place/follow -- then the named handler
// for the spell-specific 20%. Fail is the unified base_fail - C*competence_weight (floored at
// min_fail); C = ctx.CompetenceBase() (incl. cast-chain base=). need_fail/GetEnemy/GET_CAST_SUCCESS
// from the old per-spell fail are intentionally retired here.
EStageResult CastSummonAction(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	const auto &s = ctx.action_or_default().GetSummon();
	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	int pfail = s.base_fail - static_cast<int>(ctx.CompetenceBase() * s.competence_weight);
	if (pfail < s.min_fail) { pfail = s.min_fail; }
	if (pfail > 100) { pfail = 100; }
	if (!privilege::IsImmortal(ch) && number(0, 101) < pfail) {
		const bool fail_sticks = !CanUseFeat(ch, EFeat::kFavorOfDarkness) || number(0, 3) == 0;
		if (fail_sticks) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
	}
	// A negative vnum tells ReadMobile to load a *summoned* instance: no trigger assignment, not
	// counted in mob-index total_online, EMobFlag::kCompanion ally flag set.
	// The <summon vnum> is the plain positive
	// prototype, so negate it here (issue.summons-fix).
	CharData *mob = ReadMobile(-s.mob_vnum, kVirtual);
	if (!mob) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (IsSummonTargetProtected(ch, mob, spell_id)) {
		return EStageResult::kSuccess;
	}
	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		return EStageResult::kSuccess;
	}
	const int duration = FinalizeSummonedMob(ch, mob, spell_id, s.keeper);
	if (!s.handler.empty()) {
		const auto it = kSummonHandlers.find(s.handler);
		if (it != kSummonHandlers.end()) {
			it->second(ch, mob, ctx, duration);
		} else {
			err_log("CastSummonAction: unknown summon handler '%s' for %s.",
					s.handler.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
		}
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment;
	return EStageResult::kSuccess;
}

// Corpse-based summon (kAnimateDead / kResurrection), a manual-cast handler: the mob is read from
// the source corpse and the failure chance is the legacy con/level/cast-success/remort roll (not
// the C-based <summon> fail), so it does not fit the data-driven skeleton. Reuses the shared
// FinalizeSummonedMob tail + IsSummonTargetProtected / CheckCharmices / SpillCorpseContents.
static EStageResult CorpseSummon(ActionContext &ctx, bool resurrection) {
	CharData *const ch = ctx.caster();
	ObjData *const obj = ctx.ovict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonCharmed) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (obj == nullptr || !IS_CORPSE(obj)) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoCorpse).c_str(),
			false, ch, nullptr, nullptr, kToChar);
		return EStageResult::kSuccess;
	}
	MobVnum mob_num = GET_OBJ_VAL(obj, 2);
	int pfail = 0;
	if (resurrection) {
		if (mob_num <= 0) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kResurrectBadCorpse) + "\r\n", ch);
			return EStageResult::kSuccess;
		}
		CharData *tmp_mob = mob_proto + GetMobRnum(mob_num);
		pfail = 10 + tmp_mob->get_con() * 2
			- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - remort::GetRealRemort(ch) * 5;
	} else {
		if (mob_num <= 0) {
			mob_num = kMobSkeleton;
		} else {
			CharData *tmp_mob = mob_proto + GetMobRnum(mob_num);
			pfail = 10 + tmp_mob->get_con() * 2
				- number(1, GetRealLevel(ch)) - GET_CAST_SUCCESS(ch) - remort::GetRealRemort(ch) * 5;
			mob_num = PickNecroMobForCorpse(ch, GetRealLevel(tmp_mob));
		}
	}
	if (!privilege::IsImmortal(ch) && number(0, 101) < pfail) {
		const bool fail_sticks = !CanUseFeat(ch, EFeat::kFavorOfDarkness) || number(0, 3) == 0;
		if (fail_sticks) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonFail) + "\r\n", ch);
			ExtractObjFromWorld(obj);
			return EStageResult::kSuccess;
		}
	}
	CharData *mob = ReadMobile(-mob_num, kVirtual);
	if (!mob) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kSummonNoProto) + "\r\n", ch);
		return EStageResult::kSuccess;
	}
	if (resurrection) {
		RenameAsUndead(ch, mob);
	}
	if (IsSummonTargetProtected(ch, mob, spell_id)) {
		return EStageResult::kSuccess;
	}
	if (!resurrection && mob_num >= kMobNecrodamager && mob_num <= kLastNecroMob) {
		BoostNecroDamage(ch, mob, spell_id);
	}
	if (!CheckCharmices(ch, mob, spell_id)) {
		ExtractCharFromWorld(mob, false);
		ExtractObjFromWorld(obj);
		return EStageResult::kSuccess;
	}
	const int duration = FinalizeSummonedMob(ch, mob, spell_id, false);
	if (!resurrection) {
		EnhanceAnimateDead(ch, mob, mob_num, spell_id, duration);
	}
	mob->SetFlag(EMobFlag::kNoSkillTrain);
	SpillCorpseContents(ch, obj);
	mob->char_specials.saved.alignment = ch->char_specials.saved.alignment;
	return EStageResult::kSuccess;
}

static EStageResult SpellAnimateDead(ActionContext &ctx) { return CorpseSummon(ctx, false); }
static EStageResult SpellResurrection(ActionContext &ctx) { return CorpseSummon(ctx, true); }


// Helpers driving the per-category CastToPoints refactor.
namespace {

// One row of the four-category dispatch table. The Amount lives in TalentPoints
// and gates whether this category fires at all (skip if !present, no calc_amount
// invocation -- the eager-compute waste).
struct PointsCategory {
	points_intensity::ECategory cat;
	ESpellMsg msg_key;
	const talents_actions::Points::Amount &amount;
};

// Short category label for the tester trace.
const char *PointsCatName(points_intensity::ECategory cat) {
	using points_intensity::ECategory;
	switch (cat) {
		case ECategory::kHeal: return "heal";
		case ECategory::kMoves: return "moves";
		case ECategory::kThirst: return "thirst";
		case ECategory::kFull: return "full";
		default: return "?";
	}
}

// Lambda factored out for clarity: turns a Points::Amount into a signed delta
// using the shared (dice, competencies, bonus_mod) trio captured by reference.
// Heal carries an npc_coeff multiplier (legacy default 1.0 ~ +100% for mobs);
// non-heal categories default to 0 (no NPC boost).
auto MakeAmountCalculator(const CharData *ch, double competencies, double bonus_mod, double noise_z) {  // issue.potency-noise: dice dropped
	return [ch, competencies, bonus_mod, noise_z](const talents_actions::Points::Amount &a) -> int {
		// Option-2 subquadratic: dice scaled multiplicatively by
		// skill/stat (alpha) plus an additive term (beta). alpha=0 -> old Formula A.
		// issue.random-noise-rework: sigma>0 -> multiplicative truncated-normal spread (heal/moves),
		// mean = min + beta*competence (beta = per-competence scale k), CV ~ sigma. Else additive.
		// issue.potency-noise: one formula -- min + beta*C*(1 + weight*d). noise_z carries the cast's
		// realized deviation d; weight=0 -> deterministic.
		int v = CalcNoisyAmount(a.min, a.beta * competencies, a.weight, /*cap*/ 0, noise_z);
		v += static_cast<int>(v * bonus_mod);
		if (ch->IsNpc()) {
			v += static_cast<int>(v * a.npc_coeff);
		}
		return v;
	};
}

// Per-category percent formula for the {intensity} placeholder:
// - heal/moves use amount-relative percent (positive on improve, negative on degrade).
// - thirst/full IMPROVE uses "fraction of current condition relieved" (positive 0..100).
// - thirst/full DEGRADE uses signed percent of the AFTER-state on the 0..48 scale, so a
//   thirstier/hungrier outcome lands a more-negative tier in the unified Resolve walk.
int ComputePointsPercent(points_intensity::ECategory cat, int amount, CharData *victim) {
	using points_intensity::ECategory;
	switch (cat) {
		case ECategory::kHeal: {
			const int max_hp = victim->get_real_max_hit();
			return (max_hp > 0) ? (amount * 100) / max_hp : 0;
		}
		case ECategory::kMoves: {
			const int max_mv = victim->get_real_max_move();
			return (max_mv > 0) ? (amount * 100) / max_mv : 0;
		}
		case ECategory::kThirst:
		case ECategory::kFull: {
			const unsigned cond_idx = (cat == ECategory::kThirst) ? condition::kThirst : condition::kFull;
			if (amount >= 0) {
				// Improve: fraction of current discomfort relieved.
				const int current = GET_COND(victim, cond_idx);
				const int removed = (current > 0) ? std::min(amount, current) : 0;
				return (current > 0) ? (removed * 100) / current : 0;
			}
			// Degrade: signed percent of the after-state on the kMaxCondition scale.
			// gain_condition clamps to [0, kMaxCondition] so we predict the same here.
			const int before = GET_COND(victim, cond_idx);
			const int raw_after = before + (-amount);   // amount<0 means XML "make worse"
			const int after = std::clamp(raw_after, 0, kMaxCondition);
			return -(after * 100) / kMaxCondition;
		}
		default:
			return 0;
	}
}

// Resolve {intensity} and act() the message. Silent when the sheaf has nothing
// for this category, or when an {intensity} placeholder resolves to empty
// (would otherwise emit "Вы почувствовали себя ." with a dangling period).
// Templated on Sheaf to avoid spelling out msg_container::MsgSheaf<ESpell,ESpellMsg>;
// MUD::SpellMessages()[id] always returns the right type.
template<typename Sheaf>
void EmitPointsMessage(CharData *victim, const Sheaf &sheaf,
					   ESpellMsg key, points_intensity::ECategory cat, int percent) {
	if (!sheaf.HasMessage(key)) return;
	std::string msg = sheaf.GetMessage(key);
	const auto pos = msg.find("{intensity}");
	if (pos != std::string::npos) {
		const std::string &grade = MUD::PointsIntensity().Resolve(cat, percent);
		if (grade.empty()) return;
		msg.replace(pos, std::strlen("{intensity}"), grade);
	}
	// $h / $r / $g resolve against victim's gender; route through act(). The
	// vict slot is `victim` itself so the message stays kToChar-only --
	// designers can add a separate kToRoom variant later if they want bystanders
	// to witness the effect.
	act(msg.c_str(), false, victim, nullptr, victim, kToChar);
}

// Apply heal: positive only (the <degrade> heal table is intentionally empty;
// a negative <heal> amount silently no-ops -- damage paths go through CastDamage).
// extra_percent: overheal cap above max_hp, in percent (0 = strict cap at max,
// 33 = up to 133% of max, etc.). Lacerations halves the heal only when no
// overheal is configured (extra_percent == 0); designer-flagged overheal
// spells deliberately ignore the wound penalty.
void ApplyHeal(CharData *victim, int hit, int extra_percent) {
	if (hit <= 0) return;
	if (victim->get_hit() >= kMaxHits) return;
	const int max_hp = victim->get_real_max_hit();
	if (extra_percent == 0) {
		if (victim->get_hit() >= max_hp) return;
		const int amount = AFF_FLAGGED(victim, EAffect::kLacerations) ? hit / 2 : hit;
		victim->set_hit(std::min(victim->get_hit() + amount, max_hp));
		return;
	}
	if (max_hp <= 0) {
		// Pathological char (max_hit <= 0): still let an overheal push to 1.
		victim->set_hit(std::max(victim->get_hit(), std::min(victim->get_hit() + hit, 1)));
		return;
	}
	const int cap = max_hp + max_hp * extra_percent / 100;
	victim->set_hit(std::clamp(victim->get_hit() + hit, victim->get_hit(), cap));
}

}  // namespace

// issue.character-affect-triggers: spend one trigger charge on a CHARACTER affect after its trigger
// fired (the char-affect analog of the room's ConsumeAffectCharge). Decrements every instance of the
// affect type (a multi-apply affect has one per apply; they stay in sync since impose gives each the
// same charges_max); when any hits 0 the WHOLE affect is consumed via RemoveAffectFromChar. charges == -1
// (the default) is unlimited -> no-op. Caller must NOT be mid-iteration over `bearer->affected`.
void SpendCharAffectCharge(CharData *bearer, EAffect affect_type) {
	if (!bearer) {
		return;
	}
	bool depleted = false;
	for (const auto &aff : bearer->affected) {
		if (aff && aff->affect_type == affect_type && aff->charges != -1 && --aff->charges <= 0) {
			depleted = true;
		}
	}
	if (depleted) {
		// issue.character-affect-triggers: count expired == timer expired -- fire the affect's kExpired
		// trigger before it is consumed (so a charge-limited affect gets its "end" reaction, matching the
		// timer-expiry path). No-op if it carries no kExpired action.
		RunCharAffectTrigger(bearer, affect_type, talents_actions::EActionTrigger::kExpired);
		if (bearer->purged()) {
			return;
		}
		RemoveAffectFromChar(bearer, affect_type);
	}
}

EStageResult CastToPoints(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	if (victim == nullptr) {
		log("MAG_POINTS: Ошибка! Не указана цель, spell_id: %d!\r\n", to_underlying(spell_id));
		return EStageResult::kSuccess;
	}
	// Fully data-driven: every category (heal / moves /
	// thirst / full) lives in the spell's <points> block. Spells without one
	// are misconfigured -- log and skip rather than crash.
	if (!ctx.action_or_default().Contains(talents_actions::EAction::kPoints)) {
		mudlog(fmt::format("SYSERR: spell {} ({}) has no <points> block, CastToPoints skipped",
				to_underlying(spell_id), MUD::Spell(spell_id).GetCName()),
			CMP, kLvlImmortal, SYSLOG, true);
		return EStageResult::kSuccess;
	}
	const auto &points = ctx.action_or_default().GetPoints();

	// Single prob roll for the whole action: a failed roll restores nothing.
	const int prob = points.GetProb();
	if (prob < 100 && number(1, 100) > prob) {
		return EStageResult::kSuccess;
	}

	// Shared roll: dice + competencies are rolled once per cast (not per
	// category), so heal and moves restored on the same cast scale together
	// with the same skill check.
	const double competencies = ctx.CompetenceBase();  // base override
	const double bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
	auto calc_amount = MakeAmountCalculator(ch, competencies, bonus_mod, ctx.potency().noise_dev);
	const bool tc = spell_trace::Active(ch, victim);
	if (tc) {
		spell_trace::Line(ch, victim, "&CPoints %s -> %s: C %.2f bonus_mod %.2f area %.2f.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim), competencies, bonus_mod, ctx.area_coeff);
	}

	// Per-category dispatch: only present categories run
	// through calc_amount and reach the apply/emit pass; spells with a single
	// <heal> tag won't touch the moves/thirst/full math at all.
	const PointsCategory categories[] = {
		{points_intensity::ECategory::kHeal,   ESpellMsg::kHealToVict,   points.GetHeal()},
		{points_intensity::ECategory::kMoves,  ESpellMsg::kMovesToVict,  points.GetMoves()},
		{points_intensity::ECategory::kThirst, ESpellMsg::kThirstToVict, points.GetThirst()},
		{points_intensity::ECategory::kFull,   ESpellMsg::kFullToVict,   points.GetFull()},
	};

	int amounts[std::size(categories)] = {0, 0, 0, 0};
	bool any_amount = false;
	for (size_t i = 0; i < std::size(categories); ++i) {
		const auto &c = categories[i];
		if (!c.amount.present) continue;
		int amt = static_cast<int>(calc_amount(c.amount) * ctx.area_coeff);
		// Legacy spell-modifier hook only ever applied to heal; keep that
		// scoped to the heal slot so /gear effects don't suddenly scale moves.
		if (c.cat == points_intensity::ECategory::kHeal) {
			amt = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, amt);
		}
		amounts[i] = amt;
		if (amt != 0) any_amount = true;
		if (tc) {
			spell_trace::Line(ch, victim, "&C  %s: min %.1f beta %.1f npc_coeff %.2f -> %d.&n\r\n",
				PointsCatName(c.cat), c.amount.min,
				c.amount.beta, c.amount.npc_coeff, amt);
		}
	}

	// Aggro consequence: buffing (or draining) your fighting buddy is still
	// PK-relevant. Single check on the aggregate `any_amount`, BEFORE applying
	// effects so a refused action zeroes nothing.
	if (any_amount && victim->GetEnemy() && ch != victim) {
		if (!pk_agro_action(ch, victim->GetEnemy())) {
			return EStageResult::kSuccess;
		}
	}

	// Apply + narrate per category (one message per
	// non-zero amount, emitted in heal/moves/thirst/full order). The percent
	// formula reads the BEFORE-state for thirst/full degrade, so the message
	// for each category is emitted right after that category is applied to
	// keep its "after-state" math accurate when later categories also fire.
	const auto &points_sheaf = MUD::SpellMessages()[spell_id];
	for (size_t i = 0; i < std::size(categories); ++i) {
		const int amt = amounts[i];
		if (amt == 0) continue;
		const auto &c = categories[i];
		// Compute percent for narration BEFORE we mutate the victim's state, so
		// the thirst/full degrade formula sees the pre-spell condition and
		// projects the after-state itself (ComputePointsPercent does the same
		// clamp gain_condition will apply).
		const int percent = ComputePointsPercent(c.cat, amt, victim);
		if (tc) {
			spell_trace::Line(ch, victim, "&C  %s applied: amount %d, intensity %d%%.&n\r\n",
				PointsCatName(c.cat), amt, percent);
		}
		// Apply the actual effect.
		switch (c.cat) {
			case points_intensity::ECategory::kHeal:
				ApplyHeal(victim, amt, points.GetExtraPercent());
				break;
			case points_intensity::ECategory::kMoves:
				// Positive: restore (clamped at max). Negative: drain (clamped at 0).
				victim->set_move(std::clamp(victim->get_move() + amt, 0, victim->get_real_max_move()));
				break;
			case points_intensity::ECategory::kThirst:
				// XML positive = relieve thirst -> negate for gain_condition
				// (engine field is inverted; gain_condition clamps to [0, kMaxCondition]).
				gain_condition(victim, condition::kThirst, -amt);
				break;
			case points_intensity::ECategory::kFull:
				gain_condition(victim, condition::kFull, -amt);
				break;
			default:
				break;
		}
		EmitPointsMessage(victim, points_sheaf, c.msg_key, c.cat, percent);
	}

	// accumulate the (computed) points restored across all categories.
	for (size_t i = 0; i < std::size(categories); ++i) {
		ctx.points_count += amounts[i];
	}
	update_pos(victim);

	// issue.character-affect-triggers: kPoints -- the RESTORED char's affects react to being healed by an
	// EXTERNAL caster (natural regen never reaches this stage). The reaction is run SELF-INFLICTED on the
	// caster/healer (ch), so e.g. a vampirism bearer's curse harms whoever heals them with NO PvP against
	// the bearer -- same model as kDispell. Fired once per applied category (so a <trigger category=>
	// action reacts only to its category) and deduped by affect type; a lethal self-hit stops the loop.
	if (ch != victim && !ch->purged() && !victim->purged()) {
		// Snapshot the healed char's unique affect types first: spending a charge may remove an affect,
		// which would invalidate a live iteration over victim->affected.
		std::vector<EAffect> vtypes;
		for (const auto &aff : victim->affected) {
			if (aff && std::find(vtypes.begin(), vtypes.end(), aff->affect_type) == vtypes.end()) {
				vtypes.push_back(aff->affect_type);
			}
		}
		for (size_t i = 0; i < std::size(categories); ++i) {
			if (amounts[i] == 0) {
				continue;
			}
			const int cat = static_cast<int>(categories[i].cat);
			for (const EAffect t : vtypes) {
				if (!AFF_FLAGGED(victim, t)) {
					continue;   // already consumed (charge spent by an earlier category this cast)
				}
				if (RunCharAffectTrigger(ch, t, talents_actions::EActionTrigger::kPoints, ch, amounts[i], cat)) {
					SpendCharAffectCharge(victim, t);   // charge lives on the RESTORED char's affect
				}
				if (ch->purged() || victim->purged()) {
					return EStageResult::kSuccess;
				}
			}
		}
	}
	return EStageResult::kSuccess;
}

namespace {
// Helpers for the data-driven CastUnaffects.

// True if `affect` carries at least one of `flags`. The <unaffect affect_flags=> set lists the
// EAffFlag bits (kAfCurable / kAfDispellable) an affect must have to be eligible for removal by
// that unaffect; this is the single source of truth for "can this be removed" (it replaced the old
// CheckNodispel blacklist). An affect with no matching flag -- charm/quest effects, or anything
// applied outside <affects> in code -- is irremovable.
bool AffectMatchesFlags(const Affect<EApply>::shared_ptr &affect, Bitvector flags) {
	return affect && IS_SET(affect->battleflag, flags);
}

// True if the victim carries a removable affect of the given spell type (one matching `flags`).
bool HasDispellableAffect(CharData *victim, EAffect affect_type, Bitvector flags) {
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type && AffectMatchesFlags(aff, flags)) {
			return true;
		}
	}
	return false;
}

// Evaluate a <blocking>/<breaking> condition set by affect identity:
// true if any any_of affect is present, or every all_of affect is present.
bool UnaffectConditionMet(CharData *victim, const talents_actions::TalentUnaffect::Set &set) {
	for (const auto affect_type : set.any_of.chars) {
		if (IsAffected(victim, affect_type)) {
			return true;
		}
	}
	if (!set.all_of.chars.empty()) {
		bool all = true;
		for (const auto affect_type : set.all_of.chars) {
			if (!IsAffected(victim, affect_type)) {
				all = false;
				break;
			}
		}
		if (all) {
			return true;
		}
	}
	return false;
}

// One affect queued for dispel, tagged with its source block's breaking_by_failure flag: if the
// dispel of a candidate with break_on_fail set is resisted, the whole cast chain breaks.
struct RemovalCandidate {
	bool break_on_fail{false};
	EAffect affect_type{EAffect::kUndefined};                          // char target: the affect to remove
	room_spells::ERoomAffect room_affect_type{room_spells::ERoomAffect::kUndefined};  // room target: the affect to remove
};

// Build the list of affects to dispel for a <remove>/<remove_anyway> set:
//   - any_of (explicit list)   -> the first listed affect that is present and matches `flags`
//   - all_of (explicit list)   -> every listed affect that is present and matches `flags`
//   - wildcard_any (any_of="*")-> ONE eligible affect picked uniformly at random
//   - wildcard_all (all_of="*")-> EVERY eligible affect on the victim
// Each candidate carries the set's breaking_by_failure. The wildcards replaced the dedicated
// kDispellMagic code path and enable generic "strip-by-flag" dispels (e.g.
// future sphere-specific dispels added by tagging affects with kAfXSphere flags).
void CollectRemovals(CharData *victim, const talents_actions::TalentUnaffect::Set &set,
					 std::vector<RemovalCandidate> &out, Bitvector flags, bool debuff_only = false) {
	// issue.affect-migration: candidates are keyed by affect_type (no spell read at all -- the dispel
	// downstream and the PK classification both work off the affect's own identity/flags).
	// issue.new-unaffect-spells: debuff_only skips affects that are clearly buffs (EBuff::kYes) so a
	// friendly wildcard cleanse ("unweave") removes debuffs/ambiguous without stripping real buffs.
	const auto eligible = [&](const Affect<EApply>::shared_ptr &aff) {
		return AffectMatchesFlags(aff, flags)
			&& !(debuff_only && affects::AffectBuffKind(aff->affect_type) == affects::EBuff::kYes);
	};
	if (set.wildcard_any) {
		// Reservoir sample one eligible affect uniformly.
		EAffect pick_at = EAffect::kUndefined;
		int seen = 0;
		for (const auto &aff : victim->affected) {
			if (eligible(aff) && number(1, ++seen) == 1) {
				pick_at = aff->affect_type;
			}
		}
		if (pick_at != EAffect::kUndefined) {
			out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = pick_at});
		}
	} else {
		for (const auto affect_type : set.any_of.chars) {
			if (HasDispellableAffect(victim, affect_type, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = affect_type});
				break;
			}
		}
	}
	if (set.wildcard_all) {
		for (const auto &aff : victim->affected) {
			if (eligible(aff)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = aff->affect_type});
			}
		}
	} else {
		for (const auto affect_type : set.all_of.chars) {
			if (HasDispellableAffect(victim, affect_type, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = affect_type});
			}
		}
	}
}

// A successful dispel of a stacked affect peels one stack instead of removing it
// for every affect of `spell` with stacks > 1, reduce the stack count by 1
// and the accumulated modifier proportionally (~modifier/stacks), re-applying so the character's
// stats update. If no affect of the spell has more than one stack, remove it outright.
void ReduceStackOrRemove(CharData *victim, EAffect affect_type) {
	bool any_multi = false;
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type && aff->stacks > 1) {
			any_multi = true;
			break;
		}
	}
	if (!any_multi) {
		RemoveAffectFromCharAndRecalculate(victim, affect_type);
		return;
	}
	std::vector<Affect<EApply>> rebuilt;
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type) {
			Affect<EApply> peeled = *aff;
			if (peeled.stacks > 1) {
				peeled.modifier = static_cast<int>(
						static_cast<int64_t>(peeled.modifier) * (peeled.stacks - 1) / peeled.stacks);
				peeled.stacks -= 1;
			}
			rebuilt.push_back(peeled);
		}
	}
	RemoveAffectFromChar(victim, affect_type);    // strip all of the affect's instances (deltas undone)
	for (auto &peeled : rebuilt) {
		affect_to_char(victim, peeled);           // re-add with the reduced modifier (stats recalced)
	}
}

// Remove one affect (or peel a stack of it) and emit its dispel narration. Lookup is keyed by
// the REMOVED affect's spell with kDefault fallback, so the per-affect sheaf's bespoke flavor
// fires when authored (kBlindness, kPoison, kCurse, ...) and the kDefault generic
// (kAffDispelledTo{Char,Room}) fires for every other affect -- no more silent stripping of
// common buffs.
void RemoveAffectAndAnnounce(CharData *ch, CharData *victim, EAffect affect_type) {
	// issue.character-affect-triggers: kDispell -- fire the affect's own kDispell <actions> BEFORE it is
	// stripped, run with the DISPELLER (ch) as BOTH the caster and the actor. This makes the retaliation
	// SELF-INFLICTED by the dispeller (their choice to dispel, their consequence): a <damage>/<affects>
	// on kTarFightSelf lands on the dispeller as self-damage -- no pk-aggro, no revenge-rights, and the
	// bearer (who may not have wanted a fight) is never dragged into PvP. Immortal immunity still applies
	// to the self-damage normally. This is the announced-dispel path (dispel magic / unaffect) only.
	RunCharAffectTrigger(ch, affect_type, talents_actions::EActionTrigger::kDispell, ch);
	// issue.affect-migration: a char affect is identified by affect_type; dispel narration belongs to
	// the AFFECT (its sheaf, with the kDefault generic fallback).
	ReduceStackOrRemove(victim, affect_type);
	const std::string &to_vict = affects::AffectMsg(affect_type, affects::EAffectMsgType::kAffDispelledToChar);
	if (!to_vict.empty()) {
		act(to_vict.c_str(), false, victim, nullptr, ch, kToChar);
	}
	const std::string &to_room = affects::AffectMsg(affect_type, affects::EAffectMsgType::kAffDispelledToRoom);
	if (!to_room.empty()) {
		act(to_room.c_str(), true, victim, nullptr, ch, kToRoom | kToArenaListen);
	}
}

// issue: potency-gated dispel. True if `dispel_spell` should remove `affect_spell` from victim.
// Three cases drive whether a strength check is even rolled:
//   1. a violent dispel -> always check (any affect);
//   2. a non-violent dispel of a debuff -> check;
//   3. a non-violent dispel of a buff -> no check, always removed.
// The check itself: a flat 5% chance to remove regardless of strength, otherwise the dispel's
// potency -- competence (skill+stat) * a truncated-normal noise factor * potency_weight -- must exceed the
// affect's recorded potency (the strength of the cast that imposed it).
// issue.debuff-decay: ceiling on an affect's potency. A negative <unaffect decay> RAISES an affect's
// strength on a failed removal; the cap stops repeated failures from growing it without bound (and
// guards against overflow once the float feeds integer modifiers). Generous vs normal cast potencies
// (tens) yet finite and well within int range.
constexpr float kMaxAffectPotency = 30000.0f;

// issue.random-noise-rework: weight turning a competence gap (dispeller - affect, in skill+stat
// points) into dispel win-probability points in the d100 contest below. Larger -> skill matters
// more; with the rebalanced competence range (~2..12), 4 gives roughly a +/-40-point swing.
constexpr double kDispelSkillWeight = 4.0;

// issue.debuff-decay: on a FAILED removal, shift the surviving affect's potency by `decay` percent of
// the dispeller's competence contribution (kDispelSkillWeight * competence) -- positive decay weakens
// the affect, negative strengthens it. Floored at 0, capped at kMaxAffectPotency.
void ApplyDispelDecay(float &affect_potency, float dispel_strength, int decay) {
	if (decay == 0) {
		return;
	}
	const float delta = dispel_strength * static_cast<float>(decay) / 100.0f;
	affect_potency = std::clamp(affect_potency - delta, 0.0f, kMaxAffectPotency);
}

bool DispelSucceeds(CharData *ch, CharData *victim, ESpell dispel_spell, EAffect affect_type,
					int dispel_bonus, double competence, double area_coeff = 1.0, int decay = 0) {
	float affect_potency = 0.0f;
	float *matched_potency = nullptr;
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type) {
			affect_potency = aff->potency;
			matched_potency = &aff->potency;
			break;
		}
	}
	// Tester / immortal debug line: one per affect-vs-dispel pair. Reason codes:
	//   buff -- a non-violent dispel of a buff auto-passes (no contest rolled).
	//   roll -- the d100 contest was rolled against the threshold.
	auto emit_debug = [&](int threshold, const char *kind, bool ok) {
		spell_trace::Line(ch, nullptr,
				 "Unaffect: %s [C: %.1f]. Target: %s [p: %.1f]. threshold %d%%. %s (%s).\r\n",
				 MUD::Spell(dispel_spell).GetCName(), competence,
				 affects::AffectMsg(affect_type, affects::EAffectMsgType::kShortDesc).c_str(), affect_potency,
				 threshold, ok ? "Success" : "Fail", kind);
	};
	// issue.random-noise-rework: a d100 skill contest. The dispeller's competence advantage over the
	// affect (scaled by kDispelSkillWeight) plus the per-spell dispel_bonus and the affect's own
	// dispel_mod set the win threshold; the clamp gives a symmetric 5% upset floor and 5% save ceiling.
	const double raw = kDispelSkillWeight * (area_coeff * competence - affect_potency)
			+ dispel_bonus + affects::AffectDispelMod(affect_type);
	const int threshold = std::clamp(static_cast<int>(std::lround(raw)), 5, 95);
	// A non-violent (per-target) dispel of a buff needs no contest (ally cleansing); an enemy-hand
	// dispel of a buff, or any dispel of a debuff, rolls.
	if (!MUD::Spell(dispel_spell).IsViolentAgainst(ch, victim)
			&& affects::AffectBuffKind(affect_type) == affects::EBuff::kYes) {
		emit_debug(threshold, "buff", true);
		return true;
	}
	const bool ok = number(1, 100) <= threshold;
	emit_debug(threshold, "roll", ok);
	if (!ok && matched_potency) {
		ApplyDispelDecay(*matched_potency, static_cast<float>(kDispelSkillWeight * competence), decay);
	}
	return ok;
}

// === Room-target overloads ===
// Mirror the char helpers above for a RoomData target. Room affects share the Affect template
// (Affect<ERoomApply>) so the potency/debuff/battleflag fields read identically; only the storage
// (room->affected vs victim->affected) and the apply/remove APIs differ. PK gating is intentionally
// NOT replicated here -- concurrency rules for room dispel are deferred (no clear policy yet).

bool AffectMatchesFlags(const Affect<room_spells::ERoomApply>::shared_ptr &affect, Bitvector flags) {
	return affect && IS_SET(affect->battleflag, flags);
}

bool HasDispellableAffect(RoomData *room, room_spells::ERoomAffect affect_type, Bitvector flags) {
	for (const auto &aff : room->affected) {
		if (aff && aff->affect_type == affect_type && AffectMatchesFlags(aff, flags)) {
			return true;
		}
	}
	return false;
}

bool UnaffectConditionMet(RoomData *room, const talents_actions::TalentUnaffect::Set &set) {
	auto room_has = [room](room_spells::ERoomAffect ra) {
		for (const auto &aff : room->affected) {
			if (aff && aff->affect_type == ra) {
				return true;
			}
		}
		return false;
	};
	for (const auto ra : set.any_of.rooms) {
		if (room_has(ra)) {
			return true;
		}
	}
	if (!set.all_of.rooms.empty()) {
		bool all = true;
		for (const auto ra : set.all_of.rooms) {
			if (!room_has(ra)) {
				all = false;
				break;
			}
		}
		if (all) {
			return true;
		}
	}
	return false;
}

void CollectRemovals(RoomData *room, const talents_actions::TalentUnaffect::Set &set,
					 std::vector<RemovalCandidate> &out, Bitvector flags, bool /*debuff_only*/ = false) {
	// issue.new-unaffect-spells: debuff_only is a no-op for room affects (no buff/debuff notion);
	// the parameter exists only so RunCastUnaffects can call this from the shared template.
	if (set.wildcard_any) {
		room_spells::ERoomAffect pick = room_spells::ERoomAffect::kUndefined;
		int seen = 0;
		for (const auto &aff : room->affected) {
			if (AffectMatchesFlags(aff, flags) && number(1, ++seen) == 1) {
				pick = aff->affect_type;
			}
		}
		if (pick != room_spells::ERoomAffect::kUndefined) {
			out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = pick});
		}
	} else {
		for (const auto ra : set.any_of.rooms) {
			if (HasDispellableAffect(room, ra, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = ra});
				break;
			}
		}
	}
	if (set.wildcard_all) {
		for (const auto &aff : room->affected) {
			if (AffectMatchesFlags(aff, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = aff->affect_type});
			}
		}
	} else {
		for (const auto ra : set.all_of.rooms) {
			if (HasDispellableAffect(room, ra, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = ra});
			}
		}
	}
}

// Strip every affect of `removed` from the room (room affects don't stack the way char affects do,
// so peeling has no meaning here) and emit the dispel narration. issue.affect-migration: keyed by
// the room affect's identity (ERoomAffect); narration comes from the room-affect message system
// (RoomAffectMsgRaw), the affect's own kAffDispelledTo{Char,Room} sheaf.
void RemoveAffectAndAnnounce(CharData *ch, RoomData *room, room_spells::ERoomAffect removed) {
	// issue.character-affect-triggers: kDispell -- fire the room affect's kDispell actions BEFORE the
	// strip, run with the DISPELLER (ch) as the caster so the retaliation is SELF-INFLICTED (a
	// kTarFightSelf effect lands on the dispeller as self-damage -- no PvP, same model as char affects).
	float dispel_potency = 0.0f;
	for (const auto &aff : room->affected) {
		if (aff && aff->affect_type == removed) { dispel_potency = aff->potency; break; }
	}
	room_spells::RunRoomAffectTrigger(room, ch, removed, talents_actions::EActionTrigger::kDispell, dispel_potency);
	// Canonical "erase while iterating" over std::list -- list::erase invalidates only the erased
	// iterator and returns the next one. Bypasses room_spells::RoomRemoveAffect (a thin
	// empty-list-guarded wrapper) because the guard is redundant inside a live iteration.
	for (auto it = room->affected.begin(); it != room->affected.end(); ) {
		if (*it && (*it)->affect_type == removed) {
			it = room->affected.erase(it);
		} else {
			++it;
		}
	}
	// Sheaf-direct (no kDefault fallback): a room dispel shows only the affect's OWN
	// dispel message, never the char-centric kDefault (issue.dispellbug).
	const auto &to_char = room_spells::RoomAffectMsgRaw(removed, room_spells::ERoomAffectMsgType::kAffDispelledToChar);
	if (!to_char.empty()) {
		act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar);
	}
	const auto &to_room = room_spells::RoomAffectMsgRaw(removed, room_spells::ERoomAffectMsgType::kAffDispelledToRoom);
	if (!to_room.empty()) {
		act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
	}
}

bool DispelSucceeds(CharData *ch, RoomData *room, ESpell dispel_spell, room_spells::ERoomAffect affect_type,
					int dispel_bonus, double competence, double area_coeff = 1.0, int decay = 0) {
	float affect_potency = 0.0f;
	long author_uid = 0;
	float *matched_potency = nullptr;
	for (const auto &aff : room->affected) {
		if (aff && aff->affect_type == affect_type) {
			affect_potency = aff->potency;
			author_uid = aff->caster_id;
			matched_potency = &aff->potency;
			break;
		}
	}
	auto emit_debug = [&](int threshold, const char *kind, bool ok) {
		spell_trace::Line(ch, nullptr,
				 "Unaffect: %s [C: %.1f]. Target room: %s [p: %.1f]. threshold %d%%. %s (%s).\r\n",
				 MUD::Spell(dispel_spell).GetCName(), competence,
				 NAME_BY_ITEM<room_spells::ERoomAffect>(affect_type).c_str(), affect_potency,
				 threshold, ok ? "Success" : "Fail", kind);
	};
	// issue.dispellbug: author/ally-aware room dispel. The affect's author or a live
	// ally dispels for free; anyone else must win a strength contest, and a player vs a
	// live author commits an aggressive PK act. (Dispellability is filtered upstream by
	// the <unaffect affect_flags=> mask, so kAfDispellable is already enforced.)
	// issue.random-noise-rework: d100 skill contest (see the char overload). kDispelSkillWeight scales
	// the competence gap, dispel_bonus sets the parity win-rate, clamp gives the 5% floor/ceiling.
	const double raw = kDispelSkillWeight * (area_coeff * competence - affect_potency) + dispel_bonus;
	const int threshold = std::clamp(static_cast<int>(std::lround(raw)), 5, 95);
	const auto access = room_spells::ClassifyRoomAffectAccess(ch, author_uid);
	if (access.free) {
		emit_debug(threshold, "ally", true);
		return true;
	}
	if (!ch->IsNpc() && access.author && !pk_agro_action(ch, access.author)) {
		emit_debug(threshold, "pk", false);
		return false;
	}
	const bool ok = number(1, 100) <= threshold;
	emit_debug(threshold, "roll", ok);
	if (!ok && matched_potency) {
		ApplyDispelDecay(*matched_potency, static_cast<float>(kDispelSkillWeight * competence), decay);
	}
	return ok;
}

// Templated dispel-loop shared by the char and room branches of CastUnaffects. The helpers
// (UnaffectConditionMet / CollectRemovals / DispelSucceeds / RemoveAffectAndAnnounce) are all
// overloaded on the target type, so overload resolution picks the right one at instantiation.
// PK aggro gating is char-only; for a RoomData target the `if constexpr` branch is dropped.
template<typename TTarget>
EStageResult RunCastUnaffects(CharData *ch, TTarget *target, ESpell spell_id,
							  const talents_actions::TalentUnaffect &unaffect, double area_coeff,
							  double &removed_out, double competence) {
	const bool blocking = UnaffectConditionMet(target, unaffect.GetBlocking());
	const bool breaking = UnaffectConditionMet(target, unaffect.GetBreaking());
	bool break_chain = breaking;

	const Bitvector flags = unaffect.GetAffectFlags();
	std::vector<RemovalCandidate> to_remove;
	CollectRemovals(target, unaffect.GetRemoveAnyway(), to_remove, flags, unaffect.GetDebuffOnly());
	if (!blocking) {
		CollectRemovals(target, unaffect.GetRemove(), to_remove, flags, unaffect.GetDebuffOnly());
	}
	// issue.spells-hotfix: an affect with several instances (e.g. kPoisoned on several locations) must
	// be removed and announced ONCE. RemoveAffectAndAnnounce strips ALL instances, so dedup the queue
	// by affect identity -- otherwise the 2nd+ entries re-announce an already-removed affect.
	{
		std::vector<RemovalCandidate> deduped;
		for (const auto &c : to_remove) {
			bool dup = false;
			for (const auto &e : deduped) {
				const bool same = std::is_same_v<TTarget, CharData> ? (e.affect_type == c.affect_type)
																	: (e.room_affect_type == c.room_affect_type);
				if (same) { dup = true; break; }
			}
			if (!dup) { deduped.push_back(c); }
		}
		to_remove.swap(deduped);
	}

	if (!to_remove.empty()) {
		if constexpr (std::is_same_v<TTarget, CharData>) {
			// PK-action check: keyed on the first dispelled affect; a disallowed action aborts the
			// removal entirely. Char target only.
			// issue.spell-ally-aggression (#3455): a benign cast (violent="N") that only
			// incidentally strips a buff is not an aggressive act. Gate the PK check on the
			// cast spell's per-target violence verdict, exactly like CastAffect does -- without
			// it, group prismatic aura stripping an ally's sanctuary tripped pk_agro_action and
			// evicted the caster from the clan castle. Violent dispels still aggro.
			if (ch != target && MUD::Spell(spell_id).IsViolentAgainst(ch, target)) {
				// issue.affect-migration: classify the dispelled affect by its OWN buff flag (the
				// affect-side analog of a spell's <misc violent>), not by the source spell. Stripping a
				// buff is aggression against the target itself; stripping a debuff undoes the work of
				// whoever the target is fighting, so it aggros that enemy; an ambiguous affect triggers
				// no automatic PK action (direction undetermined).
				const auto buff = affects::AffectBuffKind(to_remove.front().affect_type);
				if (buff == affects::EBuff::kYes) {
					if (!pk_agro_action(ch, target)) {
						return EStageResult::kSuccess;
					}
				} else if (buff == affects::EBuff::kNo && target->GetEnemy()) {
					if (!pk_agro_action(ch, target->GetEnemy())) {
						return EStageResult::kSuccess;
					}
				}
			}
		}
		bool removed_any = false;
		bool resisted_any = false;
		for (const auto &cand : to_remove) {
			// issue.affect-migration: dispel is keyed on the affect's identity -- affect_type (EAffect)
			// for a char target, room_affect_type (ERoomAffect) for a room target.
			bool ok;
			double removed_pot = 0.0;
			if constexpr (std::is_same_v<TTarget, CharData>) {
				ok = DispelSucceeds(ch, target, spell_id, cand.affect_type, unaffect.GetDispelBonus(),
								  competence, area_coeff, unaffect.GetDecay());
				if (ok) {
					for (const auto &aff : target->affected) {
						if (aff && aff->affect_type == cand.affect_type) { removed_pot = aff->potency; break; }
					}
				}
			} else {
				ok = DispelSucceeds(ch, target, spell_id, cand.room_affect_type, unaffect.GetDispelBonus(),
								  competence, area_coeff, unaffect.GetDecay());
				if (ok) {
					for (const auto &aff : target->affected) {
						if (aff && aff->affect_type == cand.room_affect_type) { removed_pot = aff->potency; break; }
					}
				}
			}
			if (ok) {
				if constexpr (std::is_same_v<TTarget, CharData>) {
					RemoveAffectAndAnnounce(ch, target, cand.affect_type);
				} else {
					RemoveAffectAndAnnounce(ch, target, cand.room_affect_type);
				}
				removed_any = true;
				removed_out += removed_pot;
			} else {
				resisted_any = true;
				if (cand.break_on_fail) {
					break_chain = true;
				}
			}
		}
		if (!removed_any && resisted_any && !MUD::Spell(spell_id).IsFlagged(kMagAffects)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
	} else if (breaking
			|| (!MUD::Spell(spell_id).IsFlagged(kMagAffects)
				&& (!unaffect.GetRemove().empty() || !unaffect.GetRemoveAnyway().empty()))) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	}

	return break_chain ? EStageResult::kBreak : EStageResult::kSuccess;
}

}  // namespace

// Data-driven dispel stage. Dispatches on the non-null target: a CharData strips affects from
// the victim (the historical path), a RoomData strips affects from the room.
// Exactly one of {victim, room} should be non-null; passing both is treated as the char path.
// TODO(#3342): CastUnaffects has no saving (success) check yet. kEnergyDrain/kWeaknes
// used to gate their kStrength/kDexterity removal behind a save in CastAffect; until
// that check is added here the buff is stripped regardless of the save. See their
// commented stub case in CastAffect.
EStageResult CastUnaffects(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	RoomData *const room = ctx.rvict;
	const ESpell spell_id = ctx.spell_id();
	if (victim == nullptr && room == nullptr) {
		return EStageResult::kSuccess;
	}

	if (!ctx.action_or_default().Contains(talents_actions::EAction::kUnaffect)) {
		return EStageResult::kSuccess;
	}
	const auto &unaffect = ctx.action_or_default().GetUnaffect();
	const int unaff_prob = unaffect.GetProb();
	if (unaff_prob < 100 && number(1, 100) > unaff_prob) {
		return EStageResult::kSuccess;
	}

	double removed = 0.0;
	const EStageResult r = (victim != nullptr)
			? RunCastUnaffects(ch, victim, spell_id, unaffect, ctx.area_coeff, removed, ctx.CompetenceBase())
			: RunCastUnaffects(ch, room, spell_id, unaffect, ctx.area_coeff, removed, ctx.CompetenceBase());
	ctx.dispelled_potency += removed;
	return r;
}

// Try to enchant a weapon. Returns the to_char message to relay to the caster, or nullptr when
// the caller should fall through to the kNoeffect fallback. Side effects: may consume a reagent
// (a held magical symbol in MAGIC1/2/3_ENCHANT_VNUM containers), set the obj's enchant, and
// silently emit kEnchantSetItem when the item is part of a set. Caller is responsible for the
// (ch, obj) null guard.


// When `obj` is null but `victim` isn't, pick a random item from the victim's equipment/
// inventory. If neither obj nor victim is given there is nothing to act on -- the function
// exits without effect.
// issue.obj-casting: alter-obj handlers (the per-spell object transforms migrated out of the old
// CastToAlterObjs switch). Each runs on ctx.ovict (resolved + kNoalter-guarded by the skeleton),
// does its OWN messaging, and returns kSuccess when it acted (or chose to stay silent) / kFail to
// ask the skeleton for the generic "no effect" line.
namespace handlers {
EStageResult AlterMsg(ActionContext &ctx, ESpellMsg key) {
	act(MUD::SpellMessages().GetMessage(ctx.spell_id(), key).c_str(), true, ctx.caster(), ctx.ovict, nullptr, kToChar);
	return EStageResult::kSuccess;
}
}  // namespace handlers


static const std::map<std::string, std::function<EStageResult(ActionContext &)>> kAlterObjHandlers = {
	{"AlterBless", handlers::AlterBless},
	{"AlterCurse", handlers::AlterCurse},
	{"AlterInvisible", handlers::AlterInvisible},
	{"AlterPoison", handlers::AlterPoison},
	{"AlterRemoveCurse", handlers::AlterRemoveCurse},
	{"AlterEnchantWeapon", handlers::AlterEnchantWeapon},
	{"AlterRemovePoison", handlers::AlterRemovePoison},
	{"AlterFly", handlers::AlterFly},
	{"AlterAcid", handlers::AlterAcid},
	{"AlterRepair", handlers::AlterRepair},
	{"AlterTimerRestore", handlers::AlterTimerRestore},
	{"AlterRestoration", handlers::AlterRestoration},
	{"AlterLight", handlers::AlterLight},
	{"AlterDarkness", handlers::AlterDarkness},
};

// Data-driven object transform (issue.obj-casting): resolve the target object (an explicit ovict,
// else a random equipped/carried item of the victim), guard kNoalter, then dispatch the per-spell
// transform handler (kAlterObjHandlers); kFail from the handler => the generic "no effect" line.
EStageResult CastToAlterObjs(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	ObjData *obj = ctx.ovict;
	const ESpell spell_id = ctx.spell_id();
	// issue.obj-casting: a prior action / component consumption may have destroyed the target
	// (deferred-extracted, flagged purged()); treat a purged target as absent.
	if (obj && obj->purged()) {
		obj = nullptr;
		ctx.ovict = nullptr;
	}
	// issue.spells-hotfix: the "collateral random item" fallback -- a cast with no explicit object
	// target also striking a random item the victim carries -- is now opt-in per spell via
	// <alter_obj collateral="on_damage"> and fires ONLY when damage was actually dealt this cast
	// (acid corroding gear; kAcid/kAcidArrow cannot target an object directly and rely on this).
	// Every other alter_obj spell (curse, poison, bless, fly, ...) leaves this off, so a cast aimed
	// at a character never alters their items -- cast the spell directly on an object to affect one.
	if (obj == nullptr && victim != nullptr
			&& ctx.action_or_default().GetAlterObj().collateral_on_damage
			&& ctx.result.damage != 0) {
		int rand = number(1, 50);
		if (rand <= EEquipPos::kBoths) {
			obj = GET_EQ(victim, rand);
		} else {
			for (rand -= EEquipPos::kBoths, obj = victim->carrying; rand && obj;
				 rand--, obj = obj->get_next_content());
		}
	}
	if (obj == nullptr) {
		return EStageResult::kSuccess;
	}
	if (obj->has_flag(EObjFlag::kNoalter)) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kObjResist).c_str(), true, ch, obj, nullptr, kToChar);
		return EStageResult::kSuccess;
	}
	ctx.ovict = obj;  // hand the resolved object to the handler
	const auto &a = ctx.action_or_default().GetAlterObj();
	const auto it = kAlterObjHandlers.find(a.handler);
	if (it == kAlterObjHandlers.end()) {
		err_log("CastToAlterObjs: unknown alter_obj handler '%s' for %s.",
				a.handler.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
		return EStageResult::kSuccess;
	}
	if (it->second(ctx) == EStageResult::kFail) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	}
	return EStageResult::kSuccess;
}

// issue.obj-casting: string-named post-load creation handlers (the spell-specific customization,
// e.g. shaping a created weapon/armor base). Signature (ch, created obj, ctx). The plain food/light
// spells need no handler. CreateWeapon/CreateArmor are PLUMBING STUBS: the base vnum is loaded by
// the skeleton; the stat/type customization (TODO) goes in these bodies.
static const std::map<std::string, std::function<void(CharData *, ObjData *, const ActionContext &)>>
		kCreationHandlers = {
	{"CreateWeapon", handlers::CreateWeapon},
	{"CreateArmor",  handlers::CreateArmor},
};

// Data-driven object creation (issue.obj-casting): load <obj_creation vnum>, run the optional
// post-load handler, narrate, then place in inventory (or drop to the room when over-encumbered).
EStageResult CastCreationAction(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	const auto &s = ctx.action_or_default().GetCreation();
	const auto tobj = world_objects.create_from_prototype_by_vnum(s.vnum);
	if (!tobj) {
		// Prototype lookup failed -- player-facing narration through the cast spell's
		// sheaf (kDefault fallback), plus a SYSERR for designers/admins
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kItemNoPrototype) + "\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found", to_underlying(spell_id), s.vnum);
		return EStageResult::kSuccess;
	}
	// Optional post-load customizer (kCreationHandlers): runs before narration/placement so it
	// can shape the freshly-loaded base object (weapon/armor). No handler => plain creation.
	if (!s.handler.empty()) {
		const auto it = kCreationHandlers.find(s.handler);
		if (it != kCreationHandlers.end()) {
			it->second(ch, tobj.get(), ctx);
		} else {
			err_log("CastCreationAction: unknown creation handler '%s' for %s.",
					s.handler.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
		}
	}

	// Creation narration: act() with $o = the new object; the
	// kDefault sheaf carries the generic lines, per-spell overrides may flavour them.
	const auto &item_room_msg =
			MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kItemCreatedToRoom);
	act(item_room_msg.c_str(), false, ch, tobj.get(), nullptr, kToRoom | kToArenaListen);
	const auto &item_char_msg =
			MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kItemCreatedToChar);
	act(item_char_msg.c_str(), false, ch, tobj.get(), nullptr, kToChar);
	load_otrigger(tobj.get());

	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else if (ch->GetCarryingWeight() + tobj->get_weight() > CAN_CARRY_W(ch)) {
		SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else {
		PlaceObjToInventory(tobj.get(), ch);
	}

	return EStageResult::kSuccess;
}

// Dispatch for spells whose effect is a hand-coded handler in spells.cpp (the kMagManual flag).
// Some handlers take only (caster, cvict) and ignore the unused `level` / `ovict` arguments.
// issue.damage-over-time: the poison DoT is now a data-driven <damage source="poison"> action (see
// CastDamage's poison branch), no longer a manual_cast handler -- PoisonDot was retired here.

// name -> hand-coded handler. The <manual_cast><handler val="..."/> on an
// action selects one by name (which matches the function name), replacing the old spell_id
// switch. std::function so future Lua/closure handlers can register the same way.
static const std::map<std::string, std::function<EStageResult(ActionContext &)>> kManualHandlers = {
	{"SpellControlWeather", handlers::SpellControlWeather},
	{"SpellCreateWater",    handlers::SpellCreateWater},
	{"SpellLocateObject",   handlers::SpellLocateObject},
	{"SpellCharm",          handlers::SpellCharm},
	{"SpellEnergydrain",    handlers::SpellEnergydrain},
	{"SpellFear",           handlers::SpellFear},
	{"SpellIdentify",       handlers::SpellIdentify},
	{"SpellFullIdentify",   handlers::SpellFullIdentify},
	{"SpellHolystrike",     handlers::SpellHolystrike},
	{"SpellRecall",         handlers::SpellRecall},
	{"SpellTeleport",       handlers::SpellTeleport},
	{"SpellSummon",         handlers::SpellSummon},
	{"SpellPortal",         handlers::SpellPortal},
	{"SpellRelocate",       handlers::SpellRelocate},
	{"SummonTutelar",       handlers::SummonTutelar},
	{"SpellMentalShadow",   handlers::SpellMentalShadow},
	{"SpellAnimateDead",    SpellAnimateDead},
	{"SpellResurrection",   SpellResurrection},
	// issue.affect-migration: room-affect per-tick manual handler (was the room_affects tick_handler).
	{"HandleThunderstormTick", handlers::HandleThunderstormTick},
};

// load-time validation hook (called from SpellInfoBuilder).
bool IsManualHandlerRegistered(const std::string &name) {
	return kManualHandlers.count(name) > 0;
}

// issue.vedun-hotfix #5: expose the registered handler names (sorted, std::map order) so the Vedun
// editor can offer a pick-list and reject unknown values for the <summon/alter_obj/obj_creation/
// manual_cast handler="..."> attributes (registered as enums in enum_registry).
std::vector<std::string> SummonHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kSummonHandlers) v.push_back(p.first); return v;
}
std::vector<std::string> AlterObjHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kAlterObjHandlers) v.push_back(p.first); return v;
}
std::vector<std::string> ObjCreationHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kCreationHandlers) v.push_back(p.first); return v;
}
std::vector<std::string> ManualHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kManualHandlers) v.push_back(p.first); return v;
}

EStageResult CastManual(ActionContext &ctx) {
	const std::string &name = ctx.action_or_default().GetManualHandler();
	if (name.empty()) {
		return EStageResult::kSuccess;
	}
	const auto it = kManualHandlers.find(name);
	if (it == kManualHandlers.end()) {
		err_log("CastManual: unknown handler '%s' for spell %s", name.c_str(),
				NAME_BY_ITEM<ESpell>(ctx.spell_id()).c_str());
		return EStageResult::kSuccess;
	}
	return it->second(ctx);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

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

	// An NPC reacts to a cast only when it's actually aggressive
	// against the NPC itself -- A spells from allies (PC's pet on its master, mob-to-mob)
	// fall through silently.
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
	// align: blocks the cast when the target carries the matching
	// alignment (IsGood / IsEvil / IsNeutral). kAny means no alignment block.
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
	// align: require the target to carry the matching alignment
	// (IsGood / IsEvil / IsNeutral). kAny means no alignment requirement.
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

// True if `victim` matches any of the reflection's affect flags or its alignment.
// Bare flag/alignment match -- potency-gated reflection isn't possible today (mob/object
// affects carry no potency value), so flag presence + prob is the best we can do.
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

double ActionContext::CompetenceBase() const {
	const double real = potency_.skill_coeff + potency_.stat_coeff;
	if (is_entry_action) {
		return real;
	}
	switch (action_or_default().GetBase()) {
		case talents_actions::EActionBase::kDamage:    return damage_count;
		case talents_actions::EActionBase::kPoints:    return points_count;
		case talents_actions::EActionBase::kAffects:   return affects_potency;
		case talents_actions::EActionBase::kDispelled: return dispelled_potency;
		// issue.character-affect-triggers: base="tag" -- the event tag's numeric value (e.g. damage amount).
		case talents_actions::EActionBase::kTag:
			return static_cast<double>(event_.GetTag(action_or_default().GetTagName()));
		case talents_actions::EActionBase::kCompetence:
		default:                                       return real;
	}
}

const talents_actions::Action &ActionContext::action_or_default() const {
	// Cursor active (loop driving) -> the current action; otherwise (bypass
	// callers / rooms / area setup that never rewound) -> the spell's primary.
	const talents_actions::Action *a = action();
	if (a) {
		return *a;
	}
	// issue.perk-action-patching: a patched cast whose cursor ran past the end still reads from the view.
	if (patched_ && !action_view_.empty()) {
		return *action_view_.front();
	}
	if (external_actions_ && !external_actions_->empty()) {
		return (*external_actions_)[0];
	}
	return MUD::Spell(spell_id_).actions.primary();
}

// --- ActionContext action cursor (multi-action) ---
// Defined here (not the header) so it can reach MUD::Spell for the action list.
// A spell with NO <action> blocks (flag-only spells: kDazzle/kCombatLuck/...,
// plus the summon/creation/manual stages) still has flagged stages that run their
// effect in code. To keep them firing, an empty action list yields exactly ONE
// loop iteration with action() == nullptr; the data-reading stages then fall back
// to the spell-id getters (which return the empty-action default). So the loop runs
// max(1, action_count) times.
void ActionContext::RewindActions() {
	actions_ = external_actions_ ? external_actions_ : &MUD::Spell(spell_id_).actions.list();
	action_idx_ = 0;
}

void ActionContext::NextAction() {
	++action_idx_;
}

const talents_actions::Action *ActionContext::action() const {
	// issue.perk-action-patching: a per-cast patched chain wins over the spell's own list. patched_
	// (not emptiness) is the selector, so a perk that removed every block still overrides.
	if (patched_) {
		return action_idx_ < action_view_.size() ? action_view_[action_idx_] : nullptr;
	}
	if (!actions_ || action_idx_ >= actions_->size()) {
		return nullptr;
	}
	return &(*actions_)[action_idx_];
}

bool ActionContext::HasPendingActions() const {
	const size_t count = patched_ ? action_view_.size() : (actions_ ? actions_->size() : 0);
	return action_idx_ < std::max<size_t>(1, count);
}

// issue.perk-action-patching: materialize this cast's patched action chain. Runs once per cast in
// CallMagic. Fast path: a spell with no talent patches (nearly all of them) returns immediately and the
// cast walks the spell's own const action list -- identical to before this feature. Only when the spell
// carries patches AND the caster can use the corresponding perk do we build a per-cast view.
void ActionContext::ApplyTalentPatches() {
	if (!caster_) {
		return;
	}
	const auto &patches = MUD::Spell(spell_id_).talent_patches;
	if (patches.empty()) {
		return;   // fast path
	}
	const auto &own = MUD::Spell(spell_id_).actions.list();
	action_view_.clear();
	action_view_.reserve(own.size() + patches.size());
	for (const auto &blk : own) {
		action_view_.push_back(&blk);
	}
	patched_ = true;
	// patches arrive in EFeat-enum order (MUD::Feats() is an ordered map), so application is deterministic.
	for (const auto &ref : patches) {
		const feats::TalentPatch &p = *ref.patch;
		// Only caster-held perks are materialized here (once per cast). Target-held perks must be resolved
		// per victim inside the target loop -- not wired yet, so skip them to avoid misfiring.
		if (p.scope != feats::TalentPatch::EScope::kCaster) {
			continue;
		}
		if (!CanUseFeat(caster_, ref.feat)) {
			continue;
		}
		ApplyOnePatch(p);
	}
}

// issue.perk-action-patching: apply an op="modify" field modifier (mul/add/set) to a numeric field.
// issue.affect-action-patch-improve (Phase B): bridge the patch's mod op to the manifestation-side enum,
// so op="modify" arithmetic lives on the manifestation types (IAction::ApplyFieldMod) for both spell and
// affect action chains -- no per-field setters, no duplicated mul/add/set logic here.
static talents_actions::EFieldModOp ToFieldModOp(feats::TalentPatch::EModOp op) {
	switch (op) {
		case feats::TalentPatch::EModOp::kMul: return talents_actions::EFieldModOp::kMul;
		case feats::TalentPatch::EModOp::kAdd: return talents_actions::EFieldModOp::kAdd;
		case feats::TalentPatch::EModOp::kSet: return talents_actions::EFieldModOp::kSet;
	}
	return talents_actions::EFieldModOp::kSet;
}

void ActionContext::ApplyOnePatch(const feats::TalentPatch &p) {
	using talents_actions::Action;
	using Diff = std::vector<const Action *>::difference_type;
	const auto &payload = p.payload.list();

	auto find_idx = [&](const std::string &id) -> long {
		for (size_t i = 0; i < action_view_.size(); ++i) {
			if (action_view_[i]->GetId() == id) {
				return static_cast<long>(i);
			}
		}
		return -1;
	};
	auto payload_ptrs = [&]() {
		std::vector<const Action *> v;
		v.reserve(payload.size());
		for (const auto &b : payload) {
			v.push_back(&b);
		}
		return v;
	};

	switch (p.op) {
		case feats::EPatchOp::kReplaceAll: {
			action_view_.clear();
			const auto v = payload_ptrs();
			action_view_.insert(action_view_.end(), v.begin(), v.end());
			break;
		}
		case feats::EPatchOp::kAppend: {
			const auto v = payload_ptrs();
			action_view_.insert(action_view_.end(), v.begin(), v.end());
			break;
		}
		case feats::EPatchOp::kInsert: {
			const long i = p.anchor_id.empty() ? -1 : find_idx(p.anchor_id);
			const size_t at = (i < 0) ? action_view_.size()
									  : (p.anchor_before ? static_cast<size_t>(i) : static_cast<size_t>(i) + 1);
			const auto v = payload_ptrs();
			action_view_.insert(action_view_.begin() + static_cast<Diff>(at), v.begin(), v.end());
			break;
		}
		case feats::EPatchOp::kRemove: {
			const long i = find_idx(p.action_id);
			if (i < 0) {
				break;
			}
			if (p.effect_id.empty()) {
				action_view_.erase(action_view_.begin() + static_cast<Diff>(i));
			} else {
				patch_scratch_.push_back(*action_view_[i]);
				patch_scratch_.back().EraseManifestationsById(p.effect_id);
				action_view_[i] = &patch_scratch_.back();
			}
			break;
		}
		case feats::EPatchOp::kReplace: {
			const long i = find_idx(p.action_id);
			if (i < 0) {
				break;
			}
			if (p.effect_id.empty()) {
				action_view_.erase(action_view_.begin() + static_cast<Diff>(i));
				const auto v = payload_ptrs();
				action_view_.insert(action_view_.begin() + static_cast<Diff>(i), v.begin(), v.end());
			} else {
				patch_scratch_.push_back(*action_view_[i]);
				Action &copy = patch_scratch_.back();
				copy.EraseManifestationsById(p.effect_id);
				for (const auto &b : payload) {
					copy.MergeManifestationsFrom(b);
				}
				action_view_[i] = &copy;
			}
			break;
		}
		case feats::EPatchOp::kAddEffect: {
			const long i = find_idx(p.action_id);
			if (i < 0) {
				break;
			}
			patch_scratch_.push_back(*action_view_[i]);
			Action &copy = patch_scratch_.back();
			for (const auto &b : payload) {
				copy.MergeManifestationsFrom(b);
			}
			action_view_[i] = &copy;
			break;
		}
		case feats::EPatchOp::kModify: {
			// find the block carrying the target manifestation kind (by action id if given, else first match)
			long i = -1;
			if (!p.action_id.empty()) {
				i = find_idx(p.action_id);
			} else {
				for (size_t k = 0; k < action_view_.size(); ++k) {
					if (action_view_[k]->Contains(p.mod_kind)) { i = static_cast<long>(k); break; }
				}
			}
			if (i < 0) {
				break;
			}
			patch_scratch_.push_back(*action_view_[i]);
			Action &copy = patch_scratch_.back();
			talents_actions::IAction *man = copy.CloneManifestation(p.mod_kind);
			if (man) {
				man->ApplyFieldMod(p.mod_field, ToFieldModOp(p.mod_op), p.mod_value);
			}
			action_view_[i] = &copy;
			break;
		}
	}
}

// cast each of the action's <side_spell> spells as a full nested pipeline on
// the current target. Each side cast gets its OWN rolls (its potency/success, possibly a
// different base_skill) and a fresh context, so its damage/affect accumulators and reactions
// are isolated. Loop guard (option 5): a spell already in ctx.casting (this chain) is refused.
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
		sub.SetNested();             // issue.attack-ward: a side-spell delivery must not re-run whole-cast wards
		sub.cvict = ctx.cvict;       // cast on the current target (this is a per-target stage)
		sub.area_coeff = ctx.area_coeff;  // inherit the outer per-target falloff (mass spells)
		sub.SetEvent(ctx.Event());   // issue.character-affect-triggers: a side-spell's handlers see the event too
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
	// Whole-cast gates: only for the entry action.
	if (is_entry) {
		cvict = MaybeReflectToCaster(caster, cvict, spell_id);
		ctx.cvict = cvict;
		// issue.attack-ward: WHOLE-CAST defender wards (Magic Mirror reflect / scope="all" absorb /
		// magical shield), once for the TOP-LEVEL cast. Reflect redirects ctx.cvict to the caster; a
		// whole-cast absorb stops here. Scoped damage/affect absorbs are per-manifest (CastDamage/
		// CastAffect). A side-spell sub-cast (ctx.Nested()) is part of the same incoming spell, so it
		// must NOT re-run these -- otherwise the whole-cast ward would fire twice (outer + delivery).
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
	// issue.obj-casting: <alter_obj> runs as a per-target stage (replaces the kMagAlterObjs
	// one-shot). Pure action-based, so the flag can go in Stage F.
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
	// issue.obj-casting: the object transform runs here (after manual, as the old one-shot did),
	// per target -- so an area corrode (kAcidArrow) still hits each target's item. Gated on
	// !target_died only (NOT stop_stages): the old one-shot ran independently of an affect/unaffect
	// break, so e.g. remove-curse still cleans the object even if the char-side dispel did nothing.
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

// Сообщения массовых/площадных заклинаний вынесены в lib/cfg/spell_msg.xml
// kAreaToChar / kAreaToRoom / kAreaToVict, доступны через
// MUD::SpellMessages(). См. CallMagicToArea / CallMagicToGroup.

void TrySendCastMessages(CharData *ch, CharData *victim, RoomData *room, ESpell spell_id) {
	if (room && world[ch->in_room] == room && IsAbleToSay(ch)) {
		const auto &sheaf = MUD::SpellMessages()[spell_id];
		if (sheaf.HasMessage(ESpellMsg::kAreaToChar)) {
			// вот тут надо воткнуть проверку на группу.
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
// coefficient and the matching cast_success scaling per target. A single-target cast keeps
// the old path: no roster, no distribution, no cast_success scaling.
static ECastResult RunActionOverTargets(ActionContext &ctx, const std::vector<CharData *> &targets,
										ECastTargets scope, bool is_entry) {
	CharData *caster = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (scope == ECastTargets::kSingle) {
		ctx.area_coeff = 1.0;
		ctx.cvict = targets.empty() ? nullptr : targets.front();
		return CastOnTarget(ctx, is_entry);
	}
	// issue.unstable-hotfixes: a multi-target spell with NO <area> block (e.g. a kMagGroups
	// heal) must hit the WHOLE roster at full strength -- the historical group-spell default.
	// GetArea() THROWS when there is no <area>, so guard on Contains() first (an unguarded
	// GetArea() here aborted the whole cast via CastSpell's catch -> 0 effect).
	const auto &action = ctx.action_or_default();
	const talents_actions::Area *area =
			action.Contains(talents_actions::EAction::kArea) ? &action.GetArea() : nullptr;
	// N = number of targets actually hit. No <area>, or max <= 0, means "no upper limit" ->
	// everyone in the roster; else the historical count, capped at the roster size.
	const int n = (area == nullptr || area->max_targets <= 0)
			? static_cast<int>(targets.size())
			: std::min(area->CalcTargetsQuantity(GetSkill(caster, MUD::Spell(spell_id).GetSuccessRoll().GetBaseSkill()),
													  ctx.potency().stat_coeff),
					   static_cast<int>(targets.size()));
	// issue.perk-action-patching: kMultipleCast is now data-driven (an op="modify" talent patch scales the
	// <area> decay by 0.6 for holders), so we just read the possibly-patched decay. NPC casters ignore it
	// anyway (coeff is forced to 1.0 below), which is why the old !IsNpc() guard was redundant.
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

// the ECastTargets scope implied by a per-action target selector -- decides
// single-target vs roster/distribution handling. kTarSame inherits the previous action's scope.
static ECastTargets ActionTargetScope(talents_actions::EActionTarget sel, ECastTargets prev_scope) {
	switch (sel) {
		case talents_actions::EActionTarget::kTarFoes:  return ECastTargets::kFoes;
		case talents_actions::EActionTarget::kTarGroup: return ECastTargets::kFriends;
		case talents_actions::EActionTarget::kTarMinions: return ECastTargets::kFriends;  // multi (all minions)
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
			// issue.room-affect-trigger-improve: the char that triggered an event trigger (the room
			// enterer), carried on ctx.cvict by the entry dispatcher.
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
			// issue.soullink-affect-patching: the caster/bearer's charm master (kSoulLink heals the master).
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

// Main cast entry:
// walks the spell's <action> list and runs each action over its own target list. Action[0] (the
// entry) targets the spell's scope (kSingle / kFoes / kFriends); later actions reuse the
// previous list (the kTarSame default until per-action <target> resolving lands). Entry-only
// gates (reflection / mtrigger / one-shots) fire on action[0] -- see CastOnTarget's is_entry.
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
			// A <summon> action spawns a mob in the caster's room ONCE (not per target), so -- like
			// kTarRoomThis -- it runs at the action level, not via the per-target path. Running it here
			// (entry or later) lets a summon chain off a prior action's result via base= (e.g. a
			// familiar whose strength scales with the damage just dealt). is_entry_action is set so the
			// entry summon reads the real potency C while a chained one reads its base= accumulator.
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
			// An <obj_creation> action creates an object once (not per target) -- like <summon> it
			// runs at the action level. is_entry_action is set so a future scaling handler can read
			// the action competence (entry C or base=).
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

// Run a single, cycled action of a (kService) tick spell on the room -- the action-cycling room
// tick. `phase` selects the action: action[phase % N], so a multi-phase room effect (e.g. deadly
// fog) advances one action per round. The action's own target marker drives whom it hits
// (kTarRoomThis -> the room; kTarFoes/kTarGroup/... -> the room's occupants).
// Run one cycled action (action[phase % N]) of an action list on the room. Shared by the kService
// tick-spell path and the affect-owned-actions path below.
// issue.character-affect-triggers: shared recursion guard for EVERY trigger-launched action chain
// (kKill / kExpired / kDispell / kPreHit / kPostHit / room entry+tick) -- they all run through
// RunRoomCycledAction. A triggered action that itself deals damage can kill -> ProcessDeath -> another
// trigger -> back here; the single-threaded game loop lets a plain counter bound the nesting. Reaching
// the cap almost always means a content loop, so at the cap the chain is skipped and logged once.
// Replaces the kKill-only depth guard that used to live in Damage::ProcessDeath.
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
	// issue.affect-migration: a manual_cast action runs its hand-coded handler (the same kManualHandlers
	// registry spells use), not the per-target / room-impose path. The handler reads its context (e.g.
	// the tick duration) off ctx. Replaces the old room-affect tick_handler attribute.
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

// issue.affect-action-patch-improve (Phase A): run a bearer's eligible ADDITIVE affect talent-patches
// (op=append/insert) for one affect type, as EXTRA actions alongside the affect's own action. Shared by all
// three affect runners (tick / lifecycle / per-hit event). `accept` picks which payload actions fire (those
// matching the runner's trigger). Gated on the RELATIVE (self/master/group-leader per the patch) holding the
// feat: CanUseFeat(ResolvePatchRelative(bearer, relative), feat). Mutating ops (replace/remove/modify) are
// Phase B and skipped here. Returns true if the bearer was purged (caller should stop touching it).
template <typename TriggerPred>
static bool RunAdditiveAffectPatches(ActionContext &ctx, CharData *ch, RoomData *room,
									 EAffect type, TriggerPred accept) {
	std::vector<talents_actions::Action> extra;
	for (const auto &ref : feats::AffectTalentPatches(type)) {
		if (ref.patch->op != feats::EPatchOp::kAppend && ref.patch->op != feats::EPatchOp::kInsert) {
			continue;   // additive ops only in Phase A; mutating ops (replace/remove/modify) are Phase B
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

// issue.affect-action-patch-improve (Phase B): apply an affect's MUTATING talent-patches (modify / remove /
// replace / add_effect / replace_all) IN PLACE to the per-firing COPY of its own matching actions, BEFORE
// the runner executes them. Because `list` is a copy (never the shared affect config), mutation is safe.
// Additive ops (append/insert) are handled separately by RunAdditiveAffectPatches. `accept` filters both
// which of the affect's actions we address and any payload we install, to this trigger occasion. Each patch
// is gated on CanUseFeat(ResolvePatchRelative(bearer, relative), feat) -- same gate as the additive path.
template <typename TriggerPred>
static void ApplyMutatingAffectPatches(std::vector<talents_actions::Action> &list, CharData *ch,
									   EAffect type, TriggerPred accept) {
	using talents_actions::Action;
	for (const auto &ref : feats::AffectTalentPatches(type)) {
		const auto op = ref.patch->op;
		if (op == feats::EPatchOp::kAppend || op == feats::EPatchOp::kInsert) {
			continue;   // additive ops run separately (RunAdditiveAffectPatches)
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

// issue.affect-migration: run an AFFECT's own actions (room_affects.xml <actions>) on the room each
// tick. Context (level/potency) comes from the imposing spell (ctx_spell = aff->type); the action list
// is the affect's, injected via UseExternalActions. `tick_duration` (when non-null) carries the ticking
// affect's current duration in and back out, so a manual_cast handler can branch on / modify it.
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
	// issue.damage-over-time: author credited for a poison <damage source="poison"> tick (the poisoner).
	ctx.SetDamageAuthorUid(damage_author);
	// issue.character-affect-triggers: room/exit affect-owned damage flavor for a <damage> action (empty
	// => the generic combat line, unchanged). Same channel the char runners use (Damage::Process reads it).
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

// issue.character-affect-triggers: run one CHARACTER affect's pulse/battle-pulse <actions> on its bearer
// (the char-affect analog of RunRoomTick). kPulse fires every tick; kBattlePulse only while the bearer is
// fighting. The action's effect targets the bearer via kTarSame (caster == bearer here). Threads the
// affect's duration through so a manual_cast action can end it early. Returns true if any action fired.
bool RunCharAffectTick(CharData *ch, const Affect<EApply>::shared_ptr &aff) {
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
	// issue.affect-action-patch-improve (Phase B): let a perk MUTATE this affect's per-tick actions (e.g.
	// scale a DoT's damage, strip its effect) before they run -- on the copy, so the affect config is intact.
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
	// Spell-free tick on the affect's stored potency (ctx_spell kUndefined); the bearer is the caster, so
	// a <damage> action targeting kTarFightSelf damages the bearer (a lone action's default kTarSame
	// resolves to the previous action's targets -- empty -- so it must be explicit). Room = bearer's room.
	// issue.damage-over-time: give the tick's <damage> the affect's own damage flavor (kDamageTo*), same as
	// the trigger path (RunCharAffectTrigger) -- without this a DoT deals damage silently (no visible line).
	CastRoomTickActionFromActions(ch, world[ch->in_room], ESpell::kUndefined, pulse, phase, &dur, aff->potency,
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToChar),
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToVict),
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToRoom),
			aff->caster_id);   // issue.damage-over-time: poison <damage> credits the poisoner
	aff->duration = dur;
	// issue.affect-action-patch-improve: run the affect's ADDITIVE talent-patches this tick (fresh ctx with
	// the tick's potency/author), matching the pulse triggers -- e.g. a perk that adds an effect each DoT tick.
	if (!ch->purged() && !feats::AffectTalentPatches(aff->affect_type).empty()) {
		ActionContext pctx = BuildActionContext(ch, ESpell::kUndefined, GetRealLevel(ch), aff->potency);
		pctx.SetDamageAuthorUid(aff->caster_id);
		RunAdditiveAffectPatches(pctx, ch, world[ch->in_room], aff->affect_type,
				[combat](const talents_actions::Action &a) {
					const auto &tt = a.GetTrigger();
					return tt.test(talents_actions::EActionTrigger::kPulse)
						|| (combat && tt.test(talents_actions::EActionTrigger::kBattlePulse));
				});
	}
	// issue.damage-over-time: a DoT imposed with <charges max=N> lasts exactly N ticks -- one charge per
	// tick, in OR out of combat -- independent of the (PC-vs-NPC, *30-scaled) duration. So a burn ticks the
	// same number of times whether the bearer is fighting or not. When depleted, mark it expired (duration
	// 0) so the caller's normal duration==0 removal runs next pass, rather than erasing ch->affected here
	// mid-iteration. charges == -1 (the default) leaves the affect purely duration-bound.
	if (!ch->purged() && aff->charges != -1 && --aff->charges <= 0) {
		aff->duration = 0;
	}
	return true;
}

// issue.character-affect-triggers: run ONE affect type's <actions> matching `trig`, as caster `ch` and
// with event.actor = `actor`. Unlike RunCharEventTriggers (which scans ALL of ch's affects for a per-hit
// event), this fires exactly one affect type for a per-affect lifecycle event. The CALLER picks the
// subject `ch`: for kExpired the bearer (its own timer/charges ran out; actor=null -> self/ally targets);
// for kDispell the DISPELLER (so the retaliation is self-inflicted -- kTarFightSelf lands on the dispeller
// as self-damage, no PvP against the bearer). Recursion is bounded inside RunRoomCycledAction.
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
		// issue.character-affect-triggers: kPoints category filter -- if this action pins a category
		// (<trigger category=>) and the restored category doesn't match, skip it. Actions with no
		// category (and every non-kPoints trigger, where event_category is -1) always pass.
		if (event_category >= 0 && a.GetTriggerPointsCategory() >= 0
				&& a.GetTriggerPointsCategory() != event_category) {
			continue;
		}
		fired.push_back(a);
	}
	// issue.affect-action-patch-improve (Phase B): let a perk MUTATE this affect's lifecycle actions before
	// they run (e.g. change an on-expire retaliation), matching this trigger. On the copy -- config intact.
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
	// issue.character-affect-triggers: <action target="kTarActor"> resolves off ctx.cvict (see
	// ResolveActionTargets), so point cvict at the actor (the dispeller for kDispell, the healer for
	// kPoints). Without this it would fall back to the bearer/caster and the retaliation would hit the
	// wrong char. For an actor-less trigger (kExpired) this leaves cvict null -> kTarActor yields nothing.
	ctx.cvict = actor;
	EventContext ev;
	ev.trigger = trig;
	ev.actor = actor;
	ev.amount = event_amount;
	ev.points_category = event_category;
	ctx.SetEvent(ev);
	// issue.character-affect-triggers: give any <damage> in this affect's trigger chain the affect's own
	// damage flavor (kDamageTo* sheaf) instead of the generic combat hit line. Empty slots -> the
	// generic message still shows (Damage::Process treats "no affect message" as before).
	ctx.SetAffectDamageMsg(affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToChar),
						   affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToVict),
						   affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToRoom));
	RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
	// issue.affect-action-patch-improve: also run this affect's additive talent-patches for this lifecycle
	// trigger (e.g. an on-expire / on-dispel perk effect), matching `trig`.
	RunAdditiveAffectPatches(ctx, ch, world[ch->in_room], affect_type,
			[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
	return true;
}

// issue.character-affect-triggers: fire the DYING char's kDeath actions (from die(), before raw_kill).
// Scans the char's own affects for kDeath actions and runs each on the char itself (kTarFightSelf ->
// self-heal), with event.actor = the killer. Returns true if any kDeath action resolved a "block"
// (<trigger return="0"/> or a handler override): the death is then prevented by die(). A preventing
// affect must heal the char (a <points><heal>) -- this runner does NOT auto-heal, so an unhealed block
// just delays death to the next tick (by design). Recursion is bounded inside RunRoomCycledAction.
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
		// issue.character-affect-triggers: the affect's own kDeath flavor (optional -- empty => silent),
		// shown after the action ran (so a "torn back from death" line follows the heal). $n = the bearer.
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
			// issue.character-affect-triggers: a charge is spent ONLY when the affect actually PREVENTS
			// death (a non-preventing kDeath -- curse/damage the killer -- burns nothing; and if the char
			// dies the counter is moot, it's removed with the char). Deferred: spend after the loop so
			// removing a depleted affect can't invalidate this iteration over ch->affected.
			spend_after.push_back(aff->affect_type);
		}
	}
	for (const EAffect t : spend_after) {
		SpendCharAffectCharge(ch, t);
	}
	return prevented;
}

// issue.character-affect-triggers: run every action on the bearer's affects whose trigger matches
// event.trigger (e.g. kPreHit / kPostHit), threading `event` onto the context so manual_cast handlers
// can read its rich data (amount/weapon/skill/actor). Each action targets via its own <target> (e.g.
// kTarFightVict = the current opponent) and its <side_spell> lands on that target at the proc level (NPCs
// proc at their real level, matching the old hand-coded kCloudOfArrows bolt; PCs at level 1). Snapshots
// the affect TYPES first because a fired side-spell may mutate ch->affected. Returns true if any ran.
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
		// issue.affect-action-patch-improve (Phase B): let a perk MUTATE this affect's per-hit actions before
		// they run (e.g. scale vampirism's leech, strip an on-hit rider), on the copy -- config intact.
		if (!feats::AffectTalentPatches(t).empty()) {
			ApplyMutatingAffectPatches(fired, ch, t,
					[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
		}
		// issue.vampirism-haste: a competence-based event action scales with the FIRING affect's stored
		// potency (e.g. the on-kill kHaste is as strong as the vampirism that produced it). base="tag"
		// actions (like the leech heal) read the event amount instead, so they are unaffected.
		float aff_potency = 0.0f;
		for (const auto &a : ch->affected) {
			if (a && a->affect_type == t) { aff_potency = a->potency; break; }
		}
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level, -1.0f, aff_potency);
		ctx.UseExternalActions(&fired);   // inject the action list (RunRoomCycledAction reads it off ctx)
		ctx.SetEvent(event);              // handlers (and propagated side-spell sub-ctxs) read this
		if (!fired.empty()) {   // a remove/replace_all patch may have emptied the affect's own actions
			RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		}
		if (ch->purged()) {
			return true;
		}
		// issue.affect-action-patch-improve: run this affect's ADDITIVE talent-patch actions (e.g. kSoulLink's
		// master-heal) matching the event trigger, alongside the affect's own action.
		if (RunAdditiveAffectPatches(ctx, ch, world[ch->in_room], t,
				[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); })) {
			return true;
		}
		// issue.character-affect-triggers: spend a trigger charge -- kPreHit/kPostHit/kKill leave the
		// affect in place, so charges bound how many times it may fire (no-op when unlimited). Safe here:
		// we iterate the `types` snapshot, not ch->affected.
		SpendCharAffectCharge(ch, t);
	}
	return true;
}

// issue.room-affect-trigger-improve: on-entry trigger return values. 0 = block the action that fired
// the trigger (refuse the room entry); any non-zero = allow. The dispatcher treats "no value set" as
// allow, so non-blocking affects (e.g. kHypnoticPattern, which only casts sleep) need no return attr.
namespace {
constexpr int kEntryTriggerAllow = 1;
constexpr int kEntryTriggerBlock = 0;
}

// issue.room-affect-trigger-improve: spend one trigger charge after a triggered affect's action runs.
// charges == -1 means unlimited (the default -- no-op). At 0 the affect is consumed: removed from its
// host list (the room's or the exit's `affected`) with its kAffExpired line sent to the actor's room.
static void ConsumeAffectCharge(const Affect<room_spells::ERoomApply>::shared_ptr &aff, RoomData *room,
								room_spells::RoomAffects &host, CharData *actor) {
	if (!aff || aff->charges == -1) {
		return;
	}
	if (--aff->charges <= 0) {
		// issue.character-affect-triggers: count expired == timer expired -- fire the affect's kExpired
		// trigger (in its caster's context, like the room/exit timer-expiry path) before it is consumed.
		// No-op if the caster is gone or the affect has no kExpired action.
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
	// issue.room-affect-trigger-improve: prob gate -- the trigger fires only `prob` percent of the time;
	// a (100-prob)% miss leaves the event untouched (no effect, no block). The chance is the affect's
	// own seal strength (the per-cast Affect::modifier, e.g. kForbidden) when it carries a seal_strength
	// formula, else the static <trigger prob=N>, else always (no roll). So a seal's strength IS its
	// block chance, exactly as the old IsRoomForbidden roll did.
	const std::optional<int> prob = room_spells::RoomAffectHasSeal(affect_type)
			? std::optional<int>(seal_strength)
			: action.GetTriggerProb();
	if (prob && number(1, 100) > *prob) {
		return kEntryTriggerAllow;
	}
	// issue.room-affect-trigger-improve: the affect's own "why this happened" flavor, shown to the actor
	// and the room BEFORE the action's effect runs, keyed by the room affect's sheaf AND by which trigger
	// fired -- so "walked through" / "opened" / "picked" / "unlocked" read distinctly (sheaf-direct ->
	// silent for affects that author no such line).
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
	// issue.room-affect-trigger-improve: a trigger that fired but whose side_spell produced no visible
	// effect would read as a silent no-op. That happens when the cast is refused (kNotCast) OR when a
	// violent side_spell is suppressed by a peaceful room (a god-sourced trap still "casts" but lands no
	// damage). In either case emit the affect's room-wide kTriggerNoEffect flash so the trap is felt.
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

// issue.room-affect-trigger-improve (door affects): fire a door affect's triggers when someone
// pick/unlock/opens the door in direction `dir` from `room`. Mirrors RunRoomEntryTriggers but the
// host is the EXIT's affect list, and there is a single dispatch point (the door command) so no
// before/after phase split -- the action runs and a return of 0 refuses the door action. Returns
// true to allow the door action, false to refuse it.
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
	// issue.room-affect-trigger-improve (reverse-exit resolve): a door affect cast from the FAR room
	// lives on the reverse exit, so resolve the door's other side (one validated hop, like do_doorcmd's
	// `back`) and fire its affects too -- the trap reacts no matter which side opens the door.
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
			// issue.room-affect-trigger-improve: kEnter passed here = the ENTER FAMILY (passage move-through),
			// expanding by actor kind exactly like RunRoomEntryTriggers; the door verbs (kOpen/kPick/kUnlock)
			// are matched verbatim.
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

// issue.room-affect-trigger-improve (door affects): dispel dispellable affects on the passage `dir`
// from `caster` (`развеять магию <dir>`). Reverse-resolved -- a trap cast from either side is reachable.
// Mirrors the room dispel's access model: the affect's author or a live ally strips it for free; an
// outsider must out-potency it (5% lucky override); a player vs a live author commits a PK act.
// Returns true if any affect was dispelled. Validation + the overall no-effect line belong to the
// caller (CallMagicToExit); this just runs the contest over the passage's (reverse-resolved) affects.
bool room_spells::DispelExitAffects(CharData *caster, int dir, ESpell spell_id) {
	if (!caster || caster->in_room == kNowhere || dir < 0 || dir >= EDirection::kMaxDirNum) {
		return false;
	}
	const auto near_exit = world[caster->in_room]->dir_option[dir];
	if (!near_exit) {
		return false;
	}
	std::vector<RoomData::exit_data_ptr> hosts{near_exit};
	if (near_exit->to_room() != kNowhere) {
		static const int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth,
									   EDirection::kEast, EDirection::kDown, EDirection::kUp};
		const auto rev = world[near_exit->to_room()]->dir_option[rev_dir[dir]];
		if (rev && rev->to_room() == caster->in_room) { hosts.push_back(rev); }
	}
	ActionContext ctx = BuildActionContext(caster, spell_id, GetRealLevel(caster));
	bool any = false;
	for (const auto &ex : hosts) {
		for (auto it = ex->affected.begin(); it != ex->affected.end();) {
			const auto af = *it;
			if (!af || !IS_SET(af->battleflag, kAfDispellable)) {
				++it;
				continue;
			}
			const float spell_potency = static_cast<float>(ctx.CompetenceBase());
			const auto access = room_spells::ClassifyRoomAffectAccess(caster, af->caster_id);
			bool ok;
			if (access.free) {
				ok = true;
			} else if (!caster->IsNpc() && access.author && !pk_agro_action(caster, access.author)) {
				ok = false;   // player attacking a live author's ward = a PK act they declined
			} else {
				ok = (number(1, 100) <= 5) || (spell_potency > af->potency);
			}
			if (!ok) {
				++it;
				continue;
			}
			const auto type = af->affect_type;
			const float sting_potency = af->potency;
			const auto &to_char = room_spells::RoomAffectMsgRaw(type, room_spells::ERoomAffectMsgType::kAffDispelledToChar);
			if (!to_char.empty()) { act(to_char.c_str(), false, caster, nullptr, nullptr, kToChar); }
			const auto &to_room = room_spells::RoomAffectMsgRaw(type, room_spells::ERoomAffectMsgType::kAffDispelledToRoom);
			if (!to_room.empty()) { act(to_room.c_str(), true, caster, nullptr, nullptr, kToRoom | kToArenaListen); }
			// issue.damage-over-time: fire the affect's kDispell actions (e.g. the fire-trap burning sting)
			// on the dispeller BEFORE erasing. EXIT affects previously skipped this -- only the ROOM-affect
			// dispel path (RemoveAffectAndAnnounce) fired kDispell, so a trap set on a passage never stung.
			// Run with the dispeller as caster in their room; kTarFightSelf lands it on them (no PvP).
			room_spells::RunRoomAffectTrigger(world[caster->in_room], caster, type,
					talents_actions::EActionTrigger::kDispell, sting_potency);
			it = ex->affected.erase(it);
			any = true;
		}
	}
	return any;
}

// cast `spell_id` as an area attack on every foe in the caster's room,
// regardless of the spell's own targeting flags. Used by the room-affect ticks (deadly fog /
// thunderstorm). Replaces the old direct CallMagicToArea callers.
ECastResult CastAreaInRoom(CharData *ch, ESpell spell_id, int level) {
	auto ctx = BuildActionContext(ch, spell_id, level);
	return CastSpell(ctx, ECastTargets::kFoes);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
