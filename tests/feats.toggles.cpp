// The preference flags of toggled feats (style command, feat list): exactly the toggled feats have one.
// Also the talent patch summary printed by `show featinfo`.

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

// The talent patch summary of `show featinfo`: whose feat gates it and what a modify changes.
TEST(TalentPatchDescribe, AffectPatchShowsRelative) {
	feats::TalentPatch patch;
	patch.target_affect = EAffect::kVampirism;
	patch.relative = feats::TalentPatch::ERelative::kMaster;
	EXPECT_EQ(patch.Describe(), "op=append affect=kVampirism relative=master");
}

TEST(TalentPatchDescribe, ModifyShowsFieldAndValue) {
	feats::TalentPatch patch;
	patch.op = feats::EPatchOp::kModify;
	patch.has_has_effect = true;
	patch.has_effect_kind = talents_actions::EAction::kArea;
	patch.effect_id = "kArea";
	patch.mod_kind = talents_actions::EAction::kArea;
	patch.mod_field = "decay";
	patch.mod_value = 0.6;
	EXPECT_EQ(patch.Describe(), "op=modify has_effect=kArea effect=kArea modify=kArea.decay mul=0.6");
}

TEST(TalentPatchDescribe, InsertShowsAnchorAndScope) {
	feats::TalentPatch patch;
	patch.op = feats::EPatchOp::kInsert;
	patch.target_spell = ESpell::kFireball;
	patch.scope = feats::TalentPatch::EScope::kTarget;
	patch.anchor_id = "main";
	patch.anchor_before = true;
	EXPECT_EQ(patch.Describe(), "op=insert spell=kFireball by=target before=main");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
