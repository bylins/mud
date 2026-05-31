// Part of Bylins http://www.mud.ru
// Golden tests for BufferedFileWriter: its printf must produce byte-identical
// output to snprintf for the same format/args (so converting pfile fprintf to
// it keeps the on-disk bytes and the CRC stable), and it must handle fields of
// any length without truncation or overflow (the reason we use vsnprintf rather
// than the legacy fbprintf/vsprintf).

#include "utils/buffered_file_writer.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

namespace {

TEST(BufferedFileWriter, MatchesSnprintfForMixedFormats) {
	BufferedFileWriter w;
	w.printf("Trcg: %d\n", 1234);
	w.printf("Name: %s~\n", "test");
	w.printf("Nums: %lld %lu %.2f\n", static_cast<long long>(-5), static_cast<unsigned long>(7), 3.14159);

	char buf[256];
	std::string expect;
	snprintf(buf, sizeof(buf), "Trcg: %d\n", 1234);
	expect += buf;
	snprintf(buf, sizeof(buf), "Name: %s~\n", "test");
	expect += buf;
	snprintf(buf, sizeof(buf), "Nums: %lld %lu %.2f\n",
		static_cast<long long>(-5), static_cast<unsigned long>(7), 3.14159);
	expect += buf;

	EXPECT_EQ(w.str(), expect);
}

TEST(BufferedFileWriter, HandlesLongFieldWithoutTruncation) {
	// Single field far larger than any internal start size -- must not overflow
	// or truncate (legacy fbprintf/vsprintf would have been unsafe here).
	const std::string big(100000, 'x');
	BufferedFileWriter w;
	w.printf("%s", big.c_str());
	EXPECT_EQ(w.str().size(), big.size());
	EXPECT_EQ(w.str(), big);
}

TEST(BufferedFileWriter, EmptyThenAppends) {
	BufferedFileWriter w;
	EXPECT_TRUE(w.str().empty());
	w.printf("a");
	w.printf("b%d", 2);
	w.printf("");  // empty format must be a no-op
	EXPECT_EQ(w.str(), "ab2");
}

TEST(BufferedFileWriter, PreservesEmbeddedFormatlessText) {
	BufferedFileWriter w;
	w.printf("plain line with 100%% and ~ tilde\n");
	char buf[128];
	snprintf(buf, sizeof(buf), "plain line with 100%% and ~ tilde\n");
	EXPECT_EQ(w.str(), std::string(buf));
}

}  // namespace

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
