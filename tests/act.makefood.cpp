#include <gtest/gtest.h>

#include "meat.maker.hpp"

#include <unordered_set>

TEST(Act_MakeFood, Skinned)
{
	constexpr int NUMBER_OF_RUNS = 100;
	std::unordered_set<obj_vnum> returned;

	for (auto i = 0; i < NUMBER_OF_RUNS; ++i)
	{
		const auto key = meat_mapping.random_key();
		returned.insert(key);
	}

	std::unordered_set<obj_vnum> expected;
	for (const auto& mapping : MeatMapping::RAW_MAPPING)
	{
		EXPECT_TRUE(returned.find(mapping.first) != returned.end())
			<< "Key " << mapping.first << " not found in returned set. "
			<< "Try to rerun test or increase NUMBER_OF_RUNS (currently: " << NUMBER_OF_RUNS
			<< ") value. If it does not help then it is an error.";
		expected.insert(mapping.first);
	}

	for (const auto& r : returned)
	{
		EXPECT_TRUE(expected.find(r) != expected.end()) << "Encountered unexpected key " << r
			<< ". This is definitely an error.";
	}
}

TEST(Act_MakeFood, Skinned_Artefact)
{
	EXPECT_EQ(MeatMapping::ARTEFACT_KEY, meat_mapping.get_artefact_key());
}