/**
 \file action_points.cpp - a part of the Bylins engine.
 \brief Points stage of the cast pipeline: healing and other changes to hit/move points, thirst and
        hunger, with intensity-graded messages.
*/

#include "magic_internal.h"
#include "spell_trace.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/pk.h"
#include "magic_utils.h"
#include "engine/db/global_objects.h"

namespace {
// One row of the category dispatch table; a category whose Amount is not present is skipped.
struct PointsCategory {
	points_intensity::ECategory cat;
	ESpellMsg msg_key;
	const talents_actions::Points::Amount &amount;
};

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

// Turns a Points::Amount into a signed delta: min + beta*C*(1 + weight*d), plus bonus_mod, plus
// npc_coeff for NPCs (heal defaults to 1.0 ~ +100% for mobs; other categories to 0).
auto MakeAmountCalculator(const CharData *ch, double competencies, double bonus_mod, double noise_z) {
	return [ch, competencies, bonus_mod, noise_z](const talents_actions::Points::Amount &a) -> int {
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
				const int current = GET_COND(victim, cond_idx);
				const int removed = (current > 0) ? std::min(amount, current) : 0;
				return (current > 0) ? (removed * 100) / current : 0;
			}
			// Degrade: gain_condition clamps to [0, kMaxCondition], so predict the same here.
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
	// kToChar only; designers can add a separate kToRoom variant later.
	act(msg.c_str(), false, victim, nullptr, victim, kToChar);
}

// Positive only: a negative amount no-ops (damage goes through CastDamage). extra_percent is the
// overheal cap above max_hp in percent (0 = strict cap). Lacerations halve the heal only without
// overheal; overheal spells deliberately ignore the wound penalty.
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
	// HP уже выше оверхил-капа -- лечить нечем и урезать нельзя. Без этой проверки std::clamp
	// получал lo > hi и падал по glibc-ассерту.
	if (victim->get_hit() >= cap) {
		return;
	}
	victim->set_hit(std::clamp(victim->get_hit() + hit, victim->get_hit(), cap));
}

}  // namespace

EStageResult CastToPoints(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	const ESpell spell_id = ctx.spell_id();
	if (victim == nullptr) {
		log("MAG_POINTS: Ошибка! Не указана цель, spell_id: %d!\r\n", to_underlying(spell_id));
		return EStageResult::kSuccess;
	}
	// Every category lives in the spell's <points> block; without one the spell is misconfigured.
	if (!ctx.action_or_default().Contains(talents_actions::EAction::kPoints)) {
		mudlog(fmt::format("SYSERR: spell {} ({}) has no <points> block, CastToPoints skipped",
				to_underlying(spell_id), MUD::Spell(spell_id).GetCName()),
			CMP, kLvlImmortal, SYSLOG, true);
		return EStageResult::kSuccess;
	}
	const auto &points = ctx.action_or_default().GetPoints();

	const int prob = points.GetProb();
	if (prob < 100 && number(1, 100) > prob) {
		return EStageResult::kSuccess;
	}

	// Rolled once per cast (not per category), so heal and moves on the same cast scale together.
	const double competencies = ctx.CompetenceBase();
	const double bonus_mod = ch->add_abils.percent_spellpower_add / 100.0;
	auto calc_amount = MakeAmountCalculator(ch, competencies, bonus_mod, ctx.potency().noise_dev);
	const bool tc = spell_trace::Active(ch, victim);
	if (tc) {
		spell_trace::Line(ch, victim, "&CPoints %s -> %s: C %.2f bonus_mod %.2f area %.2f.&n\r\n",
			MUD::Spell(spell_id).GetCName(), GET_NAME(victim), competencies, bonus_mod, ctx.area_coeff);
	}

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
		// The spell-modifier hook applies to heal only, so gear effects don't scale moves.
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

	// Buffing (or draining) a fighting char is PK-relevant; checked BEFORE applying anything.
	if (any_amount && victim->GetEnemy() && ch != victim) {
		if (!pk_agro_action(ch, victim->GetEnemy())) {
			return EStageResult::kSuccess;
		}
	}

	const auto &points_sheaf = MUD::SpellMessages()[spell_id];
	for (size_t i = 0; i < std::size(categories); ++i) {
		const int amt = amounts[i];
		if (amt == 0) continue;
		const auto &c = categories[i];
		// BEFORE mutating the victim: the thirst/full degrade formula projects the after-state itself.
		const int percent = ComputePointsPercent(c.cat, amt, victim);
		if (tc) {
			spell_trace::Line(ch, victim, "&C  %s applied: amount %d, intensity %d%%.&n\r\n",
				PointsCatName(c.cat), amt, percent);
		}
		switch (c.cat) {
			case points_intensity::ECategory::kHeal:
				ApplyHeal(victim, amt, points.GetExtraPercent());
				break;
			case points_intensity::ECategory::kMoves:
				// Positive: restore (clamped at max). Negative: drain (clamped at 0).
				victim->set_move(std::clamp(victim->get_move() + amt, 0, victim->get_real_max_move()));
				break;
			case points_intensity::ECategory::kThirst:
				// XML positive = relieve thirst; the engine field is inverted, hence the negation.
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

	for (size_t i = 0; i < std::size(categories); ++i) {
		ctx.points_count += amounts[i];
	}
	update_pos(victim);

	// The restored char's affects react to being healed by an EXTERNAL caster (natural regen never
	// gets here). The reaction runs self-inflicted on the healer, so e.g. a vampirism curse harms
	// whoever heals the bearer with no PvP against the bearer. Fired once per applied category
	// (a <trigger category=> reacts only to its own) and deduped by affect type.
	if (ch != victim && !ch->purged() && !victim->purged()) {
		// Snapshot first: spending a charge may remove an affect and invalidate the iteration.
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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
