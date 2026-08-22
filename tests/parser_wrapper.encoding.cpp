// The load/save cycle of an XML config must not change what it holds (issue #3681).
//
// This pins a bug that cost a world: the document was brought into the native encoding once in
// DataNode, and then AGAIN per field in parse::AttrStr. Under KOI8-R both conversions are the
// identity, so nothing showed. Under UTF-8 every field was converted twice -- and because the
// engine writes configs back (ObjSetsLoader::Load normalises the file through save()), the damage
// accumulated: cfg/mechanics/obj_sets.xml doubled on every boot until, at 6.7 GB, reading it into
// memory killed the process.
//
// Pure ASCII: the Cyrillic fixture is spelled as UTF-8 byte escapes and brought into the native
// encoding through native_text, so the same test is correct under either build.

#include "utils/parser_wrapper.h"
#include "utils/utils_parse.h"
#include "utils/native_text.h"
#include "utils/translit_koi8.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

// "privet, mir" in UTF-8, plus an ampersand: colour codes in real configs are written "&W", and
// the ampersand is the character XML escaping is most likely to mangle on a re-save.
const char *const kCyrillicUtf8 =
	"&W" "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82" ", " "\xD0\xBC\xD0\xB8\xD1\x80" "&n";

std::string ReadWhole(const std::filesystem::path &path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

// A scratch file removed on destruction, so a failing assertion cannot leave litter behind.
class TempFile {
 public:
	explicit TempFile(const char *name) : path_(std::filesystem::temp_directory_path() / name) {}
	~TempFile() { std::error_code ec; std::filesystem::remove(path_, ec); }
	const std::filesystem::path &path() const { return path_; }

 private:
	std::filesystem::path path_;
};

}  // namespace

TEST(ParserWrapperEncoding, SaveLoadSaveIsByteIdentical) {
	const std::string text = native_text::from_koi8(codepages::Utf8ToKoi8(kCyrillicUtf8));
	TempFile file("bylins_cfg_roundtrip.xml");

	{
		auto doc = parser_wrapper::DataNode::NewDocument();
		auto root = doc.AddChild("obj_sets");
		auto set = root.AddChild("set");
		set.SetValue("name", text);
		ASSERT_TRUE(doc.Save(file.path()));
	}
	const std::string first = ReadWhole(file.path());
	ASSERT_FALSE(first.empty());

	// Read it back the way the loaders do and write it out again from what we read.
	{
		parser_wrapper::DataNode loaded(file.path());
		ASSERT_TRUE(loaded.IsNotEmpty());
		ASSERT_TRUE(loaded.GoToChild("set"));
		// The value must come back exactly as it went in -- this is where the double conversion
		// showed up first.
		EXPECT_EQ(parse::AttrStr(loaded, "name"), text);

		auto doc = parser_wrapper::DataNode::NewDocument();
		auto root = doc.AddChild("obj_sets");
		auto set = root.AddChild("set");
		set.SetValue("name", parse::AttrStr(loaded, "name"));
		ASSERT_TRUE(doc.Save(file.path()));
	}
	const std::string second = ReadWhole(file.path());

	EXPECT_EQ(first.size(), second.size()) << "the file grew on a load/save cycle";
	EXPECT_EQ(first, second) << "a load/save cycle must reproduce the file byte for byte";
}

TEST(ParserWrapperEncoding, ReadsAConfigStoredInKoi8) {
	// Configs on disk are still KOI8-R, so reading one must transcode; and the result, once
	// written back, must then stay put (covered by the test above).
	TempFile file("bylins_cfg_koi8.xml");
	{
		std::ofstream out(file.path(), std::ios::binary);
		out << "<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n<obj_sets><set name=\""
			<< codepages::Utf8ToKoi8(kCyrillicUtf8) << "\"/></obj_sets>\n";
	}

	parser_wrapper::DataNode loaded(file.path());
	ASSERT_TRUE(loaded.IsNotEmpty());
	ASSERT_TRUE(loaded.GoToChild("set"));
	EXPECT_EQ(parse::AttrStr(loaded, "name"),
			  native_text::from_koi8(codepages::Utf8ToKoi8(kCyrillicUtf8)));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
