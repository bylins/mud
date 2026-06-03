// issue.vedun-editor Phase 2: the runtime enum-by-type-name registry (string -> members), which
// the scheme layer uses to build numbered enum pick-lists from a `enum<EElement>` declaration.

#include <gtest/gtest.h>

#include "engine/olc/vedun/enum_registry.h"

TEST(Vedun_EnumRegistry, ResolvesByTypeNameAndRoundTrips) {
	vedun::RegisterEditorEnums();
	const auto &reg = vedun::EnumRegistry::Instance();

	EXPECT_TRUE(reg.Known("EElement"));
	EXPECT_FALSE(reg.Known("NoSuchEnum"));

	const auto *members = reg.Members("EElement");
	ASSERT_NE(members, nullptr);
	EXPECT_FALSE(members->empty());

	const auto value = reg.ValueOf("EElement", "kLight");
	ASSERT_TRUE(value.has_value());
	const auto *name = reg.NameOf("EElement", *value);
	ASSERT_NE(name, nullptr);
	EXPECT_EQ(*name, "kLight");
}

TEST(Vedun_EnumRegistry, UnknownTypeAndMember) {
	vedun::RegisterEditorEnums();
	const auto &reg = vedun::EnumRegistry::Instance();

	EXPECT_EQ(reg.Members("Bogus"), nullptr);
	EXPECT_FALSE(reg.ValueOf("EElement", "kNotARealMember").has_value());
	EXPECT_EQ(reg.NameOf("EElement", 999999), nullptr);
}

TEST(Vedun_EnumRegistry, SpellSchemeEnumsRegistered) {
	vedun::RegisterEditorEnums();
	const auto &reg = vedun::EnumRegistry::Instance();
	for (const char *e : {"ESpell", "EElement", "EMagic", "ETarget", "EPosition", "EItemMode",
						  "EAffFlag", "ESkill", "EBaseStat"}) {
		EXPECT_TRUE(reg.Known(e)) << e << " should be registered for the spell scheme";
	}
	// EAffFlag drives the <unaffect affect_flags=...> pick-list; ESkill/EBaseStat the potency roll's
	// base_skill/base_stat ids; spot-check a known member of each.
	EXPECT_TRUE(reg.ValueOf("EAffFlag", "kAfDispellable").has_value());
	EXPECT_TRUE(reg.ValueOf("EBaseStat", "kWis").has_value());
}
