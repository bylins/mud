// Part of Bylins http://www.mud.ru
// Unit tests for BitsetFlags<EnumT, N>.
//
// The central obligation: BitsetFlags must be a byte-compatible drop-in for FlagData on disk while
// changing only the in-memory representation. Most tests therefore compare BitsetFlags output and
// parsing against FlagData for the same flags.

#include "engine/structs/bitset_flags.h"
#include "engine/structs/flag_data.h"

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace {

// A flag enum spanning four planes (indices 0..119), so plane mapping is exercised.
// flag_traits is specialized below rather than via kCount so we can use sparse named members.
enum class ETestFlag : unsigned {
	kA = 0,
	kB = 1,
	kC = 2,
	kMid = 65,    // plane 2, bit 5
	kTop = 119,   // plane 3, bit 29
};

// An enum that uses the default kCount convention.
enum class EAutoFlag : unsigned {
	kX,
	kY,
	kZ,
	kCount,   // == 3
};

}  // namespace

template<>
struct flag_traits<ETestFlag> {
	static constexpr std::size_t count = 120;
};

namespace {

using TestFlags = BitsetFlags<ETestFlag, 120>;

FlagData MakeFlagData(std::initializer_list<int> indices) {
	FlagData f;
	for (const int n : indices) {
		f.set(flag_data_by_num(n));
	}
	return f;
}

TestFlags MakeBitset(std::initializer_list<int> indices) {
	TestFlags f;
	for (const int n : indices) {
		f.set_index(static_cast<std::size_t>(n));
	}
	return f;
}

void ExpectSameSerialization(std::initializer_list<int> indices) {
	const FlagData fd = MakeFlagData(indices);
	const TestFlags bf = MakeBitset(indices);

	EXPECT_EQ(fd.to_numeric_string(), bf.to_numeric_string());

	char a[256] = {0};
	char b[256] = {0};
	fd.tascii(FlagData::kPlanesNumber, a, sizeof(a));
	bf.tascii(4, b, sizeof(b));
	EXPECT_STREQ(a, b);
}

void ExpectSameParse(const char *s) {
	FlagData fd;
	fd.from_string(s);
	TestFlags bf;
	bf.from_string(s);
	EXPECT_EQ(fd.to_numeric_string(), bf.to_numeric_string()) << "input: \"" << s << "\"";
}

}  // namespace

// ---------------------------------------------------------------------------
// flag_traits / count convention
// ---------------------------------------------------------------------------

TEST(BitsetFlags, AutoCount_FromKCountSentinel) {
	static_assert(flag_count_v<EAutoFlag> == 3, "kCount sentinel should drive the default size");
	BitsetFlags<EAutoFlag> f;  // size defaulted from kCount
	EXPECT_EQ(3u, f.size());
}

TEST(BitsetFlags, Capacity_RoundsUpToWholeWord) {
	EXPECT_EQ(120u, TestFlags::size());
	EXPECT_EQ(128u, TestFlags::capacity());           // 120 -> 128 (multiple of 64)
	EXPECT_EQ(64u, (BitsetFlags<EAutoFlag>::capacity()));
}

// ---------------------------------------------------------------------------
// Core typed operations
// ---------------------------------------------------------------------------

TEST(BitsetFlags, DefaultConstruction_IsEmpty) {
	TestFlags f;
	EXPECT_TRUE(f.empty());
	EXPECT_TRUE(f.none());
	EXPECT_FALSE(f.any());
	EXPECT_EQ(0u, f.count());
}

TEST(BitsetFlags, SetTestUnset) {
	TestFlags f;
	f.set(ETestFlag::kB);
	EXPECT_TRUE(f.test(ETestFlag::kB));
	EXPECT_TRUE(f.get(ETestFlag::kB));
	EXPECT_FALSE(f.test(ETestFlag::kA));
	EXPECT_FALSE(f.empty());
	f.unset(ETestFlag::kB);
	EXPECT_FALSE(f.test(ETestFlag::kB));
	EXPECT_TRUE(f.empty());
}

TEST(BitsetFlags, Toggle) {
	TestFlags f;
	EXPECT_TRUE(f.toggle(ETestFlag::kMid));   // returns new state
	EXPECT_TRUE(f.test(ETestFlag::kMid));
	EXPECT_FALSE(f.toggle(ETestFlag::kMid));
	EXPECT_FALSE(f.test(ETestFlag::kMid));
}

TEST(BitsetFlags, InitializerListConstruction) {
	const TestFlags f{ETestFlag::kA, ETestFlag::kTop};
	EXPECT_TRUE(f.test(ETestFlag::kA));
	EXPECT_TRUE(f.test(ETestFlag::kTop));
	EXPECT_FALSE(f.test(ETestFlag::kB));
	EXPECT_EQ(2u, f.count());
}

TEST(BitsetFlags, ClearAndSetAll) {
	TestFlags f;
	f.set_all();
	EXPECT_EQ(120u, f.count());      // exactly the logical range, not the 128-bit capacity
	EXPECT_TRUE(f.test(ETestFlag::kTop));
	f.clear();
	EXPECT_TRUE(f.empty());
}

// ---------------------------------------------------------------------------
// Set algebra
// ---------------------------------------------------------------------------

TEST(BitsetFlags, MergeIsUnion) {
	TestFlags a{ETestFlag::kA};
	const TestFlags b{ETestFlag::kB};
	a.merge(b);
	EXPECT_TRUE(a.test(ETestFlag::kA));
	EXPECT_TRUE(a.test(ETestFlag::kB));

	TestFlags c{ETestFlag::kA};
	c += b;   // deprecated alias must behave identically
	EXPECT_EQ(a, c);
}

TEST(BitsetFlags, MaskIsIntersection) {
	TestFlags a{ETestFlag::kA, ETestFlag::kB};
	const TestFlags b{ETestFlag::kB, ETestFlag::kMid};
	a.mask(b);
	EXPECT_FALSE(a.test(ETestFlag::kA));
	EXPECT_TRUE(a.test(ETestFlag::kB));
	EXPECT_FALSE(a.test(ETestFlag::kMid));
}

TEST(BitsetFlags, SubtractRemoves) {
	TestFlags a{ETestFlag::kA, ETestFlag::kB};
	const TestFlags b{ETestFlag::kB};
	a.subtract(b);
	EXPECT_TRUE(a.test(ETestFlag::kA));
	EXPECT_FALSE(a.test(ETestFlag::kB));
}

TEST(BitsetFlags, Intersects) {
	const TestFlags a{ETestFlag::kA, ETestFlag::kB};
	const TestFlags b{ETestFlag::kB, ETestFlag::kTop};
	const TestFlags c{ETestFlag::kMid};
	EXPECT_TRUE(a.intersects(b));
	EXPECT_FALSE(a.intersects(c));
}

TEST(BitsetFlags, EqualityAndInequality) {
	const TestFlags a{ETestFlag::kA, ETestFlag::kMid};
	const TestFlags b{ETestFlag::kA, ETestFlag::kMid};
	const TestFlags c{ETestFlag::kA};
	EXPECT_EQ(a, b);
	EXPECT_NE(a, c);
}

// ---------------------------------------------------------------------------
// Raw-index API (used by the name-based / dictionary formats)
// ---------------------------------------------------------------------------

TEST(BitsetFlags, IndexApiMatchesEnum) {
	TestFlags f;
	f.set_index(65);
	EXPECT_TRUE(f.test(ETestFlag::kMid));   // kMid == index 65
	EXPECT_TRUE(f.test_index(65));
	f.unset_index(65);
	EXPECT_FALSE(f.test_index(65));
}

TEST(BitsetFlags, ForEachSet_ReportsIndices) {
	const TestFlags f{ETestFlag::kA, ETestFlag::kMid, ETestFlag::kTop};
	std::vector<std::size_t> seen;
	f.for_each_set([&seen](std::size_t i) { seen.push_back(i); });
	EXPECT_EQ((std::vector<std::size_t>{0, 65, 119}), seen);
}

// ---------------------------------------------------------------------------
// Serialization parity with FlagData (the byte-compatibility obligation)
// ---------------------------------------------------------------------------

TEST(BitsetFlags, Serialization_Empty) { ExpectSameSerialization({}); }
TEST(BitsetFlags, Serialization_Plane0Bit0) { ExpectSameSerialization({0}); }
TEST(BitsetFlags, Serialization_Plane1Bit0) { ExpectSameSerialization({30}); }
TEST(BitsetFlags, Serialization_Plane2Bit5) { ExpectSameSerialization({65}); }
TEST(BitsetFlags, Serialization_Plane3Bit29) { ExpectSameSerialization({119}); }
TEST(BitsetFlags, Serialization_MultiPlane) { ExpectSameSerialization({0, 1, 31, 65, 90, 119}); }
TEST(BitsetFlags, Serialization_AllBits) {
	std::vector<int> all;
	for (int i = 0; i < 120; ++i) {
		all.push_back(i);
	}
	const FlagData fd = [&] {
		FlagData f;
		for (int n : all) {
			f.set(flag_data_by_num(n));
		}
		return f;
	}();
	TestFlags bf;
	for (int n : all) {
		bf.set_index(static_cast<std::size_t>(n));
	}
	EXPECT_EQ(fd.to_numeric_string(), bf.to_numeric_string());
}

TEST(BitsetFlags, NumericString_KnownValues) {
	EXPECT_EQ("0 0 0 0", TestFlags{}.to_numeric_string());
	EXPECT_EQ("1 0 0 0", MakeBitset({0}).to_numeric_string());
	EXPECT_EQ("1 2 3 4", MakeBitset({0, 31, 60, 61, 92}).to_numeric_string());
}

// ---------------------------------------------------------------------------
// Parse parity (legacy ASCII + numeric + single packed number)
// ---------------------------------------------------------------------------

TEST(BitsetFlags, Parse_LegacyAscii_SingleLower) { ExpectSameParse("a"); }
TEST(BitsetFlags, Parse_LegacyAscii_ExplicitPlane) { ExpectSameParse("a1"); }
TEST(BitsetFlags, Parse_LegacyAscii_Multiple) { ExpectSameParse("abk"); }
TEST(BitsetFlags, Parse_LegacyAscii_Upper) { ExpectSameParse("A0"); }
TEST(BitsetFlags, Parse_Numeric) { ExpectSameParse("42 0 999 0"); }
TEST(BitsetFlags, Parse_NumericAllPlanes) { ExpectSameParse("1 2 3 4"); }
TEST(BitsetFlags, Parse_NumericZeros) { ExpectSameParse("0 0 0 0"); }
TEST(BitsetFlags, Parse_LegacyEmpty) { ExpectSameParse("0 "); }
TEST(BitsetFlags, Parse_Empty) { ExpectSameParse(""); }
TEST(BitsetFlags, Parse_SinglePackedNumber_Plane1) { ExpectSameParse("1073741825"); }  // kIntOne | 1

TEST(BitsetFlags, Parse_RoundTripThroughNumeric) {
	TestFlags original{ETestFlag::kA, ETestFlag::kMid, ETestFlag::kTop};
	TestFlags restored;
	restored.from_string(original.to_numeric_string().c_str());
	EXPECT_EQ(original, restored);
}

TEST(BitsetFlags, Parse_RoundTripThroughTascii) {
	TestFlags original{ETestFlag::kB, ETestFlag::kMid};
	char buf[256] = {0};
	original.tascii(4, buf, sizeof(buf));
	TestFlags restored;
	restored.from_string(buf);
	EXPECT_EQ(original, restored);
}

// ---------------------------------------------------------------------------
// Overflow / reserve safety net (item 8)
// ---------------------------------------------------------------------------

TEST(BitsetFlags, Overflow_WithinCapacity_IsKeptAndPersisted) {
	TestFlags f;
	f.set_index(125);                 // in [120, 128): reserve slack -> kept (and logged once)
	EXPECT_TRUE(f.test_index(125));
	// content-aware writer must still persist it (5 planes), so a round-trip restores it.
	TestFlags restored;
	restored.from_string(f.to_numeric_string().c_str());
	EXPECT_TRUE(restored.test_index(125));
}

TEST(BitsetFlags, Overflow_BeyondCapacity_IsRefused) {
	TestFlags f;
	f.set_index(200);                 // >= capacity (128): dropped, never UB
	EXPECT_FALSE(f.test_index(200));
	EXPECT_TRUE(f.empty());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
