#include <gtest/gtest.h>

#include "engine/observability/file_event_sink.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ReadFile(const std::string& path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

std::vector<std::string> SplitLines(const std::string& blob) {
	std::vector<std::string> out;
	std::string cur;
	for (char c : blob) {
		if (c == '\n') {
			out.push_back(cur);
			cur.clear();
		} else {
			cur.push_back(c);
		}
	}
	if (!cur.empty()) {
		out.push_back(cur);
	}
	return out;
}

std::string TempPath(const char* suffix) {
	return std::string("/tmp/file_event_sink_test_") + suffix + ".jsonl";
}

}  // namespace

TEST(FileEventSink, WritesBaseFieldsAsJsonLine) {
	const auto path = TempPath("base");
	std::remove(path.c_str());
	{
		observability::FileEventSink sink(path);
		observability::Event e;
		e.name = "hit";
		e.ts_unix_ms = 1714400000000;
		sink.Emit(e);
	}
	const auto lines = SplitLines(ReadFile(path));
	ASSERT_EQ(lines.size(), 1u);
	EXPECT_EQ(lines[0], R"({"ts":1714400000000,"name":"hit"})");
	std::remove(path.c_str());
}

TEST(FileEventSink, EncodesAttributeTypes) {
	const auto path = TempPath("attrs");
	std::remove(path.c_str());
	{
		observability::FileEventSink sink(path);
		observability::Event e;
		e.name = "hit";
		e.ts_unix_ms = 1;
		e.attrs["i"] = static_cast<std::int64_t>(42);
		e.attrs["b"] = true;
		e.attrs["s"] = std::string("sorcerer");
		e.attrs["d"] = 3.5;
		sink.Emit(e);
	}
	const auto lines = SplitLines(ReadFile(path));
	ASSERT_EQ(lines.size(), 1u);
	// Order is enforced by FileEventSink: ts, name, then attributes sorted
	// alphabetically (b, d, i, s).
	EXPECT_EQ(lines[0], R"({"ts":1,"name":"hit","b":true,"d":3.5,"i":42,"s":"sorcerer"})");
	std::remove(path.c_str());
}

TEST(FileEventSink, EscapesSpecialCharsInStrings) {
	const auto path = TempPath("escape");
	std::remove(path.c_str());
	{
		observability::FileEventSink sink(path);
		observability::Event e;
		e.name = "test";
		e.ts_unix_ms = 0;
		e.attrs["s"] = std::string("line1\nline2\t\"quoted\"\\back");
		sink.Emit(e);
	}
	const auto lines = SplitLines(ReadFile(path));
	ASSERT_EQ(lines.size(), 1u);
	EXPECT_NE(lines[0].find(R"("s":"line1\nline2\t\"quoted\"\\back")"), std::string::npos);
	std::remove(path.c_str());
}

TEST(FileEventSink, AppendsManyEventsAsSeparateLines) {
	const auto path = TempPath("many");
	std::remove(path.c_str());
	{
		observability::FileEventSink sink(path);
		for (int i = 0; i < 10; ++i) {
			observability::Event e;
			e.name = "hit";
			e.ts_unix_ms = i;
			e.attrs["round"] = static_cast<std::int64_t>(i);
			sink.Emit(e);
		}
	}
	const auto lines = SplitLines(ReadFile(path));
	EXPECT_EQ(lines.size(), 10u);
	std::remove(path.c_str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
