#include "affect_handler.h"

#include "gameplay/mechanics/condition.h"   // condition::kDrunk, kDrunked
#include "utils/utils.h"                     // GET_NAME, GET_COND, GET_DRUNK_STATE

#include <algorithm>

CharData::char_affects_list_t::iterator RemoveAffect(CharData *ch,
		const CharData::char_affects_list_t::iterator &affect_i) {
	if (ch->affected.empty()) {
		log("SYSERR: affect_remove(%s) when no affects...", GET_NAME(ch));
		return ch->affected.end();
	}
	const auto af = *affect_i;
	if (af->affect_type == EAffect::kAbstinent) {
		if (ch->player_specials) {
			GET_DRUNK_STATE(ch) = GET_COND(ch, condition::kDrunk) = std::min(GET_COND(ch, condition::kDrunk), kDrunked - 1);
		} else {
			log("SYSERR: player_specials is not set.");
		}
	}
	return ch->affected.erase(affect_i);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
