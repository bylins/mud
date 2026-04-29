#include <gtest/gtest.h>

#include "utils/random.h"

#include <vector>

namespace {

std::vector<int> draw_sequence(unsigned seed, int count) {
	SetRandomSeed(seed);
	std::vector<int> out;
	out.reserve(count);
	for (int i = 0; i < count; ++i) {
		out.push_back(number(1, 1000000));
	}
	return out;
}

}  // namespace

TEST(RandomSeed, SameSeedProducesSameSequence) {
	const auto a = draw_sequence(42, 100);
	const auto b = draw_sequence(42, 100);
	EXPECT_EQ(a, b);
}

TEST(RandomSeed, DifferentSeedsProduceDifferentSequences) {
	const auto a = draw_sequence(42, 100);
	const auto b = draw_sequence(43, 100);
	EXPECT_NE(a, b);
}

TEST(RandomSeed, ResetWithinRunRestartsSequence) {
	SetRandomSeed(42);
	std::vector<int> first;
	for (int i = 0; i < 50; ++i) {
		first.push_back(number(1, 1000000));
	}
	SetRandomSeed(42);
	std::vector<int> second;
	for (int i = 0; i < 50; ++i) {
		second.push_back(number(1, 1000000));
	}
	EXPECT_EQ(first, second);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
