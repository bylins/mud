/**
\file feat_text.h - a part of the Bylins engine.
\brief Shared pieces of the feat text that `show featinfo` prints.
*/

#ifndef BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_TEXT_H_
#define BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_TEXT_H_

#include <fmt/format.h>

#include <string>

namespace feats {

// The remorts term of a formula: "реморты" for a divisor of 1, otherwise "реморты/N".
inline std::string RemortsShare(int divisor) {
	return divisor == 1 ? std::string("реморты") : fmt::format("реморты/{}", divisor);
}

} // namespace feats

#endif // BYLINS_SRC_GAMEPLAY_ABILITIES_FEAT_TEXT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
