#include <gtest/gtest.h>

#include "utils/random.h"

#include <vector>

namespace {

std::vector<int> draw_sequence(unsigned seed, int count) {
	SetRandomSeed(seed);
	std::vector<int> out;
	for (int i = 0; i < count; ++i) {
		out.push_back(number(1, 100));
	}
	return out;
}

}  // namespace

TEST(RandomSeed, SameSeedProducesSameSequence) {
	const auto a = draw_sequence(42, 50);
	const auto b = draw_sequence(42, 50);
	EXPECT_EQ(a, b);
}

TEST(RandomSeed, DifferentSeedsProduceDifferentSequences) {
	const auto a = draw_sequence(42, 50);
	const auto b = draw_sequence(43, 50);
	EXPECT_NE(a, b);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
