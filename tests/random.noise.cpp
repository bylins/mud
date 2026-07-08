// Part of Bylins http://www.mud.ru
// issue.random-noise-rework (P1): proof that the multiplicative truncated-normal amount
// (CalcNoisyAmount) keeps a CONSTANT relative spread (CV ~ sigma) as competence grows,
// whereas the legacy additive-dice noise (fixed variance on a growing deterministic mean)
// has a CV that shrinks toward zero. This is the whole point of the reformulation.

#include "gameplay/magic/magic_utils.h"
#include "utils/random.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

namespace {
struct Stats { double mean; double cv; };

Stats Sample(const std::function<int()> &draw, int n) {
	double s = 0.0, s2 = 0.0;
	for (int i = 0; i < n; ++i) {
		const double x = draw();
		s += x;
		s2 += x * x;
	}
	const double mean = s / n;
	const double var = std::max(0.0, s2 / n - mean * mean);
	return {mean, mean > 0.0 ? std::sqrt(var) / mean : 0.0};
}

constexpr int kN = 40000;
}  // namespace

// NEW model: amount = min + k*C*(1+eps), eps ~ truncated normal(0, sigma).
// mean scales with competence C; CV stays ~sigma at every C.
TEST(RandomNoise, MultiplicativeNoiseHasConstantCv) {
	SetRandomSeed(12345);
	const double k = 20.0, sigma = 0.15;
	for (double c : {0.5, 2.0, 4.0}) {
		const Stats st = Sample([&] { return CalcNoisyAmount(0.0, k * c, sigma, 0); }, kN);
		std::printf("[NEW] C=%.1f  mean=%.1f (k*C=%.1f)  CV=%.3f\n", c, st.mean, k * c, st.cv);
		EXPECT_NEAR(st.mean, k * c, k * c * 0.05);  // absolute magnitude scales with competence
		EXPECT_GT(st.cv, 0.10);                     // relative spread stays ~sigma ...
		EXPECT_LT(st.cv, 0.20);                     // ... constant, not vanishing
	}
}

// OLD model: amount = min + dice*dw + beta*C. Fixed-variance dice on a mean that grows
// with beta*C -> the CV falls as competence rises (randomness effectively disappears).
TEST(RandomNoise, AdditiveDiceNoiseCvShrinks) {
	SetRandomSeed(12345);
	const double dw = 0.6, beta = 20.0;
	double cv_low = 1.0, cv_high = 1.0;
	for (double c : {0.5, 4.0}) {
		const Stats st = Sample([&] {
			return static_cast<int>(std::ceil(RollDices(5, 9) * dw + beta * c));
		}, kN);
		std::printf("[OLD] C=%.1f  mean=%.1f  CV=%.3f\n", c, st.mean, st.cv);
		(c < 1.0 ? cv_low : cv_high) = st.cv;
	}
	EXPECT_LT(cv_high, cv_low);  // relative randomness fades as competence grows
}

// issue.random-noise-rework P2 (model proof): the competence-weight rebalance. Validates the exact
// anchoring arithmetic the content converter will use: hi-dominant skill weights + a real stat weight make
// competence (skill+stat) the dominant driver, k is solved to preserve today's mean at a reference skill,
// and sigma is the bounded relative-variance overlay. Demonstrates OLD (skill irrelevant, dice-dominated)
// vs NEW (skill dominates, sigma noise) on a representative fire damage spell.
namespace {
double SkillCoeff(int skill, double low, double hi) {  // weights are PERCENT (/100)
	const int lo = std::min(skill, 75), hisk = std::max(0, skill - 75);
	return (lo * low + hisk * hi) / 100.0;
}
double StatCoeff(int stat, int thr, double w) { return std::max(0, stat - thr) * w / 100.0; }
double Competence(int skill, int stat, double low, double hi, int thr, double w) {
	return SkillCoeff(skill, low, hi) + StatCoeff(stat, thr, w);
}
}  // namespace

TEST(RebalanceModel, SkillDominatesAndAnchorHolds) {
	// representative fire primary-damage spell
	const double mu_dice = 37.0, dw = 0.6, beta_old = 1.0, min_v = 0.0, sigma = 0.25;
	const int stat = 30, anchor = 140;
	auto old_mean = [&](int s) { return min_v + mu_dice * dw + beta_old * Competence(s, stat, 3, 1.25, 22, 0.5); };
	auto c_new = [&](int s) { return Competence(s, stat, 1, 7, 22, 18); };
	const double k = (old_mean(anchor) - min_v) / c_new(anchor);  // anchor NEW mean to OLD at reference skill
	auto new_mean = [&](int s) { return min_v + k * c_new(s); };

	EXPECT_NEAR(new_mean(140), old_mean(140), 0.5);          // today's balance preserved at the anchor
	EXPECT_LT(old_mean(200) / old_mean(75), 1.2);            // OLD: skill barely matters
	EXPECT_GT(new_mean(200) / new_mean(75), 3.0);            // NEW: skill dominates (here ~5x)
	// NEW CV is sigma by construction (see MultiplicativeNoiseHasConstantCv); OLD CV shrinks with skill.
	std::printf("[REBAL] sigma=%.2f k=%.2f  OLD 75/140/200 = %.1f/%.1f/%.1f (span %.2fx)  NEW = %.1f/%.1f/%.1f (span %.2fx)\n",
			sigma, k, old_mean(75), old_mean(140), old_mean(200), old_mean(200) / old_mean(75),
			new_mean(75), new_mean(140), new_mean(200), new_mean(200) / new_mean(75));
}

// issue.random-noise-rework P3: stored potency (dispel strength / cure priority / re-apply
// "keep stronger") is DETERMINISTIC competence -- the rolled dice must not leak in.
TEST(RebalanceModel, PotencyIsDeterministicCompetence) {
	// issue.potency-noise: dice retired -- CalcCastPotency = skill_coeff + stat_coeff only.
	const RollResult r{/*skill*/3.0, /*stat*/2.0, /*low*/0.0};
	EXPECT_FLOAT_EQ(CalcCastPotency(r), 5.0f);
}

// The dispel contest (issue.random-noise-rework): a d100 skill check. The win threshold is
// clamp(k*(C_dispel - affect_potency) + dispel_bonus, 5, 95). The additive bonus is a flat
// win-% at competence parity (level-independent), the competence gap shifts it by k points each,
// and the clamp gives a 5% upset floor/ceiling. Mirrors DispelSucceeds' arithmetic.
TEST(RebalanceModel, DispelContestIsSkillDrivenWithAdditiveBonus) {
	SetRandomSeed(4242);
	const double k = 4.0;  // == kDispelSkillWeight
	auto threshold = [&](double c_dispel, double affect_pot, int bonus) {
		const double raw = k * (c_dispel - affect_pot) + bonus;
		return std::clamp(static_cast<int>(std::lround(raw)), 5, 95);
	};
	auto win_rate = [&](double c_dispel, double affect_pot, int bonus) {
		const int t = threshold(c_dispel, affect_pot, bonus);
		int wins = 0;
		for (int i = 0; i < kN; ++i) {
			if (number(1, 100) <= t) {
				++wins;
			}
		}
		return static_cast<double>(wins) / kN;
	};
	// At competence parity the bonus IS the win rate -- a flat knob, independent of absolute potency.
	EXPECT_NEAR(win_rate(8.0, 8.0, 50), 0.50, 0.02);
	EXPECT_NEAR(win_rate(2.0, 2.0, 50), 0.50, 0.02);   // same bonus, tiny potencies -> same rate
	EXPECT_NEAR(win_rate(8.0, 8.0, 40), 0.40, 0.02);   // bonus 40 -> ~40%
	EXPECT_NEAR(win_rate(8.0, 8.0, 85), 0.85, 0.02);   // bonus 85 -> ~85%
	// Dispel-tuning category targets at parity (threshold = dispel_bonus + affect dispel_mod):
	EXPECT_NEAR(win_rate(8.0, 8.0, 80), 0.80, 0.02);       // debuff via a remover (bonus 80) -> ~1.25 attempts
	EXPECT_NEAR(win_rate(8.0, 8.0, 50 - 35), 0.15, 0.02);  // critical buff via dispel magic (bonus 50, mod -35)
	// Skill dominates: a competence advantage shifts the rate by k points per competence point.
	EXPECT_GT(win_rate(12.0, 2.0, 50), 0.88);          // +10 gap -> ~90%
	EXPECT_LT(win_rate(2.0, 12.0, 50), 0.12);          // -10 gap -> ~10%
	// Clamp floor/ceiling: nothing is ever guaranteed.
	EXPECT_NEAR(win_rate(50.0, 0.0, 100), 0.95, 0.02); // overwhelming -> capped at 95%
	EXPECT_NEAR(win_rate(0.0, 50.0, 0), 0.05, 0.02);   // hopeless -> floored at 5%
	// The bonus is a flat percentage-point offset: the SAME competence gap gives the SAME rate
	// regardless of the absolute competence level (unlike the old multiplier).
	EXPECT_NEAR(win_rate(9.0, 8.0, 50) - win_rate(3.0, 2.0, 50), 0.0, 0.03);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

// issue.potency-noise / issue.damage-over-time: a DoT tick's per-tick damage must scale off the
// affect's STORED POTENCY (fed as the cast competence C) -- i.e. min + beta*C -- NOT just its `min`
// floor. Locks the fix for the base_override regression, where a DoT's scale base read the always-0
// `dices` field so CalcTotalSpellDmg passed scaled = beta*0, collapsing every tick to `min`.
TEST(DamageOverTime, TickScalesOffStoredPotencyNotJustMin) {
	const double min = 5.0, beta = 2.0, C = 10.0;   // affect stored potency C = 10
	// deterministic tick (weight 0, realized noise 0): value must be min + beta*C = 25.
	const int dot = CalcNoisyAmount(min, beta * C, /*weight*/ 0.0, /*cap*/ 0, /*noise_dev*/ 0.0);
	EXPECT_EQ(dot, static_cast<int>(std::lround(min + beta * C)));
	EXPECT_GT(dot, static_cast<int>(min));   // must NOT collapse to the floor
	// The regression this guards: a zero scale base (beta*0) yields only `min`.
	EXPECT_EQ(CalcNoisyAmount(min, beta * 0.0, 0.0, 0, 0.0), static_cast<int>(min));
}
