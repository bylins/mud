/**
\file karma.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "karma.h"

#include <fmt/format.h>

#include "engine/entities/char_data.h"

// Adds karma string to KARMA
// \TODO Move everything related to karma to this module.
void AddKarma(CharData *ch, const std::string &punish, const std::string &reason) {
	// Причина с ведущей точкой -- признак "записывать не надо".
	if (!reason.empty() && reason[0] == '.') {
		return;
	}

	const time_t nt = time(nullptr);
	// Раньше строка собиралась в буфер длиной kMaxInputLength и молча обрезалась:
	// длинное название наказания вместе с причиной туда не влезало.
	const std::string line = fmt::format("{} :: {} [{}]\r\n", rustime(localtime(&nt)), punish, reason);
	KARMA(ch) = str_add(KARMA(ch), line.c_str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
