/**
 \file action_damage.cpp - a part of the Bylins engine.
 \brief Damage stage of the cast pipeline: per-hit damage amount, magic resist, saving throw, multi-hit
        and instant death.
*/

#include "magic_internal.h"
#include "spell_trace.h"
#include "gameplay/mechanics/antisaving.h"
#include "gameplay/mechanics/poison.h"
#include "gameplay/fight/common.h"
#include "magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/abilities/abilities_rollsystem.h"

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

/**
 * Number of *extra* hits a multi-hit damage spell deals beyond its single mandatory hit.
 * The caller adds 1 for the base hit. Three modes:
 *   - no skill / kUndefined: prob% chance of `max`, else 0;
 *   - skill + prob > 0:      prob% chance of `extra` (= min(skill,75)/divisor, capped at max), else 0;
 *   - skill + prob == 0:     random 0..extra (uniform spread of attack count).
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

static int CalcTotalSpellDmg(CharData *ch, CharData *victim, ESpell spell_id,
							const talents_actions::Action &action, double competence,
							double noise_z = std::numeric_limits<double>::quiet_NaN()) {
	const auto &potency_roll = MUD::Spell(spell_id).GetPotencyRoll();
	const bool has_dmg = action.Contains(talents_actions::EAction::kDamage);
	// A <damage prob=> spell may simply not fire (default 100); a miss deals 0 damage, which is
	// handled downstream like a full magic resist.
	if (has_dmg) {
		const int dmg_prob = action.GetDmg().GetProb();
		if (dmg_prob < 100 && number(1, 100) > dmg_prob) {
			return 0;
		}
	}
	int total_dmg{0};
	// Base per-hit amount only: magic resist is rolled per hit in CastDamage's loop.
	{
		const float skill_coeff = potency_roll.CalcSkillCoeff(ch);
		const float stat_coeff = potency_roll.CalcBaseStatCoeff(ch);
		const float bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
		const float elem_coeff = CalcMagicElementCoeff(victim, spell_id);

		float dmg;
		if (has_dmg) {
			// Additive model: amount = min + beta*C*(1 + weight*d), C = skill_coeff + stat_coeff (for a DoT
			// tick, the affect's stored potency); noise_z is the realized deviation d. An absent <amount>
			// means min 0 and weights 1.0.
			const auto &dmg_act = action.GetDmg();
			const float C = static_cast<float>(competence);
				dmg = static_cast<float>(CalcNoisyAmount(dmg_act.GetAmountMin(),
						dmg_act.GetAmountBeta() * C, dmg_act.GetAmountWeight(), /*cap*/ 0, noise_z));
		} else {
			// A kMagDamage spell without a <damage> action deals no direct damage but stays an aggressive act
			// (a 0-damage hit still starts the fight) -- what taunts like kWarcryOfChallenge rely on. Real damage
			// comes from a <damage> action or a <side_spell>.
			dmg = 0.0f;
		}

		total_dmg = static_cast<int>(dmg * elem_coeff);
		total_dmg = RollDamagePercent(total_dmg, ch->add_abils.percent_spellpower_add);
		int complex_mod = total_dmg;
		total_dmg = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, total_dmg);
		complex_mod = total_dmg - complex_mod;
		spell_trace::Line(ch, victim,
				"&CMag.dmg (%s -> %s). Skill: %2.2f, Stat: %2.2f, Amount: %2.2f, Bonus: %1.2f, Cmplx: %d, Elem.coeff: %1.2f, Total: %d &n\r\n",
				GET_NAME(ch), GET_NAME(victim), skill_coeff, stat_coeff, dmg, 1 + bonus_mod, complex_mod, elem_coeff, total_dmg);
	}

	return total_dmg;
}

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
	// Author of a self-inflicted hit (a DoT tick's caster_id), so a kill by the bearer's own tick is
	// credited to that character. 0 = no author.
	dmg.author_uid = author_uid;
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;
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

}  // namespace

EStageResult CastDamage(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	int rand = 0, count = 1, modi = 0;

	if (victim == nullptr || victim->in_room == kNowhere || ch == nullptr)
		return EStageResult::kSuccess;

	// Урон по ЭТОЙ цели: у площадного каста стадия идет по целям по очереди, и без сброса
	// на входе значение от предыдущей цели утекало бы в проверки по следующей.
	ctx.result.damage = 0;

	// A poison-sourced <damage> tick: GET_POISON-based kPoisonDmg, no-flee, credited to the poisoner.
	// Poison ignores MR, area, saving throws and wards, so the general damage path is skipped.
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
	auto ch_start_pos = ch->GetPosition();
	auto victim_start_pos = victim->GetPosition();
	const bool tc = spell_trace::Active(ch, victim);

	if (ch != victim) {
		modi = antisaving::CalcAntiSavings(ch);
		modi += antisaving::CalcClassAntiSavingsMod(ch, spell_id);
		if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc())
			modi = modi - 50;
	}

	// Roll the damage saving throw once and keep it in ctx: the instant-death gate below needs a failed
	// save. kNone saving and self-casts count as a failed save.
	if (ch != victim && ctx.action_or_default().Contains(talents_actions::EAction::kDamage)) {
		ctx.last_saving_result = CalcGeneralSaving(ch, victim,
				ctx.action_or_default().GetDmg().GetSaving(), modi);
	}

	// <instant_death prob=>: the target must fail its save, the prob roll must fire and the caster must
	// win the opposed luck test. Then an NPC takes hit + 11 (past the -11 death floor) and a PC is left at
	// 1 HP; the lethal hit ignores armour/absorb/sanct/prism so the amount lands exactly.
	if (ch != victim && ctx.action_or_default().Contains(talents_actions::EAction::kDamage)) {
		const auto &id_act = ctx.action_or_default().GetDmg();
		if (id_act.HasInstantDeath()
				&& !victim->get_role(RoleBit(EMobClass::kBoss))
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

	// Absent <hits> keeps the single hit (count = 1).
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
			// MR, scoped ward and save are rolled per hit, so a multi-hit spell is resisted per attack.
			ctx.ResetMrApplied();
			// 1) damage-scoped ward (e.g. Shadow Cloak absorb) -- consumes this hit; it emits its own msg.
			if (TryScopedAbsorb(ctx, talents_actions::EWardScope::kDamage)) {
				continue;
			}
			int hit_dmg = total_dmg;
			// 2) global magic-resist gate (skipped if an MR ward already spent this hit's MR roll).
			if (!ctx.MrApplied()) {
				const int mr = std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, GET_MR(victim));
				if (number(1, 100) <= mr) {
					hit_dmg = 0;
					if (tc) {
						spell_trace::Line(ch, victim, "&C  hit %d: resisted by magic resist (MR %d).&n\r\n",
							total_hits - count + 1, mr);
					}
				}
				ctx.SetMrApplied();
			}
			// 3) a saving throw halves this hit.
			if (hit_dmg > 0 && ch != victim
					&& ctx.action_or_default().Contains(talents_actions::EAction::kDamage)
					&& CalcGeneralSaving(ch, victim, ctx.action_or_default().GetDmg().GetSaving(), modi)) {
				hit_dmg /= 2;
			}
			const int hp_before = victim->get_hit();
			rand = LandOneDamageHit(ch, victim, spell_id, hit_dmg,
									ch_start_pos, victim_start_pos, count,
									ctx.AffectDamageMsgChar(), ctx.AffectDamageMsgVict(),
									ctx.AffectDamageMsgRoom(), ctx.DamageAuthorUid());
			// Count the HP actually removed (after resist/save, capped at the target's HP) so a chained
			// action scales off real damage; a dead victim counts the HP it had.
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
