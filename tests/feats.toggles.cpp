// The preference flags of toggled feats (style command, feat list): exactly the toggled feats have one.

#include "gameplay/abilities/feats.h"

#include <gtest/gtest.h>

#include <map>

TEST(FeatToggles, ToggledFeatsHaveTheirFlag) {
	const std::map<EFeat, EPrf> toggled = {
		{EFeat::kPowerAttack, EPrf::kPerformPowerAttack},
		{EFeat::kGreatPowerAttack, EPrf::kPerformGreatPowerAttack},
		{EFeat::kAimingAttack, EPrf::kPerformAimingAttack},
		{EFeat::kGreatAimingAttack, EPrf::kPerformGreatAimingAttack},
		{EFeat::kSerratedBlade, EPrf::kPerformSerratedBlade},
		{EFeat::kScirmisher, EPrf::kSkirmisher},
		{EFeat::kDoubleThrower, EPrf::kDoubleThrow},
		{EFeat::kTripleThrower, EPrf::kTripleThrow},
	};
	for (auto id = EFeat::kFirst; id <= EFeat::kLast; ++id) {
		const auto flag = GetPrfWithFeatNumber(id);
		const auto it = toggled.find(id);
		if (it == toggled.end()) {
			EXPECT_FALSE(flag.has_value()) << NAME_BY_ITEM<EFeat>(id);
		} else {
			ASSERT_TRUE(flag.has_value()) << NAME_BY_ITEM<EFeat>(id);
			EXPECT_EQ(*flag, it->second) << NAME_BY_ITEM<EFeat>(id);
		}
	}
	EXPECT_FALSE(GetPrfWithFeatNumber(EFeat::kUndefined).has_value());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
