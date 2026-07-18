#include <gtest/gtest.h>

#include <memory>

#include "gameplay/skills/skills.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/classes/classes_constants.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/entities/obj_data.h"
#include "simulator/character_builder.h"

// issue.potion-hotfix: forward-declared rather than pulling the heavy engine/db/db.h into this
// unity-built test TU (it drags in descriptor_data.h / player_i.h / weather.h ...). Defined in db.cpp.
extern int ConvertDrinkPoisonField(CObjectPrototype *obj, bool proto);

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
	to->SetPotionValueKey(ObjVal::EValueKey::kMakerSkill, 100);
	to->SetPotionValueKey(ObjVal::EValueKey::kMakerStat, 30);

	auto from = std::make_shared<ObjData>(CObjectPrototype(42));
	from->set_type(EObjType::kPotion);
	from->SetPotionValueKey(ObjVal::EValueKey::kMakerSkill, 80);
	from->SetPotionValueKey(ObjVal::EValueKey::kMakerStat, 20);

	// 18 units at skill 100 mixed with 2 at skill 80 -> (18*100 + 2*80) / 20 = 98 (kvirund's example).
	drinkcon::mix_potion_values(to.get(), from.get(), /*n_to*/ 18, /*n_from*/ 2);

	EXPECT_EQ(to->GetPotionValueKey(ObjVal::EValueKey::kMakerSkill), 98);
	EXPECT_EQ(to->GetPotionValueKey(ObjVal::EValueKey::kMakerStat), 29);  // (18*30 + 2*20) / 20 = 29
}

// issue.potion-hotfix: contents freshness lives in the named kLiquidTimer key (no longer the overloaded
// val[3]). drinkcon::age_contents ages it one tick per call, only for perishable types, and spoils the
// potion when it reaches 0. This is what the decay manager drives every tick for every registered
// food/liquid.
TEST(PotionFreshness, AgeContentsDecrementsTimerForPerishableTypesOnly) {
	auto o = std::make_shared<ObjData>(CObjectPrototype(42));

	o->set_type(EObjType::kLiquidContainer);
	o->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, 3);
	EXPECT_EQ(drinkcon::age_contents(o.get()), 2);
	EXPECT_EQ(o->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer), 2);
	EXPECT_EQ(drinkcon::age_contents(o.get()), 1);
	EXPECT_EQ(drinkcon::age_contents(o.get()), 0);   // reaches 0 -> spoiled
	EXPECT_EQ(drinkcon::age_contents(o.get()), 0);   // stays 0 (no freshness clock running)

	// A non-perishable type (a standalone potion bottle) is never aged via kLiquidTimer -- its own decay
	// is the object timer -- so age_contents is a no-op and leaves the key untouched.
	auto p = std::make_shared<ObjData>(CObjectPrototype(43));
	p->set_type(EObjType::kPotion);
	p->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, 5);
	EXPECT_EQ(drinkcon::age_contents(p.get()), 0);
	EXPECT_EQ(p->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer), 5);
}

// issue.potion-hotfix: when the freshness timer hits 0 the potion SPOILS -- its power inputs
// (kMakerSkill/kMakerStat) are halved. (The poison level is a % of the spell potency, which needs the
// spell registry, so it is exercised at runtime; here we pin the registry-independent halving, incl.
// that repeated spoilage drives the power all the way to zero -- a stored 0 is honored, not re-defaulted.)
TEST(PotionSpoilage, TimerZeroHalvesPowerInputsDownToZero) {
	auto o = std::make_shared<ObjData>(CObjectPrototype(42));
	o->set_type(EObjType::kLiquidContainer);
	o->SetPotionValueKey(ObjVal::EValueKey::kMakerSkill, 100);
	o->SetPotionValueKey(ObjVal::EValueKey::kMakerStat, 40);

	o->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, 1);
	EXPECT_EQ(drinkcon::age_contents(o.get()), 0);                                  // spoils
	EXPECT_EQ(o->GetPotionValueKey(ObjVal::EValueKey::kMakerSkill), 50);           // halved
	EXPECT_EQ(o->GetPotionValueKey(ObjVal::EValueKey::kMakerStat), 20);

	for (int i = 0; i < 12; ++i) {   // each remove-poison/respoil cycle halves again
		o->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, 1);
		drinkcon::age_contents(o.get());
	}
	EXPECT_EQ(o->GetPotionValueKey(ObjVal::EValueKey::kMakerSkill), 0);            // reaches inert
	EXPECT_EQ(o->GetPotionValueKey(ObjVal::EValueKey::kMakerStat), 0);
}

// issue.potion-hotfix: the boot/load converter that migrates the historically overloaded drink/food
// val[3] into the named keys, for BOTH prototype objects (ConvertObjValues at boot) and player-held
// items (read_one_object_new on load from inventory/depot/mail/auction/clan house). val[3]==1 was
// "poisoned"; a value > 1 was a freshness countdown; a kPotion's val[3] is a spell id and must be left
// alone.
TEST(DrinkPoisonMigration, Val3SplitsIntoNamedKeys) {
	// poisoned drink (val[3]==1) -> kLiquidPoison=1, val[3] cleared
	auto poisoned = std::make_shared<ObjData>(CObjectPrototype(42));
	poisoned->set_type(EObjType::kLiquidContainer);
	poisoned->set_val(3, 1);
	EXPECT_EQ(ConvertDrinkPoisonField(poisoned.get(), false), 1);
	EXPECT_EQ(poisoned->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison), 1);
	EXPECT_EQ(poisoned->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer), -1);  // absent
	EXPECT_EQ(poisoned->get_val(3), 0);
	EXPECT_EQ(ConvertDrinkPoisonField(poisoned.get(), false), 0);                 // idempotent

	// a freshness countdown (val[3] > 1) -> kLiquidTimer, val[3] cleared
	auto fresh = std::make_shared<ObjData>(CObjectPrototype(42));
	fresh->set_type(EObjType::kFood);
	fresh->set_val(3, 500);
	EXPECT_EQ(ConvertDrinkPoisonField(fresh.get(), false), 1);
	EXPECT_EQ(fresh->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer), 500);
	EXPECT_EQ(fresh->get_val(3), 0);

	// a kPotion's val[3] is a spell id, NOT poison -- the drink converter must leave it alone
	auto potion = std::make_shared<ObjData>(CObjectPrototype(42));
	potion->set_type(EObjType::kPotion);
	potion->set_val(3, 1);
	EXPECT_EQ(ConvertDrinkPoisonField(potion.get(), false), 0);
	EXPECT_EQ(potion->get_val(3), 1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
