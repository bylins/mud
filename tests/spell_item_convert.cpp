// issue.magic-items: unit tests for ConvertSpellItemToEValueKey -- the load-time migration of a
// scroll/wand/staff prototype's legacy val[] payload into the kSpellItem* ObjVal keys. Mirrors the
// mapping used by tools/convert_magic_items.py.

#include <gtest/gtest.h>

#include "engine/entities/obj_data.h"
#include "engine/db/db.h"

#include <memory>

namespace {

std::shared_ptr<CObjectPrototype> make_proto(EObjType type, int v0, int v1, int v2, int v3) {
	auto p = std::make_shared<CObjectPrototype>(100600);
	p->set_type(type);
	p->set_val(0, v0);
	p->set_val(1, v1);
	p->set_val(2, v2);
	p->set_val(3, v3);
	return p;
}

int key(const std::shared_ptr<CObjectPrototype> &p, ObjVal::EValueKey k) {
	return p->GetPotionValueKey(k);
}

}  // namespace

// A scroll's val[1..3] become its three spell keys; absent (-1/0) spells stay unset.
TEST(SpellItemConvert, ScrollSpellsToKeys) {
	auto p = make_proto(EObjType::kScroll, 30, 28, 84, 175);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(28, key(p, ObjVal::EValueKey::kSpellItemSpell1Num));
	EXPECT_EQ(84, key(p, ObjVal::EValueKey::kSpellItemSpell2Num));
	EXPECT_EQ(175, key(p, ObjVal::EValueKey::kSpellItemSpell3Num));
	// idempotent: a second run finds the keys already present and does nothing.
	EXPECT_EQ(0, ConvertSpellItemToEValueKey(p.get(), true));
}

// A one-spell scroll leaves the empty slots absent.
TEST(SpellItemConvert, ScrollSingleSpell) {
	auto p = make_proto(EObjType::kScroll, 30, 28, -1, -1);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(28, key(p, ObjVal::EValueKey::kSpellItemSpell1Num));
	EXPECT_LT(key(p, ObjVal::EValueKey::kSpellItemSpell2Num), 0);
	EXPECT_LT(key(p, ObjVal::EValueKey::kSpellItemSpell3Num), 0);
}

// Wand: val3 -> spell, val1 -> max charges, val2 -> current charges.
TEST(SpellItemConvert, WandChargesAndSpellToKeys) {
	auto p = make_proto(EObjType::kWand, 30, 20, 15, 53);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(53, key(p, ObjVal::EValueKey::kSpellItemSpell1Num));
	EXPECT_EQ(20, key(p, ObjVal::EValueKey::kSpellItemMaxCharges));
	EXPECT_EQ(15, key(p, ObjVal::EValueKey::kSpellItemCurCharges));
}

// Staff uses the same layout as a wand.
TEST(SpellItemConvert, StaffLikeWand) {
	auto p = make_proto(EObjType::kStaff, 30, 10, 10, 42);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(42, key(p, ObjVal::EValueKey::kSpellItemSpell1Num));
	EXPECT_EQ(10, key(p, ObjVal::EValueKey::kSpellItemMaxCharges));
	EXPECT_EQ(10, key(p, ObjVal::EValueKey::kSpellItemCurCharges));
}

// A non-magic-item type is never touched.
TEST(SpellItemConvert, NonSpellItemUntouched) {
	auto p = make_proto(EObjType::kArmor, 1, 2, 3, 4);
	EXPECT_EQ(0, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_LT(key(p, ObjVal::EValueKey::kSpellItemSpell1Num), 0);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
