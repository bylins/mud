// issue.vedun-editor Phase 0: NAMES_OF<E>() enum enumeration facility.
// The Vedun editor builds numbered enum/flag pick-lists from these (value -> name) maps.

#include <gtest/gtest.h>

#include "gameplay/magic/spells_constants.h"

// A single-token enum (ESpell): the map is non-empty, value-ordered, and round-trips
// with the existing NAME_BY_ITEM / ITEM_BY_NAME registries.
TEST(Vedun_EnumNames, SpellMembersListedAndRoundTrip) {
	const auto &names = NAMES_OF<ESpell>();
	EXPECT_FALSE(names.empty());
	ASSERT_TRUE(names.count(ESpell::kArmor));
	EXPECT_EQ(names.at(ESpell::kArmor), "kArmor");
	EXPECT_EQ(names.at(ESpell::kArmor), NAME_BY_ITEM<ESpell>(ESpell::kArmor));
	EXPECT_EQ(ITEM_BY_NAME<ESpell>("kArmor"), ESpell::kArmor);
}

TEST(Vedun_EnumNames, ElementMembersListed) {
	const auto &names = NAMES_OF<EElement>();
	EXPECT_FALSE(names.empty());
	bool has_light = false;
	for (const auto &[value, name] : names) {
		if (name == "kLight") {
			has_light = true;
		}
	}
	EXPECT_TRUE(has_light) << "EElement should enumerate kLight";
}

// A flag-set enum (EMagic): every listed (value, name) round-trips name -> value, so the
// editor can both display the members and resolve a picked one.
TEST(Vedun_EnumNames, FlagEnumRoundTrips) {
	const auto &names = NAMES_OF<EMagic>();
	EXPECT_FALSE(names.empty());
	for (const auto &[value, name] : names) {
		EXPECT_EQ(ITEM_BY_NAME<EMagic>(name), value) << "round-trip failed for " << name;
	}
}
