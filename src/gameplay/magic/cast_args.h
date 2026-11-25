/**
\file cast_args.h - a part of the Bylins engine.
\brief The words a caster typed after a spell's name (issue #2689).
\details "колдовать !оживить скелет! труп яростный" -> words {"труп", "яростный"}, target_words 1:
		 "труп" named the target, "яростный" is the spell's own option (the skeleton kind). A spell
		 that ignores targets ("колдовать !X! перун") gets {"перун"} with target_words 0. Whether and
		 how to use the words is up to the spell; most read only AfterTarget(). Casts that come from
		 items, triggers or mobs carry an empty CastArgs.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MAGIC_CAST_ARGS_H_
#define BYLINS_SRC_GAMEPLAY_MAGIC_CAST_ARGS_H_

#include <cstddef>
#include <string>
#include <vector>

struct CastArgs {
	// Every word after the spell's name, in order, as typed.
	std::vector<std::string> words;
	// How many leading words named the target: 1 when the target (a character, an object, a direction)
	// was found by name, 0 when the spell ignores targets or picked one by default.
	std::size_t target_words{0};

	// The words after the target -- the spell's own options.
	[[nodiscard]] std::vector<std::string> AfterTarget() const {
		if (target_words >= words.size()) {
			return {};
		}
		return {words.begin() + static_cast<std::ptrdiff_t>(target_words), words.end()};
	}

	// AfterTarget() joined by single spaces ("пресвятой богородице"); empty if there are none.
	[[nodiscard]] std::string AfterTargetText() const {
		std::string text;
		for (const auto &word : AfterTarget()) {
			if (!text.empty()) {
				text += ' ';
			}
			text += word;
		}
		return text;
	}
};

#endif // BYLINS_SRC_GAMEPLAY_MAGIC_CAST_ARGS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
