/**
\file native_text.h - a part of the Bylins engine.
\brief Character-semantic operations in the engine's *native runtime encoding* (issue #3681).

The migration flips the engine's internal string encoding from KOI8-R (1 byte == 1 character) to
UTF-8 (multibyte). Code that must reason about characters -- counting display width, capitalising
a letter, truncating without splitting a character -- should go through this thin dispatch layer
instead of assuming bytes. The active encoding is chosen at build time by the `internal_encoding`
Meson option, which defines INTERNAL_ENCODING_UTF8 for the UTF-8 build:

  * KOI8-R (default, current behaviour): every helper is byte-for-byte identical to the open-coded
    byte logic it replaces, so routing call sites through it is a no-op.
  * UTF-8: helpers operate on code points via the utf8:: primitives.

This lets the whole byte-vs-char refactor land and ship on KOI8-R (safely, as a no-op) ahead of
the encoding flip. Once the flip is permanent the KOI8-R branch and this indirection are removed.
*/

#ifndef BYLINS_SRC_UTILS_NATIVE_TEXT_H_
#define BYLINS_SRC_UTILS_NATIVE_TEXT_H_

#include <cstddef>
#include <string_view>

namespace native_text {

// True iff the engine was built with UTF-8 as its native runtime encoding. Reflects the flag the
// library itself was compiled with (not the translation unit that calls this), so callers/tests
// can branch reliably regardless of their own compile flags.
bool native_is_utf8();

// Number of display characters in a byte range / view. KOI8-R: the byte count. UTF-8: the number
// of code points (malformed bytes counted as one each, so it never stalls on legacy data).
std::size_t char_count(const char *begin, const char *end);
std::size_t char_count(std::string_view s);

// Uppercase the first character of the null-terminated string `s` in place (ASCII + Russian
// Cyrillic incl. Yo). No-op on an empty string, on a non-cased first character, or in the (never
// occurring for these alphabets) case where the uppercase form has a different byte length.
void capitalize_first(char *s);

// Largest byte offset <= max_bytes that lands on a character boundary, so cutting the string
// there never splits a multibyte character. KOI8-R: min(max_bytes, s.size()).
std::size_t truncate_offset(std::string_view s, std::size_t max_bytes);

}  // namespace native_text

#endif  // BYLINS_SRC_UTILS_NATIVE_TEXT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
