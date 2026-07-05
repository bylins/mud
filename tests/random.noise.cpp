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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
