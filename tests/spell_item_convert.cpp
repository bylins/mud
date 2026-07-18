// issue.magic-items: unit tests for ConvertSpellItemToEValueKey -- the load-time migration of a
// scroll/wand/staff prototype's legacy val[] payload into the kSpellItem* ObjVal keys. Mirrors the
// mapping used by tools/convert_magic_items.py.

#include <gtest/gtest.h>

#include "engine/entities/obj_data.h"
#include "engine/db/db.h"
#include "utils/utils_parse.h"

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
	EXPECT_EQ(28, key(p, ObjVal::EValueKey::kSpell1Num));
	EXPECT_EQ(84, key(p, ObjVal::EValueKey::kSpell2Num));
	EXPECT_EQ(175, key(p, ObjVal::EValueKey::kSpell3Num));
	// idempotent: a second run finds the keys already present and does nothing.
	EXPECT_EQ(0, ConvertSpellItemToEValueKey(p.get(), true));
}

// A one-spell scroll leaves the empty slots absent.
TEST(SpellItemConvert, ScrollSingleSpell) {
	auto p = make_proto(EObjType::kScroll, 30, 28, -1, -1);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(28, key(p, ObjVal::EValueKey::kSpell1Num));
	EXPECT_LT(key(p, ObjVal::EValueKey::kSpell2Num), 0);
	EXPECT_LT(key(p, ObjVal::EValueKey::kSpell3Num), 0);
}

// Wand: val3 -> spell, val1 -> max charges, val2 -> current charges.
TEST(SpellItemConvert, WandChargesAndSpellToKeys) {
	auto p = make_proto(EObjType::kWand, 30, 20, 15, 53);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(53, key(p, ObjVal::EValueKey::kSpell1Num));
	EXPECT_EQ(20, key(p, ObjVal::EValueKey::kMaxCharges));
	EXPECT_EQ(15, key(p, ObjVal::EValueKey::kCurCharges));
}

// Staff uses the same layout as a wand.
TEST(SpellItemConvert, StaffLikeWand) {
	auto p = make_proto(EObjType::kStaff, 30, 10, 10, 42);
	EXPECT_EQ(1, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_EQ(42, key(p, ObjVal::EValueKey::kSpell1Num));
	EXPECT_EQ(10, key(p, ObjVal::EValueKey::kMaxCharges));
	EXPECT_EQ(10, key(p, ObjVal::EValueKey::kCurCharges));
}

// A non-magic-item type is never touched.
TEST(SpellItemConvert, NonSpellItemUntouched) {
	auto p = make_proto(EObjType::kArmor, 1, 2, 3, 4);
	EXPECT_EQ(0, ConvertSpellItemToEValueKey(p.get(), true));
	EXPECT_LT(key(p, ObjVal::EValueKey::kSpell1Num), 0);
}

// issue.magic-items: the unified keys keep READ-ALIASES for the old POTION_*/SPELLITEM_* on-disk
// strings and WRITE the new short canonical name -- so every existing potion/scroll/wand/staff still
// loads, and is re-saved in the unified form.
TEST(SpellItemConvert, UnifiedKeyReadAliases) {
	text_id::Init();  // idempotent
	using K = ObjVal::EValueKey;
	auto num = [](const char *s) { return text_id::ToNum(text_id::kObjVals, s); };
	auto str = [](K k) { return text_id::ToStr(text_id::kObjVals, to_underlying(k)); };

	// the new short name AND both legacy prefixes resolve to the same unified key
	EXPECT_EQ(num("SPELL1_NUM"), to_underlying(K::kSpell1Num));
	EXPECT_EQ(num("POTION_SPELL1_NUM"), to_underlying(K::kSpell1Num));
	EXPECT_EQ(num("SPELLITEM_SPELL1_NUM"), to_underlying(K::kSpell1Num));
	EXPECT_EQ(num("POTION_SKILL"), to_underlying(K::kMakerSkill));
	EXPECT_EQ(num("SPELLITEM_SKILL"), to_underlying(K::kMakerSkill));
	EXPECT_EQ(num("POTION_STAT"), to_underlying(K::kMakerStat));
	EXPECT_EQ(num("SPELLITEM_MAX_CHARGES"), to_underlying(K::kMaxCharges));

	// writing always uses the new canonical short name (the first Add wins for num->str)
	EXPECT_EQ(str(K::kSpell1Num), "SPELL1_NUM");
	EXPECT_EQ(str(K::kMakerSkill), "MAKER_SKILL");
	EXPECT_EQ(str(K::kMakerStat), "MAKER_STAT");
	EXPECT_EQ(str(K::kMaxCharges), "MAX_CHARGES");
	EXPECT_EQ(str(K::kCurCharges), "CUR_CHARGES");
	// potion-specific keys keep their original names
	EXPECT_EQ(str(K::kPotionPotency), "POTION_POTENCY");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
