/**
\file utf8.h - a part of the Bylins engine.
\brief Character-semantic helpers over UTF-8 strings (issue #3681, "Plan Napoleon").

This is the encoding-agnostic building block for the KOI8-R -> UTF-8 migration: code that
must reason about *characters* (code points) rather than bytes -- length, substring, indexed
access, case folding -- lives here. Everything is plain UTF-8 in / UTF-8 out with no external
dependency (no iconv/ICU). ASCII passes through untouched, so the helpers are also correct for
pure-ASCII input regardless of the ambient encoding.

Case folding covers exactly what the legacy a_ucc/a_lcc tables covered: ASCII A-Z and the
Russian Cyrillic block (U+0410..U+044F plus Yo, U+0401/U+0451). Any other code point passes
through unchanged.
*/

#ifndef BYLINS_SRC_UTILS_UTF8_H_
#define BYLINS_SRC_UTILS_UTF8_H_

#include <cstddef>
#include <string>
#include <string_view>

namespace utf8 {

// Number of bytes the UTF-8 sequence starting with lead byte `c` claims to span.
// Returns 1 for ASCII and for any byte that cannot start a sequence, so a caller that
// advances by the result always makes forward progress.
int sequence_length(unsigned char c);

// Decode the code point starting at byte position `pos` in `s`.
// Writes the code point (or the raw byte, on a malformed sequence) to `cp` and returns the
// number of bytes consumed: >=1 while `pos` is in range, 0 once `pos >= s.size()`.
// Malformed sequences never stall: they yield the single offending byte and a length of 1.
std::size_t decode(std::string_view s, std::size_t pos, char32_t &cp);

// Append `cp` to `out` as UTF-8. Returns the number of bytes written, or 0 for a value that is
// not a valid Unicode scalar (surrogate half or > U+10FFFF), in which case `out` is untouched.
std::size_t encode(char32_t cp, std::string &out);

// Same, but writes into a caller-supplied buffer of at least 4 bytes and never allocates.
// Returns the number of bytes written, or 0 for a value that is not a Unicode scalar.
std::size_t encode(char32_t cp, char *out);

// Strict, whole-string well-formedness check per the Unicode Table 3-7 byte-sequence grammar
// (rejects overlong forms, surrogates, code points above U+10FFFF and stray continuation bytes).
bool is_valid(std::string_view s);

// Number of code points. Malformed bytes count as one code point each; never throws.
std::size_t length(std::string_view s);

// Byte offset of the `index`-th code point, or `s.size()` when `index` is past the end.
std::size_t byte_offset(std::string_view s, std::size_t index);

// The bytes of the `index`-th code point, as a view into `s`. Empty when `index` is out of range.
std::string_view char_at(std::string_view s, std::size_t index);

// std::string::substr, but `pos`/`count` are counted in code points instead of bytes.
std::string substr(std::string_view s, std::size_t pos, std::size_t count = std::string_view::npos);

// Single-code-point case folding (ASCII + Russian Cyrillic incl. Yo); other values pass through.
char32_t to_lower(char32_t cp);
char32_t to_upper(char32_t cp);

// Whole-string case folding. Malformed bytes are copied through verbatim.
std::string to_lower(std::string_view s);
std::string to_upper(std::string_view s);

}  // namespace utf8

#endif  // BYLINS_SRC_UTILS_UTF8_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
