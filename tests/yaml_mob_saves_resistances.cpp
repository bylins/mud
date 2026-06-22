// Part of Bylins http://www.mud.ru
// Tests for the named-map serialization of mob saves and resistances.
//
// The YAML mob format stores saving throws and resistances as named maps
// (e.g. "kWill: 10", "kFire: 20") instead of positional integer arrays, so the
// meaning of each value is explicit and the on-disk order is decoupled from the
// ESaving / EResist enum order. These tests pin:
//   * the name<->index mapping the loader relies on (NAME_BY_ITEM/ITEM_BY_NAME);
//   * order-independence of map parsing;
//   * the "missing key means 0" / "emit non-zero only" rule, including omitting
//     the whole block when every value is zero;
//   * graceful skipping of unknown keys and the ESaving::kNone sentinel;
//   * the deprecated positional-list backward-compatibility path.
//
// The Parse*/Emit* helpers below mirror the reader in
// YamlWorldDataSource::ParseMobFile and the writer in
// YamlWorldDataSource::SaveMobs; they use the real enum mapping functions and
// emitter so a divergence in either is caught here.
//
// Guarded by HAVE_YAML: yaml-cpp is only available when the YAML world format
// is enabled, so in non-YAML builds this file compiles to nothing (same pattern
// as koi8r.yaml.emitter.cpp).

#ifdef HAVE_YAML

#include "engine/entities/entities_constants.h"
#include "engine/db/koi8r_yaml_emitter.h"
#include "engine/structs/structs.h"

#include <yaml-cpp/yaml.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

constexpr std::size_t kNumResist = EResist::kLastResist - EResist::kFirstResist + 1;
constexpr std::size_t kNumSaves =
	static_cast<int>(ESaving::kLast) - static_cast<int>(ESaving::kFirst) + 1;

std::array<int, kNumResist> ParseResistances(const YAML::Node &node) {
	std::array<int, kNumResist> out{};
	if (node.IsMap()) {
		for (const auto &kv : node) {
			try {
				const auto r = ITEM_BY_NAME<EResist>(kv.first.as<std::string>());
				out[r] = std::clamp(kv.second.as<int>(), kMinResistance, kMaxNpcResist);
			} catch (const std::out_of_range &) {
			}
		}
	} else if (node.IsSequence()) {
		int idx = 0;
		for (const auto &v : node) {
			if (idx < static_cast<int>(out.size())) {
				out[idx] = std::clamp(v.as<int>(), kMinResistance, kMaxNpcResist);
			}
			idx++;
		}
	}
	return out;
}

std::array<int, kNumSaves> ParseSaves(const YAML::Node &node) {
	std::array<int, kNumSaves> out{};
	if (node.IsMap()) {
		for (const auto &kv : node) {
			try {
				const int s = static_cast<int>(ITEM_BY_NAME<ESaving>(kv.first.as<std::string>()));
				if (s >= 0 && s < static_cast<int>(out.size())) {
					out[s] = std::clamp(kv.second.as<int>(), kMinSaving, kMaxSaving);
				}
			} catch (const std::out_of_range &) {
			}
		}
	} else if (node.IsSequence()) {
		int idx = 0;
		for (const auto &v : node) {
			if (idx < static_cast<int>(out.size())) {
				out[idx] = std::clamp(v.as<int>(), kMinSaving, kMaxSaving);
			}
			idx++;
		}
	}
	return out;
}

std::string EmitResistances(const std::array<int, kNumResist> &vals) {
	bool any = false;
	for (const int v : vals) {
		if (v != 0) {
			any = true;
			break;
		}
	}
	std::ostringstream oss;
	if (any) {
		Koi8rYamlEmitter yaml(oss);
		yaml.Key("resistances");
		yaml.BeginBlock();
		for (std::size_t i = 0; i < vals.size(); ++i) {
			if (vals[i] != 0) {
				yaml.Key(NAME_BY_ITEM<EResist>(static_cast<EResist>(i)));
				yaml.Value(vals[i]);
			}
		}
		yaml.EndBlock();
	}
	return oss.str();
}

}  // namespace

// The writer emits NAME_BY_ITEM(i) and the loader reads it back with
// ITEM_BY_NAME; this round-trip is what keeps the file order independent of the
// enum order. Pinned for every value in both enums.
TEST(MobSavesResistances, EnumNamesRoundTrip) {
	for (int i = EResist::kFirstResist; i <= EResist::kLastResist; ++i) {
		const auto e = static_cast<EResist>(i);
		EXPECT_EQ(e, ITEM_BY_NAME<EResist>(NAME_BY_ITEM<EResist>(e)));
	}
	for (int i = static_cast<int>(ESaving::kFirst); i <= static_cast<int>(ESaving::kLast); ++i) {
		const auto e = static_cast<ESaving>(i);
		EXPECT_EQ(e, ITEM_BY_NAME<ESaving>(NAME_BY_ITEM<ESaving>(e)));
	}
}

TEST(MobSavesResistances, NamedMapMapsToCorrectIndices) {
	const auto r = ParseResistances(YAML::Load("kFire: 20\nkMind: 5\nkDark: -10\n"));
	EXPECT_EQ(20, r[EResist::kFire]);
	EXPECT_EQ(5, r[EResist::kMind]);
	EXPECT_EQ(-10, r[EResist::kDark]);
	// Keys not present default to 0.
	EXPECT_EQ(0, r[EResist::kAir]);
	EXPECT_EQ(0, r[EResist::kWater]);

	const auto s = ParseSaves(YAML::Load("kWill: 7\nkStability: 3\n"));
	EXPECT_EQ(7, s[static_cast<int>(ESaving::kWill)]);
	EXPECT_EQ(3, s[static_cast<int>(ESaving::kStability)]);
	EXPECT_EQ(0, s[static_cast<int>(ESaving::kCritical)]);
}

TEST(MobSavesResistances, MapParsingIsOrderIndependent) {
	const auto a = ParseSaves(YAML::Load("kWill: 1\nkReflex: 2\n"));
	const auto b = ParseSaves(YAML::Load("kReflex: 2\nkWill: 1\n"));
	EXPECT_EQ(a, b);
	EXPECT_EQ(1, a[static_cast<int>(ESaving::kWill)]);
	EXPECT_EQ(2, a[static_cast<int>(ESaving::kReflex)]);
}

// DEPRECATED positional-list format: kept for worlds generated before the named
// map. Remove this path once the new format has settled.
TEST(MobSavesResistances, LegacyPositionalListStillParses) {
	const auto r = ParseResistances(YAML::Load("[1, 2, 3, 4, 5, 6, 7, 8]"));
	for (int i = 0; i < static_cast<int>(kNumResist); ++i) {
		EXPECT_EQ(i + 1, r[i]);
	}
	const auto s = ParseSaves(YAML::Load("[11, 12, 13, 14]"));
	EXPECT_EQ(11, s[0]);
	EXPECT_EQ(14, s[3]);
}

TEST(MobSavesResistances, UnknownKeyIsSkipped) {
	const auto r = ParseResistances(YAML::Load("kFire: 7\nkBogus: 99\n"));
	EXPECT_EQ(7, r[EResist::kFire]);
	EXPECT_EQ(0, r[EResist::kAir]);
}

// kNone is the ESaving sentinel; its index is past the array, so it must be
// ignored rather than writing out of bounds.
TEST(MobSavesResistances, SaveSentinelKeyIsSkipped) {
	const auto s = ParseSaves(YAML::Load("kWill: 3\nkNone: 99\n"));
	EXPECT_EQ(3, s[static_cast<int>(ESaving::kWill)]);
}

TEST(MobSavesResistances, ValuesAreClamped) {
	const auto r = ParseResistances(YAML::Load("kFire: 9999\nkAir: -9999\n"));
	EXPECT_EQ(kMaxNpcResist, r[EResist::kFire]);
	EXPECT_EQ(kMinResistance, r[EResist::kAir]);

	const auto s = ParseSaves(YAML::Load("kWill: 9999\nkReflex: -9999\n"));
	EXPECT_EQ(kMaxSaving, s[static_cast<int>(ESaving::kWill)]);
	EXPECT_EQ(kMinSaving, s[static_cast<int>(ESaving::kReflex)]);
}

TEST(MobSavesResistances, EmitSkipsZerosAndRoundTrips) {
	std::array<int, kNumResist> vals{};
	vals[EResist::kFire] = 20;
	vals[EResist::kMind] = -5;

	const std::string text = EmitResistances(vals);
	EXPECT_NE(std::string::npos, text.find("kFire"));
	EXPECT_NE(std::string::npos, text.find("kMind"));
	// Zero-valued resistances are not written.
	EXPECT_EQ(std::string::npos, text.find("kAir"));
	EXPECT_EQ(std::string::npos, text.find("kWater"));

	const YAML::Node root = YAML::Load(text);
	EXPECT_EQ(vals, ParseResistances(root["resistances"]));
}

TEST(MobSavesResistances, EmitOmitsBlockWhenAllZero) {
	const std::array<int, kNumResist> vals{};
	EXPECT_TRUE(EmitResistances(vals).empty());
}

#endif  // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
