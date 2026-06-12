/**
\file experience.h - a part of the Bylins engine.
\brief issue.chardata-cleaning: experience-gain rules.
\detail New home for experience-related logic pulled off char_data. Experience
        tables, gain/loss formulas, etc. should accrete here over time; for now it
        holds the zone group size and the "may this kill grant exp?" predicate.
*/

#ifndef BYLINS_SRC_GAMEPLAY_CORE_EXPERIENCE_H_
#define BYLINS_SRC_GAMEPLAY_CORE_EXPERIENCE_H_

class CharData;

namespace experience {

// Group size configured for the zone the mob `ch` belongs to; 1 for players/invalid.
int GetZoneGroup(const CharData *ch);

// Whether killing `victim` may grant `ch` experience (name ok, not on arena, real mob, ...).
bool OkGainExp(const CharData *ch, const CharData *victim);


// Experience needed to reach `level` (per-class/level table, scaled by remort count).
long GetExpUntilNextLvl(CharData *ch, int level);

// Number of changing remort exp coefficients (the rest are unchanged).
const int kMaxExpCoefficientsUsed = 15;

}  // namespace experience

#endif  // BYLINS_SRC_GAMEPLAY_CORE_EXPERIENCE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
