// issue.vedun-editor Phase 2: the .scheme XML loader (per-tag grammar + prohibited list).

#include <gtest/gtest.h>

#include "engine/olc/vedun/scheme.h"

#include <cstdio>
#include <fstream>

namespace {
const char *kSchemePath = "vedun_test.scheme";

void WriteSampleScheme() {
	std::ofstream f(kSchemePath);
	f << R"SCHEME(<scheme>
	<prohibited>
		<element id="kUndefined"/>
	</prohibited>
	<tag name="spell" desc="A spell definition">
		<attr name="id" type="enum" enum="ESpell" readonly="Y" desc="identifier"/>
		<attr name="element" type="enum" enum="EElement"/>
		<child tag="misc"/>
		<child tag="mana"/>
	</tag>
	<tag name="misc">
		<attr name="danger" type="int" min="0" max="10" desc="PK danger"/>
		<attr name="violent" type="bool"/>
	</tag>
</scheme>)SCHEME";
}
} // namespace

TEST(Vedun_Scheme, ParsesTagsAttrsAndProhibited) {
	WriteSampleScheme();
	const auto scheme = vedun::LoadScheme(kSchemePath);
	std::remove(kSchemePath);

	EXPECT_FALSE(scheme.Empty());
	EXPECT_TRUE(scheme.IsProhibited("kUndefined"));
	EXPECT_FALSE(scheme.IsProhibited("kArmor"));

	const auto *spell = scheme.FindTag("spell");
	ASSERT_NE(spell, nullptr);
	EXPECT_EQ(spell->desc, "A spell definition");
	EXPECT_EQ(spell->children.size(), 2u);

	const auto *id = spell->FindAttr("id");
	ASSERT_NE(id, nullptr);
	EXPECT_EQ(id->type, vedun::FieldType::kEnum);
	EXPECT_EQ(id->enum_type, "ESpell");
	EXPECT_TRUE(id->readonly);

	const auto *misc = scheme.FindTag("misc");
	ASSERT_NE(misc, nullptr);
	const auto *danger = misc->FindAttr("danger");
	ASSERT_NE(danger, nullptr);
	EXPECT_EQ(danger->type, vedun::FieldType::kInt);
	ASSERT_TRUE(danger->min.has_value());
	EXPECT_EQ(*danger->min, 0);
	ASSERT_TRUE(danger->max.has_value());
	EXPECT_EQ(*danger->max, 10);
	EXPECT_EQ(misc->FindAttr("violent")->type, vedun::FieldType::kBool);
}

TEST(Vedun_Scheme, MissingFileYieldsEmptyScheme) {
	const auto scheme = vedun::LoadScheme("definitely_no_such.scheme");
	EXPECT_TRUE(scheme.Empty());
	EXPECT_EQ(scheme.FindTag("spell"), nullptr);
}

TEST(Vedun_Scheme, SchemePathDerivation) {
	EXPECT_EQ(vedun::SchemePathFor("cfg/spells.xml").string(), "cfg/spells.scheme");
}
