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

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
