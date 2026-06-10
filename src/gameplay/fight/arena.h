/**
\file arena.h - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Arena combat helpers.
\detail issue.utils-cleaning: home for arena-specific rendering pulled out of utils.h.
*/

#ifndef BYLINS_SRC_GAMEPLAY_FIGHT_ARENA_H_
#define BYLINS_SRC_GAMEPLAY_FIGHT_ARENA_H_

class CharData;

namespace arena {

// A viewer's rendering of character `ch`'s name in grammatical case `pad`. On the arena
// (arena=true) the real name is always revealed (spectators must see the fighters);
// otherwise it falls back to normal visibility via sight::PersonName (real name if the
// viewer can see `ch`, else the indefinite-person placeholder). Was the APERS macro.
const char *VisibleName(const CharData *ch, const CharData *viewer, int pad, bool arena);

}  // namespace arena

#endif  // BYLINS_SRC_GAMEPLAY_FIGHT_ARENA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
