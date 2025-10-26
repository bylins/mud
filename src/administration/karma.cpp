/**
\file karma.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"

// Adds karma string to KARMA
// \TODO Move everything related to karma to this module.
void AddKarma(CharData *ch, const char *punish, const char *reason) {
	if (reason && (reason[0] != '.')) {
		char smallbuf[kMaxInputLength];
		time_t nt = time(nullptr);
		snprintf(smallbuf, kMaxInputLength, "%s :: %s [%s]\r\n", rustime(localtime(&nt)), punish, reason);
		KARMA(ch) = str_add(KARMA(ch), smallbuf);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
