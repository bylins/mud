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


// issue.magic-items-hotfix: a drink-container/fountain liquid core lives in the kLiquid* keys; the val
// mutators (get/set/dec/inc/add/sub_val) redirect indices 0..2 to them, so val[] is not the store.
TEST(DrinkconLiquidCore, ValMutatorsRedirectToKeys) {
	auto p = make_proto(EObjType::kLiquidContainer, 24, 18, 5, 0);  // set_val -> keys
	EXPECT_EQ(24, key(p, ObjVal::EValueKey::kLiquidCapacity));
	EXPECT_EQ(18, key(p, ObjVal::EValueKey::kLiquidCurrent));
	EXPECT_EQ(5,  key(p, ObjVal::EValueKey::kLiquidType));
	EXPECT_EQ(24, p->get_val(0));  // get_val reads the key
	EXPECT_EQ(18, p->get_val(1));
	EXPECT_EQ(5,  p->get_val(2));
}

TEST(DrinkconLiquidCore, DrinkingUpdatesTheKey) {
	auto p = make_proto(EObjType::kFountain, 50, 50, 2, 0);
	p->sub_val(1, 20);  // drink 20
	EXPECT_EQ(30, p->get_val(1));
	EXPECT_EQ(30, key(p, ObjVal::EValueKey::kLiquidCurrent));  // key is the store, kept in sync
	p->dec_val(1);
	EXPECT_EQ(29, key(p, ObjVal::EValueKey::kLiquidCurrent));
}

TEST(DrinkconLiquidCore, ZeroIsStoredNotAbsent) {
	auto p = make_proto(EObjType::kLiquidContainer, 24, 0, 0, 0);  // empty container of water
	EXPECT_EQ(0, key(p, ObjVal::EValueKey::kLiquidCurrent));
	EXPECT_EQ(0, key(p, ObjVal::EValueKey::kLiquidType));
	EXPECT_EQ(0, p->get_val(1));
	EXPECT_EQ(0, p->get_val(2));
}

TEST(DrinkconLiquidCore, NonDrinkTypeUsesValArray) {
	auto p = make_proto(EObjType::kArmor, 1, 2, 3, 4);
	EXPECT_LT(key(p, ObjVal::EValueKey::kLiquidCapacity), 0);  // no liquid key created
	EXPECT_EQ(1, p->get_val(0));                               // reads m_vals
}

TEST(DrinkconLiquidCore, ConvertIsNoOpWhenKeysPresent) {
	auto p = make_proto(EObjType::kLiquidContainer, 24, 18, 5, 0);  // set_val already populated the keys
	EXPECT_EQ(0, ConvertDrinkconLiquidCore(p.get(), true));
}
