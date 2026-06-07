#include "gameplay/ai/subcmd_resolver.h"

#include "engine/core/comm.h"        // SendMsgToChar
#include "utils/mud_string.h"        // one_argument
#include "utils/utils.h"             // skip_spaces, kMaxInputLength
#include "utils/utils_string.h"      // str_cmp, strn_cmp

#include <cstring>
#include <utility>

SubCmdResolver::SubCmdResolver(std::string greeting, std::initializer_list<Row> rows)
	: greeting_(std::move(greeting)), rows_(rows) {}

std::string SubCmdResolver::Tooltip() const {
	std::string out = "(";
	bool first = true;
	for (const auto &row : rows_) {
		if (row.names.empty()) {
			continue;
		}
		if (!first) {
			out += ", ";
		}
		out += row.names.front();
		first = false;
	}
	out += ")";
	return out;
}

const SubCmdResolver::Row *SubCmdResolver::Resolve(const char *word, bool &ambiguous) const {
	ambiguous = false;
	// 1) exact match wins (case-insensitive), so a full word never loses to a longer synonym.
	for (const auto &row : rows_) {
		for (const auto &name : row.names) {
			if (!str_cmp(word, name.c_str())) {
				return &row;
			}
		}
	}
	// 2) otherwise resolve a UNIQUE abbreviation; matches in several distinct rows are ambiguous.
	const Row *match = nullptr;
	const size_t len = std::strlen(word);
	for (const auto &row : rows_) {
		for (const auto &name : row.names) {
			if (len <= name.size() && !strn_cmp(word, name.c_str(), len)) {
				if (match && match != &row) {
					ambiguous = true;
					return nullptr;
				}
				match = &row;
				break;
			}
		}
	}
	return match;
}

int SubCmdResolver::Match(const char *word, bool &ambiguous) const {
	const Row *row = Resolve(word, ambiguous);
	return row ? row->id : -1;
}

int SubCmdResolver::Dispatch(CharData *ch, void *me, char *argument) const {
	char word[kMaxInputLength];
	char *rest = one_argument(argument, word);
	skip_spaces(&rest);

	if (!*word) {
		const std::string msg = greeting_ + " " + Tooltip() + "\r\n";
		SendMsgToChar(msg.c_str(), ch);
		return 1;
	}

	bool ambiguous = false;
	const Row *row = Resolve(word, ambiguous);
	if (ambiguous) {
		const std::string msg = "Уточните, что именно: " + Tooltip() + "\r\n";
		SendMsgToChar(msg.c_str(), ch);
		return 1;
	}
	if (!row) {
		SendMsgToChar("Вам явно в какое-то другое заведение.\r\n", ch);
		return 1;
	}
	return row->handler(ch, me, rest);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
