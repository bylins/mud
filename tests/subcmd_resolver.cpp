// issue.specials: SubCmdResolver maps single-word subcommand synonyms to an id + handler, resolving a
// typed word by exact match, then by unique abbreviation (prefix), rejecting ambiguous prefixes; and it
// generates the usage tooltip from the synonyms. ASCII-only fixture (the logic is encoding-agnostic).

#include <gtest/gtest.h>

#include "gameplay/ai/subcmd_resolver.h"

namespace {
int dummy(CharData *, void *, char *) { return 0; }

SubCmdResolver MakeResolver() {
	return SubCmdResolver("Greeting?", {
		{{"balance", "saldo"}, 0, dummy},   // two synonyms -> same id
		{{"deposit"},          1, dummy},
		{{"sell"},             2, dummy},
		{{"send"},             3, dummy},   // shares the "se" prefix with "sell"
	});
}
}  // namespace

TEST(SubCmdResolver, ExactMatchWins) {
	const auto r = MakeResolver();
	bool ambiguous = true;
	EXPECT_EQ(r.Match("balance", ambiguous), 0);
	EXPECT_FALSE(ambiguous);
	// exact "send" must win even though "sen"/"se" overlaps "sell"
	EXPECT_EQ(r.Match("send", ambiguous), 3);
	EXPECT_FALSE(ambiguous);
}

TEST(SubCmdResolver, SynonymsShareId) {
	const auto r = MakeResolver();
	bool ambiguous = false;
	EXPECT_EQ(r.Match("saldo", ambiguous), 0);
	EXPECT_EQ(r.Match("balance", ambiguous), 0);
}

TEST(SubCmdResolver, UniqueAbbreviation) {
	const auto r = MakeResolver();
	bool ambiguous = true;
	EXPECT_EQ(r.Match("dep", ambiguous), 1);
	EXPECT_FALSE(ambiguous);
	EXPECT_EQ(r.Match("sel", ambiguous), 2);  // "sel" is unique to "sell"
	EXPECT_FALSE(ambiguous);
}

TEST(SubCmdResolver, AmbiguousPrefixRejected) {
	const auto r = MakeResolver();
	bool ambiguous = false;
	EXPECT_EQ(r.Match("se", ambiguous), -1);  // matches both "sell" and "send"
	EXPECT_TRUE(ambiguous);
}

TEST(SubCmdResolver, UnknownWord) {
	const auto r = MakeResolver();
	bool ambiguous = true;
	EXPECT_EQ(r.Match("xyz", ambiguous), -1);
	EXPECT_FALSE(ambiguous);
}

TEST(SubCmdResolver, CaseInsensitive) {
	const auto r = MakeResolver();
	bool ambiguous = false;
	EXPECT_EQ(r.Match("BALANCE", ambiguous), 0);
	EXPECT_EQ(r.Match("Dep", ambiguous), 1);
}

TEST(SubCmdResolver, TooltipFromPrimarySynonyms) {
	const auto r = MakeResolver();
	EXPECT_EQ(r.Tooltip(), "(balance, deposit, sell, send)");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
