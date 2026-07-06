#include <gtest/gtest.h>

#include <memory>

#include "gameplay/skills/skills.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/classes/classes_constants.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/entities/obj_data.h"
#include "simulator/character_builder.h"

// issue.potion-hotfix: a potion buff's DURATION scales off the potion MAKER's skill (stored on the
// potion, brew skill or the authored default), NEVER the drinker's -- a potion acts on its own.
// CalcDuration's skill_override carries that maker skill. These pin the mechanism without a live cast
// (which can't apply an affect to an immortal -- the only easily-scriptable test char).

TEST(PotionBuffDuration, MakerSkillBonusValueBeatsZero) {
	// The skill bonus that feeds a skill-scaled buff duration, computed from an explicit maker skill.
	EXPECT_EQ(CalcNoviceSkillBonusForValue(0, 1), 0);
	EXPECT_GT(CalcNoviceSkillBonusForValue(80, 1), 0);
}

TEST(PotionBuffDuration, MakerSkillOverrideGivesDurationWhereZeroGivesNone) {
	simulator::CharacterBuilder b;
	b.make_basic_player(static_cast<short>(ECharClass::kSorcerer), 30);
	const auto v = b.get();
	ASSERT_NE(v, nullptr);

	// Purely skill-scaled duration (base 0, divisor 1, no clamp). skill_override is the potion's maker
	// skill; the drinker (the CharData passed as caster) is never consulted when it is >= 0.
	const int with_maker = CalcDuration(v.get(), v.get(), ESkill::kFirst, 0, 1, 0, 0, /*override*/ 80);
	const int no_skill   = CalcDuration(v.get(), v.get(), ESkill::kFirst, 0, 1, 0, 0, /*override*/ 0);

	EXPECT_EQ(no_skill, 0);             // a 0-skill maker -> no bonus -> buff would not stick
	EXPECT_GT(with_maker, no_skill);    // an 80-skill maker -> a real duration
}

// issue.potion-hotfix: pouring blends two potions' maker skill/stat by quantity, so the mixed
// container casts at the weighted-average strength -- not the container's original nor the poured one's.
TEST(PotionMix, WeightedBlendOfSkillAndStat) {
	auto to = std::make_shared<ObjData>(CObjectPrototype(42));
	to->set_type(EObjType::kPotion);
	to->SetPotionValueKey(ObjVal::EValueKey::kPotionSkill, 100);
	to->SetPotionValueKey(ObjVal::EValueKey::kPotionStat, 30);

	auto from = std::make_shared<ObjData>(CObjectPrototype(42));
	from->set_type(EObjType::kPotion);
	from->SetPotionValueKey(ObjVal::EValueKey::kPotionSkill, 80);
	from->SetPotionValueKey(ObjVal::EValueKey::kPotionStat, 20);

	// 18 units at skill 100 mixed with 2 at skill 80 -> (18*100 + 2*80) / 20 = 98 (kvirund's example).
	drinkcon::mix_potion_values(to.get(), from.get(), /*n_to*/ 18, /*n_from*/ 2);

	EXPECT_EQ(to->GetPotionValueKey(ObjVal::EValueKey::kPotionSkill), 98);
	EXPECT_EQ(to->GetPotionValueKey(ObjVal::EValueKey::kPotionStat), 29);  // (18*30 + 2*20) / 20 = 29
}

// issue.potion-hotfix: the CONTENTS freshness (val[3]) of a food/liquid spoils one step per tick down
// to a floor of 1, and only for perishable types. This is what the decay manager drives every tick for
// every registered food/liquid (the old code only decayed the rare item that also carried a timed spell).
TEST(PotionFreshness, DecrementsToSpoiledFloorForPerishableTypesOnly) {
	auto o = std::make_shared<ObjData>(CObjectPrototype(42));

	o->set_type(EObjType::kLiquidContainer);
	o->set_val(3, 3);
	o->decrement_freshness();
	EXPECT_EQ(o->get_val(3), 2);
	o->decrement_freshness();
	EXPECT_EQ(o->get_val(3), 1);
	o->decrement_freshness();
	EXPECT_EQ(o->get_val(3), 1);  // floored: spoiled contents never drop below 1

	o->set_type(EObjType::kFood);
	o->set_val(3, 2);
	o->decrement_freshness();
	EXPECT_EQ(o->get_val(3), 1);

	// A non-perishable type (e.g. the standalone potion bottle) is left untouched -- its own decay is the
	// object timer, not val[3] (which on a kPotion holds a spell id).
	o->set_type(EObjType::kPotion);
	o->set_val(3, 5);
	o->decrement_freshness();
	EXPECT_EQ(o->get_val(3), 5);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
