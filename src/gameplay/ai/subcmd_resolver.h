#ifndef BYLINS_SRC_GAMEPLAY_AI_SUBCMD_RESOLVER_H_
#define BYLINS_SRC_GAMEPLAY_AI_SUBCMD_RESOLVER_H_

#include <initializer_list>
#include <string>
#include <vector>

class CharData;

// issue.specials: a small per-special-procedure subcommand dispatcher. Each row maps a list of
// single-word synonyms (e.g. {"баланс", "сальдо"}) to a subcommand id (a per-proc enum value, stored
// as int) and its handler. A typed word resolves by exact match, else by UNIQUE abbreviation (prefix);
// an ambiguous prefix is rejected. The usage tooltip is GENERATED from the synonyms (names[0] of each
// row), so the hint can never drift from the actual subcommands. Strings live in code for now; later
// they move to special_msg.xml and this table is built from there -- the API stays the same.
//
// Lookup is currently a linear, case-insensitive scan (str_cmp/strn_cmp, KOI8-aware) -- correct and
// cheap for the handful of subcommands a spec proc has; the same API can be backed by a shared
// prefix-tree later (and reused for socials/command abbreviation).
class SubCmdResolver {
 public:
	// rest = the argument text after the subcommand word; me = the carrier (mob/obj), as for spec procs.
	using Handler = int (*)(CharData *ch, void *me, char *rest);

	struct Row {
		std::vector<std::string> names;  // synonyms; names[0] is the primary shown in the tooltip
		int id;                          // the per-proc subcommand enum value (clarity / future XML key)
		Handler handler;
	};

	SubCmdResolver(std::string greeting, std::initializer_list<Row> rows);

	// Always "handles" the input (returns the handler result, or 1 for the framework replies):
	// empty word -> greeting + tooltip; unknown/ambiguous -> a clarifying reply; else dispatches.
	int Dispatch(CharData *ch, void *me, char *argument) const;

	[[nodiscard]] std::string Tooltip() const;  // "(баланс, вложить, получить, ...)"

	// Resolve a typed word to its row id (the enum value), or -1 if unknown; sets `ambiguous` when the
	// word is a prefix of more than one subcommand. Exposed so Dispatch's resolution can be unit-tested.
	[[nodiscard]] int Match(const char *word, bool &ambiguous) const;

 private:
	[[nodiscard]] const Row *Resolve(const char *word, bool &ambiguous) const;

	std::string greeting_;
	std::vector<Row> rows_;
};

#endif  // BYLINS_SRC_GAMEPLAY_AI_SUBCMD_RESOLVER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
