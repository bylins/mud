// Part of Bylins http://www.mud.ru
// Regression tests for YAML/SQLite loader flag packing.
//
// Background: dictionary lookup returns a *bit index* (0..119). FlagData::set()
// expects a *packed value* -- (plane << 30) | (1 << bit_in_plane). Until this
// branch the YAML loader passed the raw bit index straight into SetFlag/AFF_FLAGS,
// which OR'd the literal integer (e.g. set(13) corrupted bits 0,2,3 instead of
// setting bit 13). Symptoms:
//   stat хозяин постоялого двора ->
//     MOB флаги: спец,!ходит,подбирает,моб,!заколоть,!очар,UNDEF
//   instead of legacy:
//     MOB флаги: !ходит,моб,!очар,!призывать,!усыпить,!убить,UNDEF
//
// These tests reproduce both code paths (the old buggy one and the fixed one)
// and assert their outputs differ in exactly the way we observed.

#include "engine/structs/flag_data.h"
#include "engine/structs/structs.h"

#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

namespace {

// Mirror of the loader's "for each flag name, look up bit index, set on FlagData"
// loop. The two implementations differ only in whether they pack via
// flag_data_by_num before calling FlagData::set().
using DictEntry = std::pair<std::string, long>;
using DictEntries = std::vector<DictEntry>;
using FlagNames = std::vector<std::string>;

void ApplyFixed(FlagData& flags, const DictEntries& dict, const FlagNames& names) {
	for (const auto& name : names) {
		for (const auto& [dict_name, bit_index] : dict) {
			if (dict_name == name) {
				flags.set(static_cast<Bitvector>(flag_data_by_num(static_cast<int>(bit_index))));
				break;
			}
		}
	}
}

void ApplyBuggy(FlagData& flags, const DictEntries& dict, const FlagNames& names) {
	for (const auto& name : names) {
		for (const auto& [dict_name, bit_index] : dict) {
			if (dict_name == name) {
				flags.set(static_cast<Bitvector>(bit_index));   // missing flag_data_by_num()
				break;
			}
		}
	}
}

}  // namespace

// Real-world reproduction: action_flags of mob #4022 ("хозяин постоялого двора").
// Legacy: bits 1, 3, 13, 14, 15, 27 set on plane 0.
TEST(YamlLoaderFlagPacking, MobActionFlagsFixedPathSetsExpectedBits) {
	const DictEntries dict = {
		{"kSentinel", 1}, {"kNpc", 3},
		{"kNoCharm", 13}, {"kNoSummon", 14}, {"kNoSleep", 15},
		{"kProtect", 27},
	};
	const FlagNames file_flags = {
		"kSentinel", "kNpc", "kNoCharm", "kNoSummon", "kNoSleep", "kProtect",
	};

	FlagData flags;
	ApplyFixed(flags, dict, file_flags);

	const Bitvector expected =
		(1u << 1) | (1u << 3) | (1u << 13) | (1u << 14) | (1u << 15) | (1u << 27);
	EXPECT_EQ(expected, flags.get_plane(0));
	EXPECT_EQ(0u, flags.get_plane(1));
	EXPECT_EQ(0u, flags.get_plane(2));
}

// Documents the bug we fixed: the buggy path ORs the raw bit indices, producing
// 1 | 3 | 13 | 14 | 15 | 27 = 31 (bits 0..4) instead of bits 1, 3, 13, 14, 15, 27.
// This test passes only while the buggy semantics are preserved as a literal
// reference -- it's the "negative" we used to confirm the regression.
TEST(YamlLoaderFlagPacking, BuggyPathProducesObservedSymptom) {
	const DictEntries dict = {
		{"kSentinel", 1}, {"kNpc", 3},
		{"kNoCharm", 13}, {"kNoSummon", 14}, {"kNoSleep", 15},
		{"kProtect", 27},
	};
	const FlagNames file_flags = {
		"kSentinel", "kNpc", "kNoCharm", "kNoSummon", "kNoSleep", "kProtect",
	};

	FlagData flags;
	ApplyBuggy(flags, dict, file_flags);

	// Exactly what `stat хозяин` showed under the bug: bits 0..4 set.
	EXPECT_EQ(31u, flags.get_plane(0));
	const Bitvector correct =
		(1u << 1) | (1u << 3) | (1u << 13) | (1u << 14) | (1u << 15) | (1u << 27);
	EXPECT_NE(correct, flags.get_plane(0));
}

// Plane 3 was a separate bug in the loader's local IndexToBitvector helper:
// it returned `1u << idx` for indices >= 90 instead of the canonical
// `kIntThree | (1u << (idx-90))`. Now we delegate to flag_data_by_num().
TEST(YamlLoaderFlagPacking, BitIndexInPlane3PacksWithIntThreeMarker) {
	EXPECT_EQ(static_cast<Bitvector>(1u << 0),  static_cast<Bitvector>(flag_data_by_num(0)));
	EXPECT_EQ(static_cast<Bitvector>(1u << 13), static_cast<Bitvector>(flag_data_by_num(13)));
	EXPECT_EQ(kIntOne | static_cast<Bitvector>(1u << 0), static_cast<Bitvector>(flag_data_by_num(30)));
	EXPECT_EQ(kIntTwo | static_cast<Bitvector>(1u << 0), static_cast<Bitvector>(flag_data_by_num(60)));
	EXPECT_EQ(kIntThree | static_cast<Bitvector>(1u << 0), static_cast<Bitvector>(flag_data_by_num(90)));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
