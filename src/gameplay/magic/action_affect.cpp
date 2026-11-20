/**
 \file action_affect.cpp - a part of the Bylins engine.
 \brief Affect stage of the cast pipeline: imposing affects (stacking, duration, modifiers, imposition
        messages and side effects), plus affects materialized from mob and equipment flags.
*/

#include "magic_internal.h"

#include "administration/privilege.h"
#include "gameplay/fight/fight_stuff.h"
#include "gameplay/mechanics/mount.h"
#include "spell_trace.h"
#include "gameplay/mechanics/antisaving.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/fight/fight_hit.h"
#include "magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/remort.h"

void ShowAffExpiredMsg(EAffect affect_type, CharData *ch) {
	if (!ch->IsNpc() && ch->IsFlagged(EPlrFlag::kWriting)) {
		return;
	}

	// The char always learns the affect wore off (generic fallback for kUndefined); the room is
	// told only if the affect authors a kAffExpiredToRoom message.
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

// Knock the victim down to `pos` (with the spell's kKnockdown* messages, dropping it from a horse)
// and/or force the fight to stop. EPosition::kUndefined as `pos` leaves the position alone and only
// runs the force_stopfight branch.
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

// Builds the affect node(s) for a mob's intrinsic (prototype-flag) buff. The affect and its granting
// spell share a name, so we roll that spell's <potency_roll> for the mob exactly as a real cast would:
// the stored potency makes dispel a contest, and each apply's modifier uses competence =
// skill_coeff + stat_coeff. An affect with <apply> children gets one node per apply; an applies-less
// buff gets a single bare flag carrier (location kNone). Nodes are permanent and
// uncredited (caster_id 0). No same-named spell -> zero roll (flat mins, potency 0).
// Такой узел СНИМАЕТСЯ магией (заклинание «разлом»): снятие идёт состязанием по силе, а при
// следующем пробуждении зоны материализация вернёт его из флагов прототипа. Неснимаемо только то,
// что ставит код: пакет призванного существа (kAfCharmBond) и волшебство надетого.
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
		af.duration = kPermanentAffectDuration;
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

std::vector<Affect<EApply>> BuildEquipmentMaterializedAffect(const ObjData *obj, EAffect affect_type,
																	 int timer, int power_percent) {
	RollResult roll;   // noise_dev stays 0 -> deterministic: an item bonus is fixed, no cast-noise draw
	try {
		const ESpell spell_id = ITEM_BY_NAME<ESpell>(NAME_BY_ITEM<EAffect>(affect_type));
		const auto &pr = MUD::Spell(spell_id).GetPotencyRoll();
		const int ilvl = static_cast<int>(obj->get_ilevel());
		const double pct = power_percent / 100.0;
		const int skill = static_cast<int>(remort::CalcSkillCap(ilvl) * pct);
		const int stat = static_cast<int>((kEquipmentAffectBaseStat + ilvl) * pct);
		roll.skill_coeff = pr.CalcSkillCoeffForValue(skill);
		roll.stat_coeff = pr.CalcBaseStatCoeffForValue(stat);
	} catch (const std::out_of_range &) {
		// no same-named spell: zero roll -> flag-only carrier / flat apply mins, potency 0
	}
	const float cast_potency = CalcCastPotency(roll);
	const double competence = roll.skill_coeff + roll.stat_coeff;
	std::vector<Affect<EApply>> nodes;
	auto make = [&](EApply loc, int mod) {
		Affect<EApply> af;
		af.affect_type = affect_type;
		af.duration = timer;
		af.location = loc;
		af.modifier = mod;
		af.caster_id = obj->get_id();
		af.potency = cast_potency;
		af.battleflag = affects::AffectFlagsByType(affect_type);
		af.battleflag.set(EAffFlag::kAfFromEquipment);
		nodes.push_back(af);
	};
	const auto &applies = affects::AffectApplies(affect_type);
	if (applies.empty()) {
		make(EApply::kNone, 0);       // flag-only buff: bare carrier (potency still gates the dispel)
	} else {
		for (const auto &ap : applies) {
			make(ap.location, ComputeApplyModifier(ap, competence, roll));
		}
	}
	return nodes;
}

// Applies one affect produced by a TalentAffect apply to the victim, honoring the
// EAffFlag update flags: kAfAccumulateDuration adds to an existing
// affect's duration, kAfUpdateDuration refreshes it to the longer value, and
// kAfUpdateMod replaces the modifier only when the new magnitude is larger. The
// caller runs affect_total() afterwards.
static void ApplyTalentAffect(CharData *victim, Affect<EApply> &af, int max_stacks,
							  const AffectFlags &extra_battleflag = {}) {
	const AffectFlags &eff_flags = affects::AffectFlagsByType(af.affect_type);
	const bool accum_dur = eff_flags.get(EAffFlag::kAfAccumulateDuration);
	const bool update_dur = eff_flags.get(EAffFlag::kAfUpdateDuration);
	const bool update_mod = eff_flags.get(EAffFlag::kAfUpdateMod);
	for (auto it = victim->affected.begin(); it != victim->affected.end(); ++it) {
		const auto existing = *it;
		const bool same_id = existing->affect_type == af.affect_type;
		// Отдельно живущий экземпляр в слияние не идёт -- накастованный баф ложится
		// рядом. Перечень видов и причины -- у IsStandaloneAffectInstance. Слияние снимает узел и
		// заводит новый из данных каста, а per-instance пометки (kAfCharmBond, вещевые) при этом
		// не переносятся, поэтому пакет призванного и вещевое волшебство после слияния становились
		// обычным бафом -- и следующее снятие уносило их насовсем.
		if (IsStandaloneAffectInstance(existing)) {
			continue;
		}
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
				// Non-stacking affect: kAfUpdateMod keeps the larger-magnitude modifier.
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
		// A reposition spell (knockdown/stun) signals success with its "knocked down" line, so a saved
		// cast needs no extra "no effect".
		if (!talent.HasReposition()) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
		spell_trace::Line(ch, victim, "&CAffect %s on %s resisted by saving throw.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim));
		return false;
	}
	// Re-apply behavior (update/accumulate) comes from affects.xml per affect_type.
	AffectFlags reapply_flags;
	bool already_affected = false;
	for (const auto &apply : talent.GetApplies()) {
		if (apply.id != EAffect::kUndefined) {
			reapply_flags.merge(affects::AffectFlagsByType(apply.id));
			already_affected = already_affected || IsAffected(victim, apply.id);
		}
	}
	const bool can_reapply = reapply_flags.get(EAffFlag::kAfAccumulateDuration)
		|| reapply_flags.get(EAffFlag::kAfUpdateDuration);
	if (ch != victim && already_affected && !can_reapply) {
		if (ch->in_room == victim->in_room) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
		spell_trace::Line(ch, victim, "&CAffect %s on %s skipped: already affected (no reapply).&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim));
		return false;
	}
	// Duration scales with the action's <duration skill=>, else the spell's potency-roll base_skill
	// (kUndefined -> flat duration). `victim` decides the unit (PC: hours -> ticks; NPC: raw).
	ESkill duration_skill = talent.GetDurationSkill();
	if (duration_skill == ESkill::kUndefined) {
		duration_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
	}
	// kAfBattledec grants are measured in combat rounds, so they skip the PC hours->ticks conversion.
	const bool raw_rounds = talent.GetBattleflags().get(EAffFlag::kAfBattledec);
	// Resistances shorten only violent effects, never a beneficial cast's duration.
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
	// `apply` is either an affect-owned affects::AffectApply or, for affects without their own applies,
	// the spell's TalentAffect::Apply; the affect id is passed explicitly since AffectApply has none.
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
		// Trigger charges, spent when a persisting-affect trigger fires.
		taf.charges = talent.GetChargesMax();
		// <affects potency_weight=> (default 1.0) lets big-modifier spells record a weaker potency so they
		// stay dispellable.
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
	// A spell that randomly picks among several affects (e.g. kAcidArrow -> blind/poison/silence) uses
	// its own per-apply data and one random pool; the affect-owned path would impose every affect's applies.
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
	// Impose each distinct affect with its own applies from affects.xml, falling back to the spell's
	// per-apply data for affects that have none. Random-flagged applies form one pool with a single winner.
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

// Shared affect-imposition narration; the affect owns the strings, `failed` picks success vs FAIL.
// To the affected (kToChar; $n=affected, $N=other), the room, and -- only when other != affected --
// the other side (kToVict). The wielded weapon is $o (armed/unarmed split by AffectMsgWeapon).
// Missing keys stay silent.
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
		// Constant-lag spells use a non-positive bonus_divisor; skill-scaling ones add
		// potency.low_skill_coeff / bonus_divisor.
		const bool tc = spell_trace::Active(ch, victim);
		if (side.HasLag()) {
			SetBattleLag(victim, potency.low_skill_coeff, side.GetLagBase(), side.GetLagBonusDivisor());
			if (tc) {
				spell_trace::Line(ch, victim, "&C  lag: base %u bonus_divisor %.2f skill_coeff %.2f.&n\r\n",
					side.GetLagBase(), side.GetLagBonusDivisor(), potency.low_skill_coeff);
			}
		}
		// E.g. kSleep knocks to kSleep, kPeaceful stops the fight (pos kUndefined). Runs after the
		// saving/affect-resist gate, so the position only changes when the debuff actually lands.
		if (side.HasReposition()) {
			ForceReposition(victim, spell_id, side.GetRepositionPos(), side.GetRepositionStopFight());
			if (tc) {
				spell_trace::Line(ch, victim, "&C  reposition: pos %d stop_fight %d.&n\r\n",
					to_underlying(side.GetRepositionPos()), side.GetRepositionStopFight() ? 1 : 0);
			}
		}
	}
	// Imposition narration belongs to the affect; an unflagged (kUndefined) affect falls back to
	// the spell's sheaf. Missing messages stay silent.
	EAffect imposed_aff = EAffect::kUndefined;
	if (action.Contains(talents_actions::EAction::kAffect)) {
		for (const auto &ap : action.GetAffect().GetApplies()) {
			if (ap.id != EAffect::kUndefined) { imposed_aff = ap.id; break; }
		}
	}
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
	// Урон и аффект в одном действии -- одно попадание, а не два независимых броска. Отбили урон
	// в ноль (спасброском, резистом, поглощением, затуханием по площади) -- аффекту взяться не от
	// чего. Чистые бафы и дебафы без <damage> не затрагиваются. Смерть цели (-1) здесь ни при чём --
	// цепочку останавливает is_vict_dead.
	if (ctx.action_or_default().Contains(talents_actions::EAction::kDamage)
			&& ctx.result.damage == 0) {
		return EStageResult::kSuccess;
	}
	ctx.ResetArApplied();   // affect-resist gates once per affect delivery

	// A benign cast is never an aggressive act. IsViolentAgainst (not IsViolent) is needed so that
	// ambiguous ("A") spells resolve by caster<->target relationship; otherwise an ally buff cast
	// on an ally in combat would trip pk_agro_action.
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
	// Scoped affect-absorb rolls per affect delivery (incl. a side-spell's); whole-cast
	// reflect/absorb already ran at the entry gate.
	if (TryScopedAbsorb(ctx, talents_actions::EWardScope::kAffect)) {
		return EStageResult::kSuccess;
	}

	// Affect-resist (GET_AR) blocks a debuff at most once per delivery. This pre-roll consumes that
	// single AR roll for a non-warcry violent debuff, so the blanket AR block below won't roll again.
	if (!MUD::Spell(spell_id).IsFlagged(kMagWarcry) && ch != victim
		&& MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) {
		ctx.SetArApplied();
		if (number(1, 999) <= GET_AR(victim) * 10) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
			spell_trace::Line(ch, victim, "&CAffect %s on %s blocked: AR pre-roll (AR %d).&n\r\n",
				MUD::Spell(spell_id).GetCName(), GET_NAME(victim), GET_AR(victim));
			return EStageResult::kSuccess;
		}
	}

	int modi = 0;
	if (ch != victim) {
		modi = antisaving::CalcAntiSavings(ch);
		modi += antisaving::CalcClassAntiSavingsMod(ch, spell_id);
	}

	if (ch->IsFlagged(EPrf::kAwake) && !victim->IsNpc()) {
		modi = modi - 50;
	}

	// A violent spell can never touch an immortal target. An "A" spell resolves against the
	// immortal: kDispellMagic on an immortal outsider is aggressive -> blocked; on a grouped
	// immortal -> non-violent, not blocked here.
	if (privilege::IsImmortal(victim) && MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		spell_trace::Line(ch, victim, "&CAffect %s on %s blocked: target is immortal.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim));
		return EStageResult::kSuccess;
	}
	// Blanket affect-resist block on any debuff, before any saving throw or affect is built. Skipped
	// if the pre-roll above already spent this stage's AR roll (warcries skip the pre-roll).
	if (!ctx.ArApplied() && ch != victim && MUD::Spell(spell_id).IsViolentAgainst(ch, victim)) {
		ctx.SetArApplied();
		if (number(1, 100) <= GET_AR(victim)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
			spell_trace::Line(ch, victim, "&CAffect %s on %s blocked: affect-resist (AR %d).&n\r\n",
				MUD::Spell(spell_id).GetCName(), GET_NAME(victim), GET_AR(victim));
			return EStageResult::kSuccess;
		}
	}
	const bool has_affect_talent = ctx.action_or_default().Contains(talents_actions::EAction::kAffect);
	// Every landed affect records the cast's potency so a later dispel can be strength-gated
	// (see CastUnaffects/DispelSucceeds).
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
			// Same value a dispel later reads: cast_potency * <affects potency_weight>.
			ctx.affects_potency += cast_potency * ctx.action_or_default().GetAffect().GetPotencyWeight();
		}
	}
	return EStageResult::kSuccess;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
