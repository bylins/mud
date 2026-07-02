#ifndef _AFFECTHANDLER_HPP_
#define _AFFECTHANDLER_HPP_
#include "engine/entities/char_data.h"

// Remove the affect at `affect_i` from `ch`'s affect list and return the iterator after it
// (the removal counterpart of ImposeAffect). Was CharData::AffectRemove. The kAbstinent case
// clamps drunkenness back below the "drunk" threshold so the hangover does not re-trigger.
CharData::char_affects_list_t::iterator RemoveAffect(CharData *ch,
		const CharData::char_affects_list_t::iterator &affect_i);

// issue.damage-change: the old per-affect IAffectHandler system (AffectParent / *Parameters /
// IAffectHandler / CombatLuckAffectHandler / handle_affects) was an early attempt at active affects.
// It was never wired -- no affect ever set Affect::handler, so handle_affects() was a no-op -- and is
// superseded by the data-driven affect <actions> (talents_actions). Removed. This header now only
// declares RemoveAffect.

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
