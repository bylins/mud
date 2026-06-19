/**
\file alignment.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\brief The character-alignment mechanic.
*/

#include "alignment.h"

#include "engine/entities/char_data.h"

namespace alignment {

int GetAlignment(const CharData *ch) { return ch->char_specials.saved.alignment; }
void SetAlignment(CharData *ch, int value) { ch->char_specials.saved.alignment = value; }

bool IsGood(const CharData *ch) { return GetAlignment(ch) >= kAligGoodMore; }
bool IsEvil(const CharData *ch) { return GetAlignment(ch) <= kAligEvilLess; }
bool IsNeutral(const CharData *ch) { return !IsGood(ch) && !IsEvil(ch); }

bool SameAlign(const CharData *ch, const CharData *vict) {
	return GetAlignment(ch) > GetAlignment(vict)
		? (GetAlignment(ch) - GetAlignment(vict)) <= kAlignDelta
		: (GetAlignment(vict) - GetAlignment(ch)) <= kAlignDelta;
}

void ChangeAlignment(CharData *ch, CharData *victim) {
	// new alignment change algorithm: if you kill a monster with alignment A,
	// you move 1/16th of the way to having alignment -A. Simple and fast.
	SetAlignment(ch, GetAlignment(ch) + (-GetAlignment(victim) - GetAlignment(ch)) / 16);
}

}  // namespace alignment

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
