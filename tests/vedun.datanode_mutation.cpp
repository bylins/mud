// issue.vedun-editor Phase 3: DataNode mutation (SetValue) + Save, and the shared-document semantics
// the editor's edit->save flow relies on (a copied node edits the same document the root saves).

#include <gtest/gtest.h>

#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>
#include <string>

TEST(Vedun_DataNodeMutation, SetValueAndSaveRoundTrip) {
	const char *src = "vedun_mut_src.xml";
	const char *out = "vedun_mut_out.xml";
	{
		std::ofstream f(src);
		f << R"(<root><item id="a" v="1"/><item id="b" v="2"/></root>)";
	}

	parser_wrapper::DataNode doc(src);
	// A copy of a child shares the same document as `doc` (the edit must be visible through `doc`).
	parser_wrapper::DataNode target;
	bool located = false;
	for (auto &child : doc.Children()) {
		if (std::string(child.GetValue("id")) == "b") {
			target = child;
			located = true;
			break;
		}
	}
	ASSERT_TRUE(located);

	EXPECT_TRUE(target.SetValue("v", "99"));        // existing attribute
	EXPECT_TRUE(target.SetValue("note", "added"));  // new attribute
	ASSERT_TRUE(doc.Save(out));                     // saving the root persists the edit on the copy

	parser_wrapper::DataNode reloaded(out);
	bool verified = false;
	for (auto &child : reloaded.Children()) {
		if (std::string(child.GetValue("id")) == "b") {
			EXPECT_STREQ(child.GetValue("v"), "99");
			EXPECT_STREQ(child.GetValue("note"), "added");
			verified = true;
		}
		if (std::string(child.GetValue("id")) == "a") {
			EXPECT_STREQ(child.GetValue("v"), "1");  // untouched sibling preserved
		}
	}
	EXPECT_TRUE(verified);

	std::remove(src);
	std::remove(out);
}

TEST(Vedun_DataNodeMutation, AddAndRemoveChildRoundTrip) {
	const char *src = "vedun_child_src.xml";
	const char *out = "vedun_child_out.xml";
	{
		std::ofstream f(src);
		f << R"(<root><spell id="s"><components><verbal/><weave/></components></spell></root>)";
	}
	parser_wrapper::DataNode doc(src);
	// Navigate to <components>.
	parser_wrapper::DataNode components;
	for (auto &spell : doc.Children()) {
		for (auto &comp : spell.Children()) {
			if (std::string(comp.GetName()) == "components") {
				components = comp;
			}
		}
	}
	ASSERT_TRUE(components.IsNotEmpty());

	// Add a new child, and remove an existing one (delete <verbal>).
	parser_wrapper::DataNode added = components.AddChild("material");
	ASSERT_TRUE(added.IsNotEmpty());
	EXPECT_TRUE(added.SetValue("any_of", "1930"));
	for (auto &comp : components.Children()) {
		if (std::string(comp.GetName()) == "verbal") {
			EXPECT_TRUE(components.RemoveChild(comp));
			break;
		}
	}

	ASSERT_TRUE(doc.Save(out));
	parser_wrapper::DataNode reloaded(out);
	bool has_verbal = false, has_weave = false, has_material = false;
	std::string material_any;
	for (auto &spell : reloaded.Children()) {
		for (auto &comp : spell.Children()) {
			for (auto &c : comp.Children()) {
				const std::string n = c.GetName();
				if (n == "verbal") { has_verbal = true; }
				if (n == "weave") { has_weave = true; }
				if (n == "material") { has_material = true; material_any = c.GetValue("any_of"); }
			}
		}
	}
	EXPECT_FALSE(has_verbal);   // removed
	EXPECT_TRUE(has_weave);     // untouched
	EXPECT_TRUE(has_material);  // added
	EXPECT_EQ(material_any, "1930");

	std::remove(src);
	std::remove(out);
}
