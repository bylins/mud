// issue.sml-cdata-msg: a <message> may carry its text either in the "val" attribute (the
// common case) or, for multi-line preformatted text, as element inner text -- plain PCDATA or
// a <![CDATA[...]]> block -- with "val" omitted. MsgSheafBuilder selects "val" if present, else
// the element's inner text (DataNode::GetValue("")), read verbatim. These tests pin that
// behaviour at the DataNode level the builder relies on.

#include <gtest/gtest.h>

#include "utils/parser_wrapper.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace {
// Mirrors MsgSheafBuilder::Build's text selection (src/engine/structs/msg_container.h):
// "val" attribute if non-empty, otherwise the element's inner text (PCDATA/CDATA).
std::string ReadMsgText(const parser_wrapper::DataNode &m) {
	const char *v = m.GetValue("val");
	return (v && *v) ? std::string(v) : std::string(m.GetValue(""));
}
}  // namespace

TEST(MsgContainer_Cdata, AttrPcdataAndMultilineCdata) {
	const char *src = "msg_cdata_src.xml";
	{
		std::ofstream f(src);
		f << "<root>\n"
			 "  <msg_sheaf id=\"s\">\n"
			 "    <message type=\"attr\" val=\"from attribute\"/>\n"
			 "    <message type=\"pcdata\">plain inner text</message>\n"
			 "    <message type=\"cdata\"><![CDATA[line 1\nline 2\nline 3]]></message>\n"
			 "  </msg_sheaf>\n"
			 "</root>\n";
	}

	parser_wrapper::DataNode doc(src);
	parser_wrapper::DataNode sheaf;
	bool found = false;
	for (auto &c : doc.Children()) {
		if (std::string(c.GetValue("id")) == "s") { sheaf = c; found = true; break; }
	}
	ASSERT_TRUE(found);

	std::string attr, pcdata, cdata;
	for (auto &m : sheaf.Children("message")) {
		const std::string t = m.GetValue("type");
		if (t == "attr") attr = ReadMsgText(m);
		else if (t == "pcdata") pcdata = ReadMsgText(m);
		else if (t == "cdata") cdata = ReadMsgText(m);
	}

	EXPECT_EQ(attr, "from attribute");
	EXPECT_EQ(pcdata, "plain inner text");
	// Multi-line CDATA content is preserved verbatim, including the internal newlines.
	EXPECT_EQ(cdata, "line 1\nline 2\nline 3");

	std::remove(src);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
