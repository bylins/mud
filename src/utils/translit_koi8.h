/**
\file translit_koi8.h - a part of the Bylins engine.
\brief Closest KOI8-R equivalent for characters KOI8-R does not have (issue #3681).
*/

#ifndef BYLINS_SRC_UTILS_TRANSLIT_KOI8_H_
#define BYLINS_SRC_UTILS_TRANSLIT_KOI8_H_

#include <string>

namespace codepages {

// UTF-8 replacement for a code point KOI8-R cannot represent, or nullptr when there is no
// sensible one (the caller then falls back to its placeholder). Replacements may be longer
// than one character: "..." for an ellipsis, "(tm)" for a trademark sign.
const char *TranslitToKoi8(char32_t code_point);

// UTF-8 text -> KOI8-R, running the table above as a pre-pass. Doing the substitution before the
// conversion is what allows a replacement to be longer than one character: the converter itself
// writes exactly one output byte per code point. No replacement ever grows the text (pinned by a
// test), so the result is never longer than the input.
std::string Utf8ToKoi8(const std::string &text);

}  // namespace codepages

#endif  // BYLINS_SRC_UTILS_TRANSLIT_KOI8_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
