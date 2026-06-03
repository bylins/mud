// issue.vedun-editor Phase 5: the default IEditableCfgLoader::FindElementNode (direct child keyed by
// `id`) the editor uses to locate an element's DOM node, exercised through a dependency-free stub.

#include <gtest/gtest.h>

#include "engine/boot/cfg_manager.h"
#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>

namespace {

// Minimal editable loader that keeps the inherited default FindElementNode.
class StubLoader : public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode) override {}
	void Reload(parser_wrapper::DataNode) override {}
	[[nodiscard]] std::string EditableWhat() const override { return "stub"; }
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const override { return {}; }
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &) const override {
		return {true, ""};
	}
};

} // namespace

TEST(Vedun_ElementFind, DefaultLocatesByIdAndSharesDocument) {
	const char *src = "vedun_find_src.xml";
	{
		std::ofstream f(src);
		f << R"(<root><spell id="kA" v="1"/><spell id="kB" v="2"/></root>)";
	}
	parser_wrapper::DataNode doc(src);
	StubLoader loader;

	parser_wrapper::DataNode hit = loader.FindElementNode(doc, "kB");
	ASSERT_TRUE(hit.IsNotEmpty());
	EXPECT_STREQ(hit.GetValue("v"), "2");

	// The located node shares `doc`'s document: editing it is visible when the root is saved.
	EXPECT_TRUE(hit.SetValue("v", "99"));
	const char *out = "vedun_find_out.xml";
	ASSERT_TRUE(doc.Save(out));
	parser_wrapper::DataNode reloaded(out);
	for (auto &child : reloaded.Children()) {
		if (std::string(child.GetValue("id")) == "kB") {
			EXPECT_STREQ(child.GetValue("v"), "99");
		}
	}
	std::remove(src);
	std::remove(out);
}

TEST(Vedun_ElementFind, DefaultReturnsEmptyForMissing) {
	const char *src = "vedun_find_missing.xml";
	{
		std::ofstream f(src);
		f << R"(<root><spell id="kA" v="1"/></root>)";
	}
	parser_wrapper::DataNode doc(src);
	StubLoader loader;
	EXPECT_TRUE(loader.FindElementNode(doc, "kNope").IsEmpty());
	std::remove(src);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
