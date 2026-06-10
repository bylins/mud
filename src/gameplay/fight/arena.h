/**
\file arena.h - a part of the Bylins engine.
\authors Created by Sventovit.
\brief Arena combat helpers.
\detail issue.utils-cleaning: home for arena-specific rendering pulled out of utils.h.
*/

#ifndef BYLINS_SRC_GAMEPLAY_FIGHT_ARENA_H_
#define BYLINS_SRC_GAMEPLAY_FIGHT_ARENA_H_

#include "utils/grammar/cases.h"  // ECase

class CharData;
class ObjData;

namespace arena {

// A viewer's rendering of character `ch`'s name in grammatical case `pad`. On the arena
// (arena=true) the real name is always revealed (spectators must see the fighters);
// otherwise it falls back to normal visibility via sight::PersonName (real name if the
// viewer can see `ch`, else the indefinite-person placeholder). Was the APERS macro.
const char *VisibleName(const CharData *ch, const CharData *viewer, int pad, bool arena);

// Object-name mirror of VisibleName (was the AOBJS/AOBJN macros). On the arena the real name is
// always shown; otherwise visibility falls back via sight::CanSeeObj to grammar's "что-то".
const char *VisibleObjShort(const ObjData *obj, const CharData *viewer, bool arena);
const char *VisibleObjName(const ObjData *obj, const CharData *viewer, ECase pad, bool arena);

}  // namespace arena

#endif  // BYLINS_SRC_GAMEPLAY_FIGHT_ARENA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
