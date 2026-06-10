/**
\file arena.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Arena combat helpers.
*/

#include "arena.h"

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/sight.h"
#include "utils/utils.h"

namespace arena {

const char *VisibleName(const CharData *ch, const CharData *viewer, int pad, bool arena) {
	return arena ? GET_PAD(ch, pad) : PersonName(ch, viewer, pad);
}

}  // namespace arena

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
