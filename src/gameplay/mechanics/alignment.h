/**
\file alignment.h - a part of the Bylins engine.
\authors Created by Sventovit.
\brief The character-alignment mechanic.
\detail issue.utils-cleaning: good/evil thresholds, the alignment accessor and the alignment-shift
        logic, gathered out of utils.h / char_data.h / fight so alignment can grow on its own.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_ALIGNMENT_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_ALIGNMENT_H_

#include <memory>

class CharData;

constexpr int kAligEvilLess = -300;  // alignment <= this is Evil
constexpr int kAligGoodMore = 300;   // alignment >= this is Good
constexpr int kAlignDelta = 10;      // SameAlign tolerance (was ALIGN_DELTA)

[[nodiscard]] int GetAlignment(const CharData *ch);  // was GetAlignment (read)
void SetAlignment(CharData *ch, int value);          // was GetAlignment (write)

[[nodiscard]] bool IsGood(const CharData *ch);
[[nodiscard]] bool IsEvil(const CharData *ch);
[[nodiscard]] bool IsNeutral(const CharData *ch);  // the band between Evil and Good (exclusive)
[[nodiscard]] inline bool IsGood(const std::shared_ptr<CharData> &ch) { return IsGood(ch.get()); }
[[nodiscard]] inline bool IsEvil(const std::shared_ptr<CharData> &ch) { return IsEvil(ch.get()); }
[[nodiscard]] inline bool IsNeutral(const std::shared_ptr<CharData> &ch) { return IsNeutral(ch.get()); }

// Are ch and vict within kAlignDelta of each other? (was the SameAlign macro)
[[nodiscard]] bool SameAlign(const CharData *ch, const CharData *vict);

// Shift ch's alignment 1/16 of the way toward -victim's, as a kill reward (was change_alignment).
void ChangeAlignment(CharData *ch, CharData *victim);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_ALIGNMENT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
