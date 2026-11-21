/**
\file feat_timer.h - a part of the Bylins engine.
\brief The delay before a feat can be used again, from the <timed> tag of a feat in cfg/feats.xml.
\details <timed base="18" min="3" remort_divisor="1"/>: the delay is base - remorts / remort_divisor, but not
 less than min. A <timed/> without attributes only marks the feat as timed; its delay is set in code.
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_TIMER_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_TIMER_H_

#include "feats_constants.h"
#include "utils/parser_wrapper.h"

#include <optional>
#include <string>

namespace feats {

struct FeatTimer {
	static constexpr int kMaxDelay{255};	// TimedFeat::time is one byte

	bool has_base{false};	// false: a plain marker, the delay is computed in code
	int base{0};
	int min{1};
	int remort_divisor{0};	// 0 -- remorts do not shorten the delay

	// The delay for a character with `remorts` remorts; only meaningful with has_base.
	[[nodiscard]] int CalcDelay(int remorts) const;
	// The delay as text for immortals, e.g. "18 - реморты ч, не меньше 3 ч".
	[[nodiscard]] std::string Describe() const;

	// Reads the <timed> child of the <feat> node `node` points at. No tag -- nullopt; an invalid tag is
	// logged and also gives nullopt.
	[[nodiscard]] static std::optional<FeatTimer> Parse(parser_wrapper::DataNode &node, EFeat owner);
};

} // namespace feats

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_TIMER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
