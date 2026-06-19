// issue.vedun-editor Phase 1: DataNode::Attributes() + element navigation against real spell data.
// The Vedun viewer reflects an element's DOM (its attributes + child tags) into a navigable tree.

#include <gtest/gtest.h>

#include "utils/parser_wrapper.h"

#include <filesystem>
#include <string>

namespace {
const char *kSpellsXml = "small/cfg/spells.xml";  // relative to the test workdir (build/)
}

// Find <spell id="kArmor"> the way do_vedun does, then enumerate its attributes.
TEST(Vedun_Dom, AttributesAndElementFind) {
	if (!std::filesystem::exists(kSpellsXml)) {
		GTEST_SKIP() << "needs the small-world cfg (" << kSpellsXml << ")";
	}
	parser_wrapper::DataNode doc{std::filesystem::path(kSpellsXml)};

	parser_wrapper::DataNode found;
	bool ok = false;
	for (auto &child : doc.Children()) {
		if (std::string(child.GetValue("id")) == "kArmor") {
			found = child;
			ok = true;
			break;
		}
	}
	ASSERT_TRUE(ok) << "kArmor should exist in spells.xml";

	const auto attrs = found.Attributes();
	EXPECT_FALSE(attrs.empty());
	bool has_id = false;
	for (const auto &[name, value] : attrs) {
		if (name == "id") {
			has_id = true;
			EXPECT_EQ(value, "kArmor");
		}
	}
	EXPECT_TRUE(has_id) << "the <spell> node should expose an 'id' attribute";
}

TEST(Vedun_Dom, MissingElementNotFound) {
	if (!std::filesystem::exists(kSpellsXml)) {
		GTEST_SKIP();
	}
	parser_wrapper::DataNode doc{std::filesystem::path(kSpellsXml)};
	bool ok = false;
	for (auto &child : doc.Children()) {
		if (std::string(child.GetValue("id")) == "kNoSuchSpell_XYZ") {
			ok = true;
			break;
		}
	}
	EXPECT_FALSE(ok);
}
