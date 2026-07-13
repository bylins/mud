/**
\file condition.h - a part of the Bylins engine.
\brief Character condition mechanic: hunger (full), thirst, drunkenness.
\detail issue.chardata-cleaning: the condition enums, thresholds and derived helpers, gathered out
        of char_data.h / utils.h / liquid.h. GET_COND (the raw conditions[] accessor) stays in
        utils.h -- it is an lvalue accessor used with both enum and plain-int indices.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_CONDITION_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_CONDITION_H_

class CharData;

// Drunkenness / condition thresholds (were loose consts in liquid.{h,cpp}). Kept at global scope:
// kDrunked/kMortallyDrunked collide by name with ESpell members, and these always coexisted as a
// global int vs a scoped enum value.
constexpr int kDrunked = 10;          // "drunk" from here
constexpr int kMortallyDrunked = 18;  // "mortally drunk" from here
constexpr int kMaxCondition = 48;
constexpr int kNormCondition = 22;    // satiety baseline

namespace condition {

// The three condition axes (index into conditions[] via GET_COND). Was enum { DRUNK, FULL, THIRST }.
enum ECondition { kDrunk, kFull, kThirst };

// What a condition penalty applies to. Was enum { P_DAMROLL, P_HITROLL, ... P_AC }.
enum EPenalty { kDamroll, kHitroll, kCast, kMemGain, kMoveGain, kHitGain, kAc };

// How far `cond` is above the satiety baseline (0 if at/below it). Was GET_COND_M.
[[nodiscard]] int GetCondAboveNorm(const CharData *ch, int cond);
// The above as a percentage of the full range. Was GET_COND_K.
[[nodiscard]] int GetCondAboveNormPercent(const CharData *ch, int cond);
// Combat/regen multiplier (0..1) from hunger+thirst for the given penalty axis. Was
// CharData::get_cond_penalty.
[[nodiscard]] float GetCondPenalty(const CharData *ch, EPenalty type);
// Impose the hangover (abstinence) affect on a drinker whose drunkenness wore off. Was
// CharData::set_abstinent; called from the affect update loop.

}  // namespace condition

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_CONDITION_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
