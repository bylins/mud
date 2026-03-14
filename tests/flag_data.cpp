// Part of Bylins http://www.mud.ru
// Unit tests for FlagData

#include "engine/structs/flag_data.h"

#include <gtest/gtest.h>
#include <cstring>

// --- Default construction ---

TEST(FlagData, DefaultConstruction_IsEmpty) {
	FlagData flags;
	EXPECT_TRUE(flags.empty());
}

TEST(FlagData, DefaultConstruction_AllPlanesZero) {
	FlagData flags;
	for (size_t i = 0; i < FlagData::kPlanesNumber; ++i) {
		EXPECT_EQ(0u, flags.get_plane(i));
	}
}

// --- set / get ---

TEST(FlagData, Set_Plane0_Bit0) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(0);  // plane 0, bit 0
	flags.set(f);
	EXPECT_TRUE(flags.get(f));
	EXPECT_FALSE(flags.empty());
}

TEST(FlagData, Set_Plane1_Bit0) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(30);  // plane 1, bit 0
	flags.set(f);
	EXPECT_TRUE(flags.get(f));
	// Plane 0 should still be zero
	EXPECT_EQ(0u, flags.get_plane(0));
}

TEST(FlagData, Set_Plane2_Bit5) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(65);  // plane 2, bit 5
	flags.set(f);
	EXPECT_TRUE(flags.get(f));
}

TEST(FlagData, Set_Plane3_Bit29) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(119);  // plane 3, bit 29
	flags.set(f);
	EXPECT_TRUE(flags.get(f));
}

// Setting a flag in one plane does not affect other planes
TEST(FlagData, Set_DoesNotAffectOtherPlanes) {
	FlagData flags;
	flags.set(flag_data_by_num(0));   // plane 0
	EXPECT_EQ(0u, flags.get_plane(1));
	EXPECT_EQ(0u, flags.get_plane(2));
	EXPECT_EQ(0u, flags.get_plane(3));
}

// --- unset ---

TEST(FlagData, Unset_ClearsFlag) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(1);
	flags.set(f);
	ASSERT_TRUE(flags.get(f));
	flags.unset(f);
	EXPECT_FALSE(flags.get(f));
	EXPECT_TRUE(flags.empty());
}

TEST(FlagData, Unset_OnlyRemovesTargetFlag) {
	FlagData flags;
	const Bitvector f0 = flag_data_by_num(0);
	const Bitvector f1 = flag_data_by_num(1);
	flags.set(f0);
	flags.set(f1);
	flags.unset(f0);
	EXPECT_FALSE(flags.get(f0));
	EXPECT_TRUE(flags.get(f1));
}

// --- toggle ---

TEST(FlagData, Toggle_SetsFlag_WhenClear) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(3);
	bool result = flags.toggle(f);
	EXPECT_TRUE(result);
	EXPECT_TRUE(flags.get(f));
}

TEST(FlagData, Toggle_ClearsFlag_WhenSet) {
	FlagData flags;
	const Bitvector f = flag_data_by_num(3);
	flags.set(f);
	bool result = flags.toggle(f);
	EXPECT_FALSE(result);
	EXPECT_FALSE(flags.get(f));
}

// --- clear / set_all ---

TEST(FlagData, Clear_ResetsAllFlags) {
	FlagData flags;
	flags.set(flag_data_by_num(0));
	flags.set(flag_data_by_num(30));
	flags.clear();
	EXPECT_TRUE(flags.empty());
}

TEST(FlagData, SetAll_SetsAllBitsInAllPlanes) {
	FlagData flags;
	flags.set_all();
	EXPECT_FALSE(flags.empty());
	for (size_t i = 0; i < FlagData::kPlanesNumber; ++i) {
		EXPECT_EQ(0x3fffffffu, flags.get_plane(i));
	}
}

// --- operator== / operator!= ---

TEST(FlagData, EqualityOperator_TwoEmpty_AreEqual) {
	FlagData a, b;
	EXPECT_EQ(a, b);
}

TEST(FlagData, EqualityOperator_SameFlags_AreEqual) {
	FlagData a, b;
	a.set(flag_data_by_num(5));
	b.set(flag_data_by_num(5));
	EXPECT_EQ(a, b);
}

TEST(FlagData, InequalityOperator_DifferentFlags_AreNotEqual) {
	FlagData a, b;
	a.set(flag_data_by_num(0));
	b.set(flag_data_by_num(1));
	EXPECT_NE(a, b);
}

// --- operator+= ---

TEST(FlagData, PlusEquals_UnionOfFlags) {
	FlagData a, b;
	a.set(flag_data_by_num(0));
	b.set(flag_data_by_num(1));
	a += b;
	EXPECT_TRUE(a.get(flag_data_by_num(0)));
	EXPECT_TRUE(a.get(flag_data_by_num(1)));
}

TEST(FlagData, PlusEquals_MultiplePlanes) {
	FlagData a, b;
	a.set(flag_data_by_num(0));   // plane 0
	b.set(flag_data_by_num(30));  // plane 1
	a += b;
	EXPECT_TRUE(a.get(flag_data_by_num(0)));
	EXPECT_TRUE(a.get(flag_data_by_num(30)));
}

// --- from_string / tascii round-trip ---

// from_string("a") sets plane 0, bit 0; tascii produces "a0 "
TEST(FlagData, FromString_SingleLower_Plane0) {
	FlagData flags;
	flags.from_string("a");
	EXPECT_TRUE(flags.get(flag_data_by_num(0)));

	char buf[64] = "";
	flags.tascii(FlagData::kPlanesNumber, buf);
	EXPECT_STREQ("a0 ", buf);
}

// from_string("a1") sets plane 1, bit 0
TEST(FlagData, FromString_ExplicitPlane1) {
	FlagData flags;
	flags.from_string("a1");
	EXPECT_FALSE(flags.get(flag_data_by_num(0)));  // plane 0 unchanged
	EXPECT_TRUE(flags.get(flag_data_by_num(30)));   // plane 1, bit 0

	char buf[64] = "";
	flags.tascii(FlagData::kPlanesNumber, buf);
	EXPECT_STREQ("a1 ", buf);
}

// Multiple flags: from_string("ab") sets bits 0 and 1 of plane 0
TEST(FlagData, FromString_MultipleFlags) {
	FlagData flags;
	flags.from_string("ab");
	EXPECT_TRUE(flags.get(flag_data_by_num(0)));
	EXPECT_TRUE(flags.get(flag_data_by_num(1)));
}

// tascii on empty FlagData produces "0 "
TEST(FlagData, Tascii_EmptyFlags_OutputsZero) {
	FlagData flags;
	char buf[64] = "";
	flags.tascii(FlagData::kPlanesNumber, buf);
	EXPECT_STREQ("0 ", buf);
}

// --- set_plane / get_plane ---

TEST(FlagData, SetPlane_GetPlane_RoundTrip) {
	FlagData flags;
	flags.set_plane(2, 0xABCDu);
	EXPECT_EQ(0xABCDu, flags.get_plane(2));
	EXPECT_EQ(0u, flags.get_plane(0));
	EXPECT_EQ(0u, flags.get_plane(1));
	EXPECT_EQ(0u, flags.get_plane(3));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
